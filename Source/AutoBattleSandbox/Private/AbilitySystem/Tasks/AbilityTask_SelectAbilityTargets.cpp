#include "AbilitySystem/Tasks/AbilityTask_SelectAbilityTargets.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "AbilitySystem/Targeting/AbilityTargetingDefinition.h"
#include "AbilitySystem/Targeting/AbilityTargetingSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UAbilityTask_SelectAbilityTargets* UAbilityTask_SelectAbilityTargets::SelectAbilityTargets(
	UBattleGameplayAbility* OwningAbility,
	UAbilityTargetingDefinition* TargetingDefinition,
	FAbilityTargetSelectionInput SelectionInput)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_SelectAbilityTargets* Task = NewAbilityTask<UAbilityTask_SelectAbilityTargets>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetingDefinition = TargetingDefinition;
	Task->SelectionInput = MoveTemp(SelectionInput);
	return Task;
}

UScriptStruct* UAbilityTask_SelectAbilityTargets::GetTaskParametersStruct() const
{
	return FSelectAbilityTargetsTaskParameters::StaticStruct();
}

bool UAbilityTask_SelectAbilityTargets::InitializeFromTaskParameters(const FInstancedStruct& Parameters)
{
	const FSelectAbilityTargetsTaskParameters* TaskParameters =
		Parameters.GetPtr<FSelectAbilityTargetsTaskParameters>();
	if (!TaskParameters)
	{
		return false;
	}

	TargetingDefinition = TaskParameters->TargetingDefinition;
	SelectionInput = FAbilityTargetSelectionInput();
	SelectionInput.Policy = TaskParameters->InputPolicy;
	SelectionInput.ProvidedTargetData = CachedAbilityContext.TargetData;
	if (const AActor* ContextTarget = CachedAbilityContext.Target.Get())
	{
		SelectionInput.ProvidedActors.Add(const_cast<AActor*>(ContextTarget));
	}

	return TargetingDefinition != nullptr;
}

bool UAbilityTask_SelectAbilityTargets::ActivateModularAbilityTask()
{
	if (!Ability || !TargetingDefinition)
	{
		Fail(EAbilityTargetSelectionFailure::InvalidDefinition);
		return true;
	}

	UWorld* World = Ability->GetWorld();
	UAbilityTargetingSubsystem* TargetingSubsystemInstance = World
		                                                         ? World->GetSubsystem<UAbilityTargetingSubsystem>()
		                                                         : nullptr;
	if (!TargetingSubsystemInstance)
	{
		Fail(EAbilityTargetSelectionFailure::InvalidDefinition);
		return true;
	}

	TargetingSubsystem = TargetingSubsystemInstance;
	TargetingContext.Ability = Ability;
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	TargetingContext.SourceActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	TargetingContext.AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	const FGameplayAbilityTargetDataHandle ProvidedTargetData = TargetingSubsystemInstance->MakeTargetData(
		SelectionInput.ProvidedTargetData,
		SelectionInput.ProvidedActors);

	if (SelectionInput.Policy != EAbilityTargetInputPolicy::NormalDefinition)
	{
		if (!ProvidedTargetData.IsValid(0))
		{
			if (SelectionInput.Policy == EAbilityTargetInputPolicy::RequireProvidedTargets)
			{
				Fail(EAbilityTargetSelectionFailure::MissingProvidedTargets);
				return true;
			}
		}
		else
		{
			EAbilityTargetSelectionFailure ValidationFailure;
			if (TargetingSubsystemInstance->ValidateTargetData(
				TargetingContext,
				*TargetingDefinition,
				ProvidedTargetData,
				ValidationFailure))
			{
				Complete(ProvidedTargetData);
				return true;
			}

			if (SelectionInput.Policy == EAbilityTargetInputPolicy::RequireProvidedTargets)
			{
				Fail(ValidationFailure);
				return true;
			}
		}
	}

	RunDefinitionSelection();
	return true;
}

void UAbilityTask_SelectAbilityTargets::OnDestroy(const bool bAbilityEnding)
{
	if (UBattleTargetSelectionComponent* SelectionComponent = ManualSelectionComponent.Get();
		SelectionComponent && ManualRequestHandle.IsValid())
	{
		SelectionComponent->CancelSelection(ManualRequestHandle, false);
	}

	ManualRequestHandle = FAbilityTargetSelectionRequestHandle();
	ManualSelectionComponent.Reset();
	UnbindSequenceDelegates();
	Super::OnDestroy(bAbilityEnding);
}

