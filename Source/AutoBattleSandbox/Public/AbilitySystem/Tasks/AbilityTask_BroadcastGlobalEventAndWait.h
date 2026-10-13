#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"
#include "ModularAbilityTask.h"
#include "ModularAbilityTaskDefinition.h"
#include "AbilityTask_BroadcastGlobalEventAndWait.generated.h"

class UAutoBattleAbilitySequenceSubsystem;
class UBattleGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FBroadcastGlobalEventWaitDelegate,
	FGlobalGameplayEventDispatch, Dispatch);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FBroadcastGlobalEventAndWaitTaskParameters : public FModularAbilityTaskParameters
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Event")
	FGameplayTag EventTag;
};

/** Broadcasts a global gameplay event and waits until the broadcasting sequence node resumes. */
UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTask_BroadcastGlobalEventAndWait : public UModularAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_BroadcastGlobalEventAndWait* BroadcastGlobalEventAndWait(
		UBattleGameplayAbility* OwningAbility,
		FGameplayTag EventTag,
		FGameplayEventData EventData);

	virtual UScriptStruct* GetTaskParametersStruct() const override;
	virtual void OnDestroy(bool bAbilityEnding) override;

public:
	UPROPERTY(BlueprintAssignable)
	FBroadcastGlobalEventWaitDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FBroadcastGlobalEventWaitDelegate OnFailed;

protected:
	virtual bool ActivateModularAbilityTask() override;
	virtual bool InitializeFromTaskParameters(const FInstancedStruct& Parameters) override;

private:
	UFUNCTION()
	void HandleSequenceNodeStateChanged(FAbilitySequenceId SequenceId, FAbilitySequenceNode Node);

	UFUNCTION()
	void HandleSequenceFinished(FAbilitySequenceId SequenceId, EAbilitySequenceState FinalState);

	void BindSequenceDelegates(UAutoBattleAbilitySequenceSubsystem& InSequenceSubsystem);
	void UnbindSequenceDelegates();
	void EvaluateCurrentSequenceState();
	void FinishTask(bool bSucceeded);

private:
	UPROPERTY()
	FGameplayTag EventTag;

	UPROPERTY()
	FGameplayEventData EventData;

	UPROPERTY()
	FGlobalGameplayEventDispatch Dispatch;

	TWeakObjectPtr<UAutoBattleAbilitySequenceSubsystem> SequenceSubsystem;
	FAbilitySequenceId WaitingSequenceId;
	FAbilitySequenceNodeId WaitingSourceNodeId;
	bool bFinished = false;
};
