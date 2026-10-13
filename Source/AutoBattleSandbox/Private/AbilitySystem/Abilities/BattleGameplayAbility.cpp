#include "AbilitySystem/Abilities/BattleGameplayAbility.h"

#include "AbilitySystem/Components/BattleAbilitySystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UBattleGameplayAbility::UBattleGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

const FGlobalGameplayAbilityTrigger* UBattleGameplayAbility::FindGlobalEventTrigger(
	const FGameplayTag EventTag) const
{
	if (!EventTag.IsValid())
	{
		return nullptr;
	}

	const FGlobalGameplayAbilityTrigger* BestTrigger = nullptr;
	for (const FGlobalGameplayAbilityTrigger& Trigger : GlobalEventTriggers)
	{
		if (!Trigger.MatchesEvent(EventTag))
		{
			continue;
		}

		const bool bHasHigherPriority = BestTrigger && Trigger.Priority > BestTrigger->Priority;
		const bool bIsExactTieBreaker = BestTrigger
			&& Trigger.Priority == BestTrigger->Priority
			&& Trigger.EventTag.MatchesTagExact(EventTag)
			&& !BestTrigger->EventTag.MatchesTagExact(EventTag);

		if (!BestTrigger || bHasHigherPriority || bIsExactTieBreaker)
		{
			BestTrigger = &Trigger;
		}
	}

	return BestTrigger;
}

bool UBattleGameplayAbility::TryGetGlobalEventTriggerPriority(
	const FGameplayTag EventTag,
	int32& OutPriority) const
{
	OutPriority = 0;

	const FGlobalGameplayAbilityTrigger* MatchingTrigger = FindGlobalEventTrigger(EventTag);
	if (!MatchingTrigger)
	{
		return false;
	}

	OutPriority = MatchingTrigger->Priority;
	return true;
}

bool UBattleGameplayAbility::BroadcastGlobalGameplayEvent(
	const FGameplayTag EventTag,
	const FGameplayEventData& EventData,
	FGlobalGameplayEventDispatch& OutDispatch) const
{
	OutDispatch = FGlobalGameplayEventDispatch();

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	UBattleAbilitySystem* BattleAbilitySystem = Cast<UBattleAbilitySystem>(ActorInfo->AbilitySystemComponent.Get());
	if (!BattleAbilitySystem)
	{
		return false;
	}

	return BattleAbilitySystem->BroadcastGlobalGameplayEvent(
		GetCurrentAbilitySpecHandle(),
		EventTag,
		EventData,
		OutDispatch);
}

bool UBattleGameplayAbility::GetActivationEventData(FGameplayEventData& OutEventData) const
{
	OutEventData = ActivationEventData;
	return bHasActivationEventData;
}

TArray<FActiveGameplayEffectHandle> UBattleGameplayAbility::ApplyGameplayEffectClassToTargetData(
	const FGameplayAbilityTargetDataHandle& TargetData,
	const TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const float GameplayEffectLevel,
	const int32 Stacks) const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return {};
	}

	return ApplyGameplayEffectToTarget(
		GetCurrentAbilitySpecHandle(),
		ActorInfo,
		GetCurrentActivationInfo(),
		TargetData,
		GameplayEffectClass,
		GameplayEffectLevel,
		Stacks);
}

TArray<FActiveGameplayEffectHandle> UBattleGameplayAbility::ApplyGameplayEffectSpecToTargetData(
	const FGameplayAbilityTargetDataHandle& TargetData,
	const FGameplayEffectSpecHandle& GameplayEffectSpec) const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return {};
	}

	return ApplyGameplayEffectSpecToTarget(
		GetCurrentAbilitySpecHandle(),
		ActorInfo,
		GetCurrentActivationInfo(),
		GameplayEffectSpec,
		TargetData);
}

void UBattleGameplayAbility::SetActivationContext(const FBattleAbilityActivationContext& ActivationContext)
{
	ActivationTargetingInput = ActivationContext.TargetingInput;
	ActivationEventData = ActivationContext.EventData;
	bHasActivationEventData = ActivationContext.bHasEventData;
}

#if WITH_EDITOR
EDataValidationResult UBattleGameplayAbility::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 TriggerIndex = 0; TriggerIndex < GlobalEventTriggers.Num(); ++TriggerIndex)
	{
		const FGlobalGameplayAbilityTrigger& Trigger = GlobalEventTriggers[TriggerIndex];
		if (!Trigger.IsValid())
		{
			Context.AddError(FText::Format(
				INVTEXT("Global event trigger at index {0} must have a valid event tag."),
				TriggerIndex));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		for (int32 OtherTriggerIndex = 0; OtherTriggerIndex < TriggerIndex; ++OtherTriggerIndex)
		{
			const FGlobalGameplayAbilityTrigger& OtherTrigger = GlobalEventTriggers[OtherTriggerIndex];
			if (Trigger.EventTag == OtherTrigger.EventTag && Trigger.MatchPolicy == OtherTrigger.MatchPolicy)
			{
				Context.AddError(FText::Format(
					INVTEXT("Global event trigger at index {0} duplicates trigger {1}."),
					TriggerIndex,
					OtherTriggerIndex));
				Result = EDataValidationResult::Invalid;
				break;
			}
		}
	}

	return Result;
}
#endif