void UAbilityTask_SelectAbilityTargets::RunDefinitionSelection()
{
	EAbilityTargetSelectionFailure DefinitionFailure;
	if (!TargetingDefinition->IsDefinitionValid(false, DefinitionFailure))
	{
		Fail(DefinitionFailure);
		return;
	}

	switch (TargetingDefinition->SelectionMode)
	{
	case EAbilityTargetSelectionMode::Automatic:
		RunAutomaticSelection();
		break;

	case EAbilityTargetSelectionMode::Manual:
		if (!TryBeginManualSelection())
		{
			Fail(EAbilityTargetSelectionFailure::NoManualSelectionProvider);
		}
		break;

	case EAbilityTargetSelectionMode::ManualWithAutomaticFallback:
		if (!TryBeginManualSelection())
		{
			RunAutomaticSelection();
		}
		break;

	default:
		Fail(EAbilityTargetSelectionFailure::InvalidDefinition);
		break;
	}
}

void UAbilityTask_SelectAbilityTargets::RunAutomaticSelection()
{
	const UAbilityTargetingSubsystem* TargetingSubsystemInstance = TargetingSubsystem.Get();
	if (!TargetingSubsystemInstance || !TargetingDefinition)
	{
		Fail(EAbilityTargetSelectionFailure::InvalidDefinition);
		return;
	}

	const FAbilityTargetSelectionResult Result = TargetingSubsystemInstance->SelectAutomatically(
		TargetingContext,
		*TargetingDefinition);
	if (Result.IsSuccessful())
	{
		Complete(Result.TargetData);
	}
	else
	{
		Fail(Result.Failure);
	}
}

bool UAbilityTask_SelectAbilityTargets::TryBeginManualSelection()
{
	UWorld* World = Ability ? Ability->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	UBattleTargetSelectionComponent* SelectionComponent = PlayerController
		                                                      ? PlayerController->FindComponentByClass<
			                                                      UBattleTargetSelectionComponent>()
		                                                      : nullptr;
	if (!SelectionComponent || !SelectionComponent->CanBeginSelection())
	{
		return false;
	}

	const UAbilityTargetingSubsystem* TargetingSubsystemInstance = TargetingSubsystem.Get();
	if (!TargetingSubsystemInstance)
	{
		return false;
	}

	TArray<FAbilityTargetCandidate> Candidates;
	TargetingSubsystemInstance->FindValidCandidates(TargetingContext, *TargetingDefinition, Candidates);
	if (Candidates.Num() < TargetingDefinition->MinimumTargets)
	{
		Fail(Candidates.IsEmpty()
			     ? EAbilityTargetSelectionFailure::NoValidTargets
			     : EAbilityTargetSelectionFailure::NotEnoughTargets);
		return true;
	}

	FAbilityManualSelectionRequest Request;
	Request.MinimumTargets = TargetingDefinition->MinimumTargets;
	Request.MaximumTargets = TargetingDefinition->MaximumTargets;
	for (const FAbilityTargetCandidate& Candidate : Candidates)
	{
		Request.Candidates.Add(Candidate.Actor);
	}

	ManualSelectionComponent = SelectionComponent;
	ManualRequestHandle = SelectionComponent->BeginSelection(
		Request,
		FAbilityManualTargetSelectionCompleted::CreateUObject(
			this,
			&ThisClass::HandleManualSelectionCompleted),
		FAbilityManualTargetSelectionCancelled::CreateUObject(
			this,
			&ThisClass::HandleManualSelectionCancelled));
	if (!ManualRequestHandle.IsValid())
	{
		ManualSelectionComponent.Reset();
		return false;
	}

	BindSequenceDelegates();
	return true;
}

void UAbilityTask_SelectAbilityTargets::HandleManualSelectionCompleted(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	ManualRequestHandle = FAbilityTargetSelectionRequestHandle();
	ManualSelectionComponent.Reset();

	const UAbilityTargetingSubsystem* TargetingSubsystemInstance = TargetingSubsystem.Get();
	EAbilityTargetSelectionFailure ValidationFailure = EAbilityTargetSelectionFailure::InvalidDefinition;
	if (!TargetingSubsystemInstance || !TargetingDefinition
		|| !TargetingSubsystemInstance->ValidateTargetData(
			TargetingContext,
			*TargetingDefinition,
			TargetData,
			ValidationFailure))
	{
		Fail(TargetingSubsystemInstance
			     ? ValidationFailure
			     : EAbilityTargetSelectionFailure::InvalidDefinition);
		return;
	}

	Complete(TargetData);
}

