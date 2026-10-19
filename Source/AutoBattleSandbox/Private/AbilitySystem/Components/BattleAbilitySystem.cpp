#include "AbilitySystem/Components/BattleAbilitySystem.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Events/GlobalGameplayEventSubsystem.h"
#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "Actors/UnitActor.h"
#include "Combat/AutoBattleGameplayTags.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"

UBattleAbilitySystem::UBattleAbilitySystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UBattleAbilitySystem::BroadcastGlobalGameplayEvent(
	const FGameplayAbilitySpecHandle SourceAbilityHandle,
	const FGameplayTag EventTag,
	const FGameplayEventData& EventData,
	FGlobalGameplayEventDispatch& OutDispatch)
{
	OutDispatch = FGlobalGameplayEventDispatch();

	UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get();
	const FBattleAbilitySequenceActivation* SourceActivation =
		ActiveSequenceActivations.Find(SourceAbilityHandle);
	if (!GlobalEventSubsystemInstance || !SourceActivation || !EventTag.IsValid())
	{
		return false;
	}

	return GlobalEventSubsystemInstance->BroadcastGlobalEvent(
		SourceActivation->SequenceId,
		SourceActivation->NodeId,
		EventTag,
		EventData,
		OutDispatch);
}

bool UBattleAbilitySystem::TryActivateAbilityWithTarget(
	const TSubclassOf<UBattleGameplayAbility> AbilityClass,
	AActor* Target)
{
	FAbilityTargetSelectionInput TargetingInput;
	TargetingInput.Policy = EAbilityTargetInputPolicy::RequireProvidedTargets;
	if (IsValid(Target))
	{
		TargetingInput.ProvidedActors.Add(Target);
	}
	return TryActivateAbilityWithTargetingInput(AbilityClass, TargetingInput);
}

bool UBattleAbilitySystem::TryActivateAbilityWithTargetingInput(
	const TSubclassOf<UBattleGameplayAbility> AbilityClass,
	const FAbilityTargetSelectionInput& TargetingInput)
{
	FGameplayAbilitySpec* AbilitySpec = AbilityClass ? FindAbilitySpecFromClass(AbilityClass) : nullptr;
	if (!AbilitySpec || AbilitySpec->IsActive())
	{
		return false;
	}

	FBattleAbilityActivationContext ActivationContext;
	ActivationContext.TargetingInput = TargetingInput;
	ActivationContext.bHasTargetingInput = true;
	PushActivationContext(AbilitySpec->Handle, ActivationContext);
	const bool bActivated = TryActivateAbility(AbilitySpec->Handle);
	if (!bActivated)
	{
		DiscardActivationContext(AbilitySpec->Handle);
	}
	return bActivated;
}

bool UBattleAbilitySystem::TryActivateAbilityWithTargetAndSequence(
	const TSubclassOf<UBattleGameplayAbility> AbilityClass,
	AActor* Target,
	FAbilitySequenceId& OutSequenceId)
{
	OutSequenceId = FAbilitySequenceId();
	const FGameplayAbilitySpec* AbilitySpec = AbilityClass ? FindAbilitySpecFromClass(AbilityClass) : nullptr;
	if (!AbilitySpec || AbilitySpec->IsActive())
	{
		return false;
	}

	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySpec->Handle;
	LastSequenceIds.Remove(AbilityHandle);
	if (!TryActivateAbilityWithTarget(AbilityClass, Target))
	{
		return false;
	}

	if (const FAbilitySequenceId* SequenceId = LastSequenceIds.Find(AbilityHandle))
	{
		OutSequenceId = *SequenceId;
	}
	return true;
}

bool UBattleAbilitySystem::GetSequenceActivation(
	const FGameplayAbilitySpecHandle AbilityHandle,
	FAbilitySequenceId& OutSequenceId,
	FAbilitySequenceNodeId& OutNodeId) const
{
	OutSequenceId = FAbilitySequenceId();
	OutNodeId = FAbilitySequenceNodeId();
	const FBattleAbilitySequenceActivation* Activation = ActiveSequenceActivations.Find(AbilityHandle);
	if (!Activation)
	{
		return false;
	}

	OutSequenceId = Activation->SequenceId;
	OutNodeId = Activation->NodeId;
	return OutSequenceId.IsValid() && OutNodeId.IsValid();
}

