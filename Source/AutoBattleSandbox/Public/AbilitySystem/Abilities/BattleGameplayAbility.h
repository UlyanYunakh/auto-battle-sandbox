#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "GameplayEffectTypes.h"
#include "ModularGameplayAbility.h"
#include "BattleGameplayAbility.generated.h"

/** Project-level gameplay ability base that participates in global event sequences. */
UCLASS(BlueprintType, Blueprintable)
class AUTOBATTLESANDBOX_API UBattleGameplayAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	UBattleGameplayAbility();

	const TArray<FGlobalGameplayAbilityTrigger>& GetGlobalEventTriggers() const { return GlobalEventTriggers; }
	const FGlobalGameplayAbilityTrigger* FindGlobalEventTrigger(FGameplayTag EventTag) const;

	UFUNCTION(BlueprintPure, Category = "Ability|Global Events")
	bool TryGetGlobalEventTriggerPriority(FGameplayTag EventTag, int32& OutPriority) const;

	UFUNCTION(BlueprintCallable, Category = "Ability|Global Events")
	bool BroadcastGlobalGameplayEvent(
		FGameplayTag EventTag,
		const FGameplayEventData& EventData,
		FGlobalGameplayEventDispatch& OutDispatch) const;

	UFUNCTION(BlueprintPure, Category = "Ability|Targeting")
	const FAbilityTargetSelectionInput& GetActivationTargetingInput() const
	{
		return ActivationTargetingInput;
	}

	UFUNCTION(BlueprintPure, Category = "Ability|Targeting")
	bool GetActivationEventData(FGameplayEventData& OutEventData) const;

	TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectClassToTargetData(
		const FGameplayAbilityTargetDataHandle& TargetData,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		float GameplayEffectLevel,
		int32 Stacks) const;

	TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectSpecToTargetData(
		const FGameplayAbilityTargetDataHandle& TargetData,
		const FGameplayEffectSpecHandle& GameplayEffectSpec) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Events")
	TArray<FGlobalGameplayAbilityTrigger> GlobalEventTriggers;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ability|Targeting")
	FAbilityTargetSelectionInput ActivationTargetingInput;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ability|Targeting")
	FGameplayEventData ActivationEventData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Ability|Targeting")
	bool bHasActivationEventData = false;

private:
	friend class UBattleAbilitySystem;

	void SetActivationContext(const FBattleAbilityActivationContext& ActivationContext);
};
