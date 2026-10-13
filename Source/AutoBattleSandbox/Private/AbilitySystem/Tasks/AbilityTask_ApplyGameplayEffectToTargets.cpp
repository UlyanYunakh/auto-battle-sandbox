#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectToTargets.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "GameplayEffect.h"

UAbilityTask_ApplyGameplayEffectToTargets*
UAbilityTask_ApplyGameplayEffectToTargets::ApplyGameplayEffectToTargets(
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

	UAbilityTask_ApplyGameplayEffectToTargets* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectToTargets>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectClass = GameplayEffectClass;
	Task->GameplayEffectLevel = GameplayEffectLevel;
	Task->Stacks = Stacks;
	return Task;
}

UAbilityTask_ApplyGameplayEffectToTargets*
UAbilityTask_ApplyGameplayEffectToTargets::ApplyGameplayEffectSpecToTargets(
	UBattleGameplayAbility* OwningAbility,
	FGameplayAbilityTargetDataHandle TargetData,
	FGameplayEffectSpecHandle GameplayEffectSpec)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_ApplyGameplayEffectToTargets* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectToTargets>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectSpec = MoveTemp(GameplayEffectSpec);
	Task->bUseGameplayEffectSpec = true;
	return Task;
}

UScriptStruct* UAbilityTask_ApplyGameplayEffectToTargets::GetTaskParametersStruct() const
{
	return FApplyGameplayEffectToTargetsTaskParameters::StaticStruct();
}

bool UAbilityTask_ApplyGameplayEffectToTargets::InitializeFromTaskParameters(const FInstancedStruct& Parameters)
{
	const FApplyGameplayEffectToTargetsTaskParameters* TaskParameters =
		Parameters.GetPtr<FApplyGameplayEffectToTargetsTaskParameters>();
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
	return GameplayEffectClass != nullptr && GameplayEffectLevel >= 0.0f && Stacks > 0;
}

bool UAbilityTask_ApplyGameplayEffectToTargets::ActivateModularAbilityTask()
{
	if (bFinished)
	{
		return true;
	}

	if (!Ability)
	{
		FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidAbility);
		return true;
	}

	if (!TargetData.IsValid(0))
	{
		FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidTargetData);
		return true;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidActorInfo);
		return true;
	}

	UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidAbility);
		return true;
	}

	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;
	if (bUseGameplayEffectSpec)
	{
		if (!GameplayEffectSpec.IsValid())
		{
			FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidGameplayEffect);
			return true;
		}

		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectSpecToTargetData(
			TargetData,
			GameplayEffectSpec);
	}
	else
	{
		if (!GameplayEffectClass || GameplayEffectClass->HasAnyClassFlags(CLASS_Abstract)
			|| GameplayEffectLevel < 0.0f || Stacks <= 0)
		{
			FinishFailed(EApplyGameplayEffectToTargetsFailure::InvalidGameplayEffect);
			return true;
		}

		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectClassToTargetData(
			TargetData,
			GameplayEffectClass,
			GameplayEffectLevel,
			Stacks);
	}

	if (ActiveEffectHandles.IsEmpty())
	{
		FinishFailed(EApplyGameplayEffectToTargetsFailure::ApplicationRejected);
		return true;
	}

	FinishApplied(ActiveEffectHandles);
	return true;
}

void UAbilityTask_ApplyGameplayEffectToTargets::FinishApplied(
	const TArray<FActiveGameplayEffectHandle>& ActiveEffectHandles)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnApplied.Broadcast(TargetData, ActiveEffectHandles);
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

void UAbilityTask_ApplyGameplayEffectToTargets::FinishFailed(
	const EApplyGameplayEffectToTargetsFailure Failure)
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