void UBattleAbilitySystem::BeginPlay()
{
	Super::BeginPlay();
	InitializeSequenceIntegration();
}

void UBattleAbilitySystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownSequenceIntegration();
	Super::EndPlay(EndPlayReason);
}

void UBattleAbilitySystem::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	RegisterAbilitySpec(AbilitySpec);
}

void UBattleAbilitySystem::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySpec.Handle;
	Super::OnRemoveAbility(AbilitySpec);
	UnregisterAbilitySpec(AbilityHandle);
}

void UBattleAbilitySystem::NotifyAbilityActivated(
	const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability)
{
	UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (BattleAbility)
	{
		FBattleAbilityActivationContext ActivationContext;
		ConsumeActivationContext(Handle, ActivationContext);
		BattleAbility->SetActivationContext(ActivationContext);

		FBattleAbilitySequenceActivation Activation;
		bool bSequenceActivationReady;
		if (const FBattleAbilitySequenceActivation* PendingActivation = PendingSequenceActivations.Find(Handle))
		{
			Activation = *PendingActivation;
			PendingSequenceActivations.Remove(Handle);

			UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
			bSequenceActivationReady = SequenceSubsystemInstance
				&& SequenceSubsystemInstance->MarkNodeActive(Activation.SequenceId, Activation.NodeId);
			if (!bSequenceActivationReady)
			{
				FinishSequenceActivation(Activation, EAbilitySequenceNodeResult::ActivationFailed);
			}
		}
		else
		{
			// StartSequence creates its root node in the Active state, so it must not be
			// passed through MarkNodeActive, which only accepts child nodes in Activating.
			bSequenceActivationReady = CreateRootSequenceActivation(*BattleAbility, Activation);
		}

		if (bSequenceActivationReady && Activation.SequenceId.IsValid() && Activation.NodeId.IsValid())
		{
			LastSequenceIds.Add(Handle, Activation.SequenceId);
			ActiveSequenceActivations.Add(Handle, Activation);
		}
	}

	Super::NotifyAbilityActivated(Handle, Ability);
}

void UBattleAbilitySystem::NotifyAbilityEnded(
	const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability,
	const bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
	AbilitiesWaitingForGlobalEvent.Remove(Handle);
	if (UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability))
	{
		BattleAbility->SetActivationContext(FBattleAbilityActivationContext());
	}

	FBattleAbilitySequenceActivation Activation;
	if (!ActiveSequenceActivations.RemoveAndCopyValue(Handle, Activation))
	{
		return;
	}

	FinishSequenceActivation(
		Activation,
		bWasCancelled ? EAbilitySequenceNodeResult::Cancelled : EAbilitySequenceNodeResult::Succeeded);
}

void UBattleAbilitySystem::InitializeSequenceIntegration()
{
	if (ActivationRequestDelegateHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SequenceSubsystem = World->GetSubsystem<UAutoBattleAbilitySequenceSubsystem>();
	GlobalEventSubsystem = World->GetSubsystem<UAutoBattleGlobalGameplayEventSubsystem>();
	UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get();
	if (!SequenceSubsystem.IsValid() || !GlobalEventSubsystemInstance)
	{
		SequenceSubsystem.Reset();
		GlobalEventSubsystem.Reset();
		return;
	}

	ActivationRequestDelegateHandle = GlobalEventSubsystemInstance->OnAbilityActivationRequested().AddUObject(
		this,
		&ThisClass::HandleGlobalAbilityActivationRequested);
	SequenceSubsystem->OnSequenceNodeStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleSequenceNodeStateChanged);
	SequenceSubsystem->OnSequenceFinished.AddUniqueDynamic(this, &ThisClass::HandleSequenceFinished);

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		RegisterAbilitySpec(AbilitySpec);
	}
}

