#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectWithAttackAnimation.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "Actors/UnitActor.h"
#include "GameplayEffect.h"

UAbilityTask_ApplyGameplayEffectWithAttackAnimation*
UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ApplyGameplayEffectWithAttackAnimation(
	UBattleGameplayAbility* OwningAbility,
	FGameplayAbilityTargetDataHandle TargetData,
	const TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const float GameplayEffectLevel,
	const int32 Stacks)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_ApplyGameplayEffectWithAttackAnimation* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectWithAttackAnimation>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectClass = GameplayEffectClass;
	Task->GameplayEffectLevel = GameplayEffectLevel;
	Task->Stacks = Stacks;
	return Task;
}

UAbilityTask_ApplyGameplayEffectWithAttackAnimation*
UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ApplyGameplayEffectSpecWithAttackAnimation(
	UBattleGameplayAbility* OwningAbility,
	FGameplayAbilityTargetDataHandle TargetData,
	FGameplayEffectSpecHandle GameplayEffectSpec)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_ApplyGameplayEffectWithAttackAnimation* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectWithAttackAnimation>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectSpec = MoveTemp(GameplayEffectSpec);
	Task->bUseGameplayEffectSpec = true;
	return Task;
}

UScriptStruct* UAbilityTask_ApplyGameplayEffectWithAttackAnimation::GetTaskParametersStruct() const
{
	return FApplyGameplayEffectWithAttackAnimationTaskParameters::StaticStruct();
}

bool UAbilityTask_ApplyGameplayEffectWithAttackAnimation::InitializeFromTaskParameters(
	const FInstancedStruct& Parameters)
{
	const FApplyGameplayEffectWithAttackAnimationTaskParameters* TaskParameters =
		Parameters.GetPtr<FApplyGameplayEffectWithAttackAnimationTaskParameters>();
	if (!TaskParameters)
	{
		return false;
	}

	TargetData = CachedAbilityContext.TargetData;
	GameplayEffectClass = TaskParameters->GameplayEffectClass;
	GameplayEffectSpec = FGameplayEffectSpecHandle();
	GameplayEffectLevel = TaskParameters->GameplayEffectLevel;
	Stacks = TaskParameters->Stacks;
	bUseGameplayEffectSpec = false;
	PopulateTargetDataFromActivationInput();
	return GameplayEffectClass != nullptr && GameplayEffectLevel >= 0.0f && Stacks > 0;
}

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::PopulateTargetDataFromActivationInput()
{
	if (TargetData.IsValid(0))
	{
		return;
	}

	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		return;
	}

	const FAbilityTargetSelectionInput& TargetingInput = BattleAbility->GetActivationTargetingInput();
	TargetData = TargetingInput.ProvidedTargetData;
	if (TargetData.IsValid(0))
	{
		return;
	}

	FGameplayAbilityTargetData_ActorArray* ActorData = nullptr;
	for (AActor* TargetActor : TargetingInput.ProvidedActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		if (!ActorData)
		{
			ActorData = new FGameplayAbilityTargetData_ActorArray();
		}
		ActorData->TargetActorArray.AddUnique(TargetActor);
	}

	if (ActorData)
	{
		TargetData.Add(ActorData);
	}
}

bool UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ActivateModularAbilityTask()
{
	if (bFinished)
	{
		return true;
	}

	UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidAbility);
		return true;
	}

	if (!TargetData.IsValid(0))
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidTargetData);
		return true;
	}

	if (bUseGameplayEffectSpec)
	{
		if (!GameplayEffectSpec.IsValid())
		{
			FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidGameplayEffect);
			return true;
		}
	}
	else if (!GameplayEffectClass || GameplayEffectClass->HasAnyClassFlags(CLASS_Abstract)
		|| GameplayEffectLevel < 0.0f || Stacks <= 0)
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidGameplayEffect);
		return true;
	}

	const FGameplayAbilityActorInfo* ActorInfo = BattleAbility->GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidActorInfo);
		return true;
	}

	AUnitActor* UnitActor = Cast<AUnitActor>(ActorInfo->AvatarActor.Get());
	if (!UnitActor)
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidUnitActor);
		return true;
	}

	FVector TargetPosition = FVector::ZeroVector;
	bool bHasTargetPosition = false;
	for (int32 DataIndex = 0; DataIndex < TargetData.Num() && !bHasTargetPosition; ++DataIndex)
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
				TargetPosition = TargetActor->GetActorLocation();
				bHasTargetPosition = true;
				break;
			}
		}
	}

	if (!bHasTargetPosition)
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::InvalidTargetData);
		return true;
	}

	AnimatedUnit = UnitActor;
	if (!UnitActor->StartAttackAnimation(this, TargetPosition))
	{
		AnimatedUnit.Reset();
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::AnimationAlreadyPlaying);
	}
	return true;
}

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::OnDestroy(const bool bAbilityEnding)
{
	if (AUnitActor* UnitActor = AnimatedUnit.Get())
	{
		UnitActor->ClearAttackAnimationTask(this);
	}
	AnimatedUnit.Reset();
	Super::OnDestroy(bAbilityEnding);
}

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ApplyEffectAtAnimationImpact()
{
	if (bFinished || bEffectTriggered)
	{
		return;
	}

	bEffectTriggered = true;
	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		PendingFailure = EApplyGameplayEffectWithAttackAnimationFailure::InvalidAbility;
		return;
	}

	if (bUseGameplayEffectSpec)
	{
		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectSpecToTargetData(TargetData, GameplayEffectSpec);
	}
	else
	{
		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectClassToTargetData(
			TargetData,
			GameplayEffectClass,
			GameplayEffectLevel,
			Stacks);
	}

	if (ActiveEffectHandles.IsEmpty())
	{
		PendingFailure = EApplyGameplayEffectWithAttackAnimationFailure::ApplicationRejected;
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnEffectApplied.Broadcast(TargetData, ActiveEffectHandles);
	}
}

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::CompleteAttackAnimation()
{
	if (bFinished)
	{
		return;
	}

	if (PendingFailure != EApplyGameplayEffectWithAttackAnimationFailure::None)
	{
		FinishFailed(PendingFailure);
		return;
	}

	if (!bEffectTriggered)
	{
		FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure::EffectNotTriggered);
		return;
	}

	CompleteSuccessfully();
}

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::CompleteSuccessfully()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast(TargetData, ActiveEffectHandles);
	}

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

void UAbilityTask_ApplyGameplayEffectWithAttackAnimation::FinishFailed(
	const EApplyGameplayEffectWithAttackAnimationFailure Failure)
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
	CancelModularAbilityTask();
}
