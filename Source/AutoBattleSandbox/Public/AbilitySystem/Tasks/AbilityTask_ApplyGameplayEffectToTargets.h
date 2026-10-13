#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayEffectTypes.h"
#include "ModularAbilityTask.h"
#include "ModularAbilityTaskDefinition.h"
#include "AbilityTask_ApplyGameplayEffectToTargets.generated.h"

class UGameplayEffect;
class UBattleGameplayAbility;

UENUM(BlueprintType)
enum class EApplyGameplayEffectToTargetsFailure : uint8
{
	None,
	InvalidAbility,
	InvalidTargetData,
	InvalidGameplayEffect,
	InvalidActorInfo,
	ApplicationRejected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FGameplayEffectAppliedToTargetsSignature,
	FGameplayAbilityTargetDataHandle, TargetData,
	const TArray<FActiveGameplayEffectHandle>&, ActiveEffectHandles);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FGameplayEffectApplicationToTargetsFailedSignature,
	EApplyGameplayEffectToTargetsFailure, Failure);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyGameplayEffectToTargetsTaskParameters : public FModularAbilityTaskParameters
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effect", meta = (ClampMin = "0.0"))
	float GameplayEffectLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Effect", meta = (ClampMin = "1"))
	int32 Stacks = 1;
};

/** Applies one gameplay effect class or prebuilt spec to every target represented by target data. */
UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTask_ApplyGameplayEffectToTargets : public UModularAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Ability|Tasks",
		meta = (
			HidePin = "OwningAbility",
			DefaultToSelf = "OwningAbility",
			BlueprintInternalUseOnly = "true",
			AdvancedDisplay = "GameplayEffectLevel,Stacks"))
	static UAbilityTask_ApplyGameplayEffectToTargets* ApplyGameplayEffectToTargets(
		UBattleGameplayAbility* OwningAbility,
		FGameplayAbilityTargetDataHandle TargetData,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		float GameplayEffectLevel = 1.0f,
		int32 Stacks = 1);

	UFUNCTION(
		BlueprintCallable,
		Category = "Ability|Tasks",
		meta = (
			HidePin = "OwningAbility",
			DefaultToSelf = "OwningAbility",
			BlueprintInternalUseOnly = "true"))
	static UAbilityTask_ApplyGameplayEffectToTargets* ApplyGameplayEffectSpecToTargets(
		UBattleGameplayAbility* OwningAbility,
		FGameplayAbilityTargetDataHandle TargetData,
		FGameplayEffectSpecHandle GameplayEffectSpec);

	virtual UScriptStruct* GetTaskParametersStruct() const override;

public:
	UPROPERTY(BlueprintAssignable)
	FGameplayEffectAppliedToTargetsSignature OnApplied;

	UPROPERTY(BlueprintAssignable)
	FGameplayEffectApplicationToTargetsFailedSignature OnFailed;

protected:
	virtual bool ActivateModularAbilityTask() override;
	virtual bool InitializeFromTaskParameters(const FInstancedStruct& Parameters) override;

private:
	void FinishApplied(const TArray<FActiveGameplayEffectHandle>& ActiveEffectHandles);
	void FinishFailed(EApplyGameplayEffectToTargetsFailure Failure);

private:
	UPROPERTY()
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY()
	FGameplayEffectSpecHandle GameplayEffectSpec;

	float GameplayEffectLevel = 1.0f;
	int32 Stacks = 1;
	bool bUseGameplayEffectSpec = false;
	bool bFinished = false;
};