void UBattleAbilitySystem::ShutdownSequenceIntegration()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);

		if (DeathSequenceId.IsValid() && SequenceSubsystemInstance->IsSequenceRunning(DeathSequenceId))
		{
			TArray<FAbilitySequenceNodeId> IgnoredNodesToCancel;
			SequenceSubsystemInstance->CancelSequence(DeathSequenceId, IgnoredNodesToCancel);
		}
	}

	if (UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get())
	{
		if (ActivationRequestDelegateHandle.IsValid())
		{
			GlobalEventSubsystemInstance->OnAbilityActivationRequested().Remove(ActivationRequestDelegateHandle);
		}

		GlobalEventSubsystemInstance->UnregisterAbilitySystem(this);
	}

	ActivationRequestDelegateHandle.Reset();
	GlobalEventSubscriptions.Reset();
	PendingSequenceActivations.Reset();
	LastSequenceIds.Reset();
	PendingActivationContexts.Reset();
	AbilitiesWaitingForGlobalEvent.Reset();
	DeathSequenceId = FAbilitySequenceId();
	DeathSequenceRootNodeId = FAbilitySequenceNodeId();
	DeathSequencePhase = EBattleDeathSequencePhase::None;

	TArray<FBattleAbilitySequenceActivation> Activations;
	ActiveSequenceActivations.GenerateValueArray(Activations);
	ActiveSequenceActivations.Reset();
	for (const FBattleAbilitySequenceActivation& Activation : Activations)
	{
		FinishSequenceActivation(Activation, EAbilitySequenceNodeResult::Cancelled);
	}

	GlobalEventSubsystem.Reset();
	SequenceSubsystem.Reset();
}

void UBattleAbilitySystem::RegisterAbilitySpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (GlobalEventSubscriptions.Contains(AbilitySpec.Handle))
	{
		return;
	}

	UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get();
	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(AbilitySpec.Ability);
	if (!GlobalEventSubsystemInstance || !BattleAbility || BattleAbility->GetGlobalEventTriggers().IsEmpty())
	{
		return;
	}

	const FGlobalGameplayAbilitySubscriptionId SubscriptionId =
		GlobalEventSubsystemInstance->RegisterAbilitySubscription(
			this,
			AbilitySpec.Handle,
			BattleAbility->GetClass());
	if (SubscriptionId.IsValid())
	{
		GlobalEventSubscriptions.Add(AbilitySpec.Handle, SubscriptionId);
	}
}

void UBattleAbilitySystem::UnregisterAbilitySpec(const FGameplayAbilitySpecHandle AbilityHandle)
{
	LastSequenceIds.Remove(AbilityHandle);
	AbilitiesWaitingForGlobalEvent.Remove(AbilityHandle);

	FGlobalGameplayAbilitySubscriptionId SubscriptionId;
	if (!GlobalEventSubscriptions.RemoveAndCopyValue(AbilityHandle, SubscriptionId))
	{
		return;
	}

	if (UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get())
	{
		GlobalEventSubsystemInstance->UnregisterAbilitySubscription(SubscriptionId);
	}
}

void UBattleAbilitySystem::HandleGlobalAbilityActivationRequested(
	const FGlobalGameplayAbilityActivationRequest& Request)
{
	if (Request.AbilitySystem != this)
	{
		return;
	}

	FBattleAbilitySequenceActivation Activation;
	Activation.SequenceId = Request.SequenceId;
	Activation.NodeId = Request.NodeId;
	Activation.EventId = Request.EventId;

	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(Request.AbilityHandle);
	if (!AbilitySpec || !AbilitySpec->Ability || !Request.AbilityClass
		|| !AbilitySpec->Ability->IsA(Request.AbilityClass)
		|| AbilitySpec->IsActive()
		|| PendingSequenceActivations.Contains(Request.AbilityHandle)
		|| ActiveSequenceActivations.Contains(Request.AbilityHandle)
		|| !AbilityActorInfo.IsValid())
	{
		FinishSequenceActivation(Activation, EAbilitySequenceNodeResult::ActivationFailed);
		return;
	}

	PendingSequenceActivations.Add(Request.AbilityHandle, Activation);
	FBattleAbilityActivationContext ActivationContext;
	ActivationContext.EventData = Request.EventData;
	ActivationContext.bHasEventData = true;
	ActivationContext.TargetingInput = Request.TargetingInput;
	ActivationContext.bHasTargetingInput = true;
	PushActivationContext(Request.AbilityHandle, ActivationContext);
	const bool bActivated = TriggerAbilityFromGameplayEvent(
		Request.AbilityHandle,
		AbilityActorInfo.Get(),
		Request.EventData.EventTag,
		&Request.EventData,
		*this);
	if (!bActivated)
	{
		DiscardActivationContext(Request.AbilityHandle);
		FBattleAbilitySequenceActivation FailedActivation;
		if (PendingSequenceActivations.RemoveAndCopyValue(Request.AbilityHandle, FailedActivation))
		{
			FinishSequenceActivation(FailedActivation, EAbilitySequenceNodeResult::ActivationFailed);
		}
	}
}

