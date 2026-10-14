#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayEffectTypes.h"
#include "ModularAbilityTask.h"
#include "ModularAbilityTaskDefinition.h"
#include "AbilityTask_ApplyGameplayEffectWithAttackAnimation.generated.h"

class AUnitActor;
class UBattleGameplayAbility;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EApplyGameplayEffectWithAttackAnimationFailure : uint8
{
	None,
	InvalidAbility,
	InvalidTargetData,
	InvalidGameplayEffect,
	InvalidActorInfo,
	InvalidUnitActor,
	AnimationAlreadyPlaying,
	EffectNotTriggered,
	ApplicationRejected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAttackAnimationEffectAppliedSignature,
	FGameplayAbilityTargetDataHandle, TargetData,
	const TArray<FActiveGameplayEffectHandle>&, ActiveEffectHandles);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAttackAnimationCompletedSignature,
	FGameplayAbilityTargetDataHandle, TargetData,
	const TArray<FActiveGameplayEffectHandle>&, ActiveEffectHandles);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAttackAnimationEffectFailedSignature,
	EApplyGameplayEffectWithAttackAnimationFailure, Failure);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyGameplayEffectWithAttackAnimationTaskParameters
	: public FModularAbilityTaskParameters
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

/** Plays the owning unit's Blueprint attack animation and applies an effect when Blueprint signals impact. */
UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTask_ApplyGameplayEffectWithAttackAnimation : public UModularAbilityTask
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
	static UAbilityTask_ApplyGameplayEffectWithAttackAnimation* ApplyGameplayEffectWithAttackAnimation(
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
	static UAbilityTask_ApplyGameplayEffectWithAttackAnimation* ApplyGameplayEffectSpecWithAttackAnimation(
		UBattleGameplayAbility* OwningAbility,
		FGameplayAbilityTargetDataHandle TargetData,
		FGameplayEffectSpecHandle GameplayEffectSpec);

	virtual UScriptStruct* GetTaskParametersStruct() const override;
	virtual void OnDestroy(bool bAbilityEnding) override;

public:
	/** Fired at the Blueprint-selected impact point, while the animation is still running. */
	UPROPERTY(BlueprintAssignable)
	FAttackAnimationEffectAppliedSignature OnEffectApplied;

	/** Fired only after Blueprint reports that the attack animation has finished. */
	UPROPERTY(BlueprintAssignable)
	FAttackAnimationCompletedSignature OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FAttackAnimationEffectFailedSignature OnFailed;

protected:
	virtual bool ActivateModularAbilityTask() override;
	virtual bool InitializeFromTaskParameters(const FInstancedStruct& Parameters) override;

private:
	friend class AUnitActor;

	void ApplyEffectAtAnimationImpact();
	void CompleteAttackAnimation();
	void FinishFailed(EApplyGameplayEffectWithAttackAnimationFailure Failure);
	void CompleteSuccessfully();
	void PopulateTargetDataFromActivationInput();

private:
	UPROPERTY()
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY()
	FGameplayEffectSpecHandle GameplayEffectSpec;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	TWeakObjectPtr<AUnitActor> AnimatedUnit;
	float GameplayEffectLevel = 1.0f;
	int32 Stacks = 1;
	EApplyGameplayEffectWithAttackAnimationFailure PendingFailure =
		EApplyGameplayEffectWithAttackAnimationFailure::None;
	bool bUseGameplayEffectSpec = false;
	bool bEffectTriggered = false;
	bool bFinished = false;
};
