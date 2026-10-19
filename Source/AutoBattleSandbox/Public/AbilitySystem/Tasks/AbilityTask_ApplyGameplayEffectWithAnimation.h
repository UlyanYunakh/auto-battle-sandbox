#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayEffectTypes.h"
#include "ModularAbilityTask.h"
#include "ModularAbilityTaskDefinition.h"
#include "AbilityTask_ApplyGameplayEffectWithAnimation.generated.h"

class UApplyAnimationProvider;
class UBattleGameplayAbility;
class UGameplayEffect;
enum class EApplyAnimationProviderFailure : uint8;

UENUM(BlueprintType)
enum class EApplyGameplayEffectWithAnimationFailure : uint8
{
	None,
	InvalidAbility,
	InvalidTargetData,
	InvalidGameplayEffect,
	InvalidActorInfo,
	InvalidAnimationProvider,
	AnimationStartFailed,
	AnimationFailed,
	EffectNotTriggered,
	ApplicationRejected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAnimatedGameplayEffectAppliedSignature,
	FGameplayAbilityTargetDataHandle, TargetData,
	const TArray<FActiveGameplayEffectHandle>&, ActiveEffectHandles);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAnimatedGameplayEffectCompletedSignature,
	FGameplayAbilityTargetDataHandle, TargetData,
	const TArray<FActiveGameplayEffectHandle>&, ActiveEffectHandles);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAnimatedGameplayEffectFailedSignature,
	EApplyGameplayEffectWithAnimationFailure, Failure);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyGameplayEffectWithAnimationTaskParameters
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
		meta = (BaseStruct = "/Script/AutoBattleSandbox.ApplyAnimationProviderDefinition",
			ExcludeBaseStruct))
	FInstancedStruct AnimationProvider;
};

/** Applies an effect at a provider-defined impact point and completes when presentation finishes. */
UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTask_ApplyGameplayEffectWithAnimation : public UModularAbilityTask
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
	static UAbilityTask_ApplyGameplayEffectWithAnimation* ApplyGameplayEffectWithAnimation(
		UBattleGameplayAbility* OwningAbility,
		FGameplayAbilityTargetDataHandle TargetData,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		FInstancedStruct AnimationProvider,
		float GameplayEffectLevel = 1.0f,
		int32 Stacks = 1);

	UFUNCTION(
		BlueprintCallable,
		Category = "Ability|Tasks",
		meta = (
			HidePin = "OwningAbility",
			DefaultToSelf = "OwningAbility",
			BlueprintInternalUseOnly = "true"))
	static UAbilityTask_ApplyGameplayEffectWithAnimation* ApplyGameplayEffectSpecWithAnimation(
		UBattleGameplayAbility* OwningAbility,
		FGameplayAbilityTargetDataHandle TargetData,
		FGameplayEffectSpecHandle GameplayEffectSpec,
		FInstancedStruct AnimationProvider);

	virtual UScriptStruct* GetTaskParametersStruct() const override;
	virtual void OnDestroy(bool bAbilityEnding) override;

public:
	UPROPERTY(BlueprintAssignable)
	FAnimatedGameplayEffectAppliedSignature OnEffectApplied;

	UPROPERTY(BlueprintAssignable)
	FAnimatedGameplayEffectCompletedSignature OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FAnimatedGameplayEffectFailedSignature OnFailed;

protected:
	virtual bool ActivateModularAbilityTask() override;
	virtual bool InitializeFromTaskParameters(const FInstancedStruct& Parameters) override;

private:
	void HandleAnimationImpact();
	void HandleAnimationFinished();
	void HandleAnimationFailed(EApplyAnimationProviderFailure Failure);
	void FinishFailed(EApplyGameplayEffectWithAnimationFailure Failure);
	void CompleteSuccessfully();
	void PopulateTargetDataFromActivationInput();
	AActor* FindPrimaryTarget() const;
	void ReleaseAnimationProvider();

private:
	UPROPERTY()
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY()
	FGameplayEffectSpecHandle GameplayEffectSpec;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	UPROPERTY()
	FInstancedStruct AnimationProviderDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UApplyAnimationProvider> ActiveAnimationProvider = nullptr;

	float GameplayEffectLevel = 1.0f;
	int32 Stacks = 1;
	EApplyGameplayEffectWithAnimationFailure PendingFailure =
		EApplyGameplayEffectWithAnimationFailure::None;
	bool bUseGameplayEffectSpec = false;
	bool bEffectTriggered = false;
	bool bFinished = false;
};