void UBattleAbilitySystem::HandleSequenceNodeStateChanged(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNode Node)
{
	if (Node.State == EAbilitySequenceNodeState::Active)
	{
		ResumeAbilityWaitingForGlobalEvent(SequenceId, Node.NodeId);
	}

	if (SequenceId == DeathSequenceId
		&& Node.NodeId == DeathSequenceRootNodeId
		&& Node.State == EAbilitySequenceNodeState::Active)
	{
		ContinueDeathSequence();
	}
}

void UBattleAbilitySystem::SetAbilityWaitingForGlobalEvent(
	const FGameplayAbilitySpecHandle AbilityHandle,
	const bool bWaiting)
{
	if (!AbilityHandle.IsValid())
	{
		return;
	}

	if (bWaiting)
	{
		AbilitiesWaitingForGlobalEvent.Add(AbilityHandle);
	}
	else
	{
		AbilitiesWaitingForGlobalEvent.Remove(AbilityHandle);
	}
}

void UBattleAbilitySystem::ResumeAbilityWaitingForGlobalEvent(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId)
{
	FGameplayAbilitySpecHandle AbilityHandle;
	for (const TPair<FGameplayAbilitySpecHandle, FBattleAbilitySequenceActivation>& Pair
	     : ActiveSequenceActivations)
	{
		if (Pair.Value.SequenceId == SequenceId
			&& Pair.Value.NodeId == NodeId
			&& AbilitiesWaitingForGlobalEvent.Contains(Pair.Key))
		{
			AbilityHandle = Pair.Key;
			break;
		}
	}

	if (!AbilityHandle.IsValid())
	{
		return;
	}

	AbilitiesWaitingForGlobalEvent.Remove(AbilityHandle);
	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilityHandle);
	UBattleGameplayAbility* Ability = AbilitySpec
		                                  ? Cast<UBattleGameplayAbility>(AbilitySpec->GetPrimaryInstance())
		                                  : nullptr;
	if (Ability && Ability->IsAbilityFrozen())
	{
		Ability->ResumeAbility();
	}
}

void UBattleAbilitySystem::HandleSequenceFinished(
	const FAbilitySequenceId SequenceId,
	const EAbilitySequenceState)
{
	if (SequenceId != DeathSequenceId)
	{
		TryStartDeathSequence();
	}
}

void UBattleAbilitySystem::PushActivationContext(
	const FGameplayAbilitySpecHandle AbilityHandle,
	const FBattleAbilityActivationContext& ActivationContext)
{
	if (AbilityHandle.IsValid())
	{
		PendingActivationContexts.FindOrAdd(AbilityHandle).Add(ActivationContext);
	}
}

bool UBattleAbilitySystem::ConsumeActivationContext(
	const FGameplayAbilitySpecHandle AbilityHandle,
	FBattleAbilityActivationContext& OutActivationContext)
{
	OutActivationContext = FBattleAbilityActivationContext();
	TArray<FBattleAbilityActivationContext>* ContextStack = PendingActivationContexts.Find(AbilityHandle);
	if (!ContextStack || ContextStack->IsEmpty())
	{
		return false;
	}

	OutActivationContext = ContextStack->Pop(EAllowShrinking::No);
	if (ContextStack->IsEmpty())
	{
		PendingActivationContexts.Remove(AbilityHandle);
	}
	return true;
}

void UBattleAbilitySystem::DiscardActivationContext(const FGameplayAbilitySpecHandle AbilityHandle)
{
	FBattleAbilityActivationContext IgnoredContext;
	ConsumeActivationContext(AbilityHandle, IgnoredContext);
}