void UAbilityTask_SelectAbilityTargets::HandleManualSelectionCancelled()
{
	ManualRequestHandle = FAbilityTargetSelectionRequestHandle();
	ManualSelectionComponent.Reset();
	Cancel();
}

void UAbilityTask_SelectAbilityTargets::Complete(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnSelected.Broadcast(TargetData);
	}

	UnbindSequenceDelegates();
	FModularGameplayAbilityContext UpdatedContext = CachedAbilityContext;
	UpdatedContext.TargetData = TargetData;
	for (int32 DataIndex = 0; DataIndex < TargetData.Num() && !UpdatedContext.Target; ++DataIndex)
	{
		const FGameplayAbilityTargetData* Data = TargetData.Get(DataIndex);
		if (!Data)
		{
			continue;
		}

		for (const TWeakObjectPtr<AActor>& TargetActor : Data->GetActors())
		{
			if (TargetActor.IsValid())
			{
				UpdatedContext.Target = TargetActor.Get();
				break;
			}
		}
	}
	CompleteModularAbilityTask(UpdatedContext);
}

void UAbilityTask_SelectAbilityTargets::Fail(const EAbilityTargetSelectionFailure Failure)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFailed.Broadcast(Failure);
	}
	UnbindSequenceDelegates();
	CancelModularAbilityTask();
}

void UAbilityTask_SelectAbilityTargets::Cancel()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast();
	}
	UnbindSequenceDelegates();
	CancelModularAbilityTask();
}

void UAbilityTask_SelectAbilityTargets::BindSequenceDelegates()
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
	const UBattleAbilitySystem* BattleAbilitySystem = ActorInfo
		                                                  ? Cast<UBattleAbilitySystem>(
			                                                  ActorInfo->AbilitySystemComponent.Get())
		                                                  : nullptr;
	if (!BattleAbilitySystem
		|| !BattleAbilitySystem->GetSequenceActivation(
			Ability->GetCurrentAbilitySpecHandle(),
			SequenceId,
			SequenceNodeId))
	{
		return;
	}

	UWorld* World = Ability->GetWorld();
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = World
		                                                                 ? World->GetSubsystem<
			                                                                 UAutoBattleAbilitySequenceSubsystem>()
		                                                                 : nullptr;
	if (!SequenceSubsystemInstance)
	{
		return;
	}

	SequenceSubsystem = SequenceSubsystemInstance;
	SequenceSubsystemInstance->OnSequenceNodeStateChanged.AddDynamic(
		this,
		&ThisClass::HandleSequenceNodeStateChanged);
	SequenceSubsystemInstance->OnSequenceFinished.AddDynamic(
		this,
		&ThisClass::HandleSequenceFinished);
}

void UAbilityTask_SelectAbilityTargets::UnbindSequenceDelegates()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceFinished);
	}
	SequenceSubsystem.Reset();
}

void UAbilityTask_SelectAbilityTargets::HandleSequenceNodeStateChanged(
	const FAbilitySequenceId InSequenceId,
	const FAbilitySequenceNode Node)
{
	if (InSequenceId != SequenceId || Node.NodeId != SequenceNodeId)
	{
		return;
	}

	UBattleTargetSelectionComponent* SelectionComponent = ManualSelectionComponent.Get();
	if (!SelectionComponent || !ManualRequestHandle.IsValid())
	{
		return;
	}

	if (Node.State == EAbilitySequenceNodeState::Suspended)
	{
		SelectionComponent->SuspendSelection(ManualRequestHandle);
	}
	else if (Node.State == EAbilitySequenceNodeState::Active)
	{
		SelectionComponent->ResumeSelection(ManualRequestHandle);
	}
	else if (Node.State == EAbilitySequenceNodeState::Finished)
	{
		Cancel();
	}
}

void UAbilityTask_SelectAbilityTargets::HandleSequenceFinished(
	const FAbilitySequenceId InSequenceId,
	const EAbilitySequenceState)
{
	if (InSequenceId == SequenceId)
	{
		Cancel();
	}
}
