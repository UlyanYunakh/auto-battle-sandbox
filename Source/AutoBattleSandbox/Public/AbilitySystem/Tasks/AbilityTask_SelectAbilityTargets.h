#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Sequences/AbilitySequenceTypes.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "AbilitySystem/Targeting/BattleTargetSelectionComponent.h"
#include "ModularAbilityTask.h"
#include "ModularAbilityTaskDefinition.h"
#include "AbilityTask_SelectAbilityTargets.generated.h"

class UAbilityTargetingDefinition;
class UAbilityTargetingSubsystem;
class UAutoBattleAbilitySequenceSubsystem;
class UBattleGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAbilityTargetsSelectedSignature,
	FGameplayAbilityTargetDataHandle, TargetData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAbilityTargetSelectionFailedSignature,
	EAbilityTargetSelectionFailure, Failure);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityTargetSelectionCancelledSignature);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FSelectAbilityTargetsTaskParameters : public FModularAbilityTaskParameters
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	TObjectPtr<UAbilityTargetingDefinition> TargetingDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	EAbilityTargetInputPolicy InputPolicy = EAbilityTargetInputPolicy::NormalDefinition;
};

UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTask_SelectAbilityTargets : public UModularAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_SelectAbilityTargets* SelectAbilityTargets(
		UBattleGameplayAbility* OwningAbility,
		UAbilityTargetingDefinition* TargetingDefinition,
		FAbilityTargetSelectionInput SelectionInput);

	virtual UScriptStruct* GetTaskParametersStruct() const override;
	virtual void OnDestroy(bool bAbilityEnding) override;

public:
	UPROPERTY(BlueprintAssignable)
	FAbilityTargetsSelectedSignature OnSelected;

	UPROPERTY(BlueprintAssignable)
	FAbilityTargetSelectionCancelledSignature OnCancelled;

	UPROPERTY(BlueprintAssignable)
	FAbilityTargetSelectionFailedSignature OnFailed;

protected:
	virtual bool ActivateModularAbilityTask() override;
	virtual bool InitializeFromTaskParameters(const FInstancedStruct& Parameters) override;

private:
	void RunDefinitionSelection();
	void RunAutomaticSelection();
	bool TryBeginManualSelection();
	void HandleManualSelectionCompleted(const FGameplayAbilityTargetDataHandle& TargetData);
	void HandleManualSelectionCancelled();
	void Complete(const FGameplayAbilityTargetDataHandle& TargetData);
	void Fail(EAbilityTargetSelectionFailure Failure);
	void Cancel();
	void BindSequenceDelegates();
	void UnbindSequenceDelegates();

	UFUNCTION()
	void HandleSequenceNodeStateChanged(FAbilitySequenceId SequenceId, FAbilitySequenceNode Node);

	UFUNCTION()
	void HandleSequenceFinished(FAbilitySequenceId SequenceId, EAbilitySequenceState FinalState);

private:
	UPROPERTY()
	TObjectPtr<UAbilityTargetingDefinition> TargetingDefinition = nullptr;

	UPROPERTY()
	FAbilityTargetSelectionInput SelectionInput;

	UPROPERTY()
	FAbilityTargetingContext TargetingContext;

	TWeakObjectPtr<UAbilityTargetingSubsystem> TargetingSubsystem;
	TWeakObjectPtr<UBattleTargetSelectionComponent> ManualSelectionComponent;
	TWeakObjectPtr<UAutoBattleAbilitySequenceSubsystem> SequenceSubsystem;
	FAbilityTargetSelectionRequestHandle ManualRequestHandle;
	FAbilitySequenceId SequenceId;
	FAbilitySequenceNodeId SequenceNodeId;
	bool bFinished = false;
};