void UBattleAbilitySystem::TryStartDeathSequence()
{
	AUnitActor* UnitActor = Cast<AUnitActor>(GetOwner());
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (bDeathResolved || DeathSequencePhase != EBattleDeathSequencePhase::None
		|| !IsValid(UnitActor) || UnitActor->IsAlive() || !SequenceSubsystemInstance)
	{
		return;
	}

	FAbilitySequenceNodeRequest DeathRequest;
	DeathRequest.DebugName = TEXT("Death");
	if (!SequenceSubsystemInstance->StartSequence(
		DeathRequest,
		DeathSequenceId,
		DeathSequenceRootNodeId))
	{
		return;
	}

	DeathSequencePhase = EBattleDeathSequencePhase::BeforeEvent;
	ContinueDeathSequence();
}

void UBattleAbilitySystem::ContinueDeathSequence()
{
	if (DeathSequencePhase == EBattleDeathSequencePhase::BeforeEvent)
	{
		DeathSequencePhase = EBattleDeathSequencePhase::AfterEvent;
		if (BroadcastDeathEvent(AutoBattleGameplayTags::Battle_Event_Death_Before))
		{
			return;
		}
	}

	if (DeathSequencePhase == EBattleDeathSequencePhase::AfterEvent)
	{
		DeathSequencePhase = EBattleDeathSequencePhase::Complete;
		if (BroadcastDeathEvent(AutoBattleGameplayTags::Battle_Event_Death_After))
		{
			return;
		}
	}

	if (DeathSequencePhase == EBattleDeathSequencePhase::Complete)
	{
		CompleteDeathSequence();
	}
}

bool UBattleAbilitySystem::BroadcastDeathEvent(const FGameplayTag EventTag)
{
	AUnitActor* UnitActor = Cast<AUnitActor>(GetOwner());
	UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystemInstance = GlobalEventSubsystem.Get();
	if (!IsValid(UnitActor) || !GlobalEventSubsystemInstance)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = UnitActor;
	EventData.Target = UnitActor;

	FGlobalGameplayEventDispatch Dispatch;
	return GlobalEventSubsystemInstance->BroadcastGlobalEvent(
			DeathSequenceId,
			DeathSequenceRootNodeId,
			EventTag,
			EventData,
			Dispatch)
		&& !Dispatch.SequenceTransition.AddedNodeIds.IsEmpty();
}

void UBattleAbilitySystem::CompleteDeathSequence()
{
	AUnitActor* UnitActor = Cast<AUnitActor>(GetOwner());
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	const FAbilitySequenceId CompletedSequenceId = DeathSequenceId;
	const FAbilitySequenceNodeId CompletedRootNodeId = DeathSequenceRootNodeId;

	bDeathResolved = true;
	DeathSequenceId = FAbilitySequenceId();
	DeathSequenceRootNodeId = FAbilitySequenceNodeId();
	DeathSequencePhase = EBattleDeathSequencePhase::None;

	if (SequenceSubsystemInstance)
	{
		FAbilitySequenceTransition IgnoredTransition;
		SequenceSubsystemInstance->FinishNode(
			CompletedSequenceId,
			CompletedRootNodeId,
			EAbilitySequenceNodeResult::Succeeded,
			IgnoredTransition);
	}

	if (IsValid(UnitActor))
	{
		UnitActor->StartDeathAnimation();
	}
}

bool UBattleAbilitySystem::CreateRootSequenceActivation(
	const UBattleGameplayAbility& Ability,
	FBattleAbilitySequenceActivation& OutActivation) const
{
	OutActivation = FBattleAbilitySequenceActivation();

	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (!SequenceSubsystemInstance)
	{
		return false;
	}

	FAbilitySequenceNodeRequest RootRequest;
	RootRequest.DebugName = Ability.GetClass()->GetFName();
	return SequenceSubsystemInstance->StartSequence(
		RootRequest,
		OutActivation.SequenceId,
		OutActivation.NodeId);
}

void UBattleAbilitySystem::FinishSequenceActivation(
	const FBattleAbilitySequenceActivation& Activation,
	const EAbilitySequenceNodeResult Result) const
{
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (!SequenceSubsystemInstance || !Activation.SequenceId.IsValid() || !Activation.NodeId.IsValid())
	{
		return;
	}

	FAbilitySequenceTransition IgnoredTransition;
	SequenceSubsystemInstance->FinishNode(
		Activation.SequenceId,
		Activation.NodeId,
		Result,
		IgnoredTransition);
}
