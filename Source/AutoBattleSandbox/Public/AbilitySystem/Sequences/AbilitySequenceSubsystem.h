#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Sequences/AbilitySequenceScheduler.h"
#include "Subsystems/WorldSubsystem.h"
#include "AbilitySequenceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAbilitySequenceStartedSignature,
	FAbilitySequenceId, SequenceId,
	FAbilitySequenceNode, RootNode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAbilitySequenceNodeAddedSignature,
	FAbilitySequenceId, SequenceId,
	FAbilitySequenceNode, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAbilitySequenceNodeStateChangedSignature,
	FAbilitySequenceId, SequenceId,
	FAbilitySequenceNode, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAbilitySequenceFinishedSignature,
	FAbilitySequenceId, SequenceId,
	EAbilitySequenceState, FinalState);

/** World-scoped owner and observable facade for ability sequence schedulers. */
UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAutoBattleAbilitySequenceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	bool StartSequence(
		const FAbilitySequenceNodeRequest& RootRequest,
		FAbilitySequenceId& OutSequenceId,
		FAbilitySequenceNodeId& OutRootNodeId);

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	bool PushChildren(
		FAbilitySequenceId SequenceId,
		FAbilitySequenceNodeId ParentNodeId,
		const TArray<FAbilitySequenceNodeRequest>& ChildRequests,
		FAbilitySequenceTransition& OutTransition);

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	bool MarkNodeActive(FAbilitySequenceId SequenceId, FAbilitySequenceNodeId NodeId);

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	bool FinishNode(
		FAbilitySequenceId SequenceId,
		FAbilitySequenceNodeId NodeId,
		EAbilitySequenceNodeResult Result,
		FAbilitySequenceTransition& OutTransition);

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	bool CancelSequence(FAbilitySequenceId SequenceId, TArray<FAbilitySequenceNodeId>& OutNodesToCancel);

	UFUNCTION(BlueprintCallable, Category = "Ability Sequence")
	void ClearFinishedSequences();

	UFUNCTION(BlueprintPure, Category = "Ability Sequence")
	bool GetSequenceSnapshot(FAbilitySequenceId SequenceId, FAbilitySequenceSnapshot& OutSnapshot) const;

	UFUNCTION(BlueprintPure, Category = "Ability Sequence")
	TArray<FAbilitySequenceSnapshot> GetAllSequenceSnapshots() const;

	UFUNCTION(BlueprintPure, Category = "Ability Sequence")
	bool IsSequenceRunning(FAbilitySequenceId SequenceId) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "Ability Sequence")
	FAbilitySequenceStartedSignature OnSequenceStarted;

	UPROPERTY(BlueprintAssignable, Category = "Ability Sequence")
	FAbilitySequenceNodeAddedSignature OnSequenceNodeAdded;

	UPROPERTY(BlueprintAssignable, Category = "Ability Sequence")
	FAbilitySequenceNodeStateChangedSignature OnSequenceNodeStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ability Sequence")
	FAbilitySequenceFinishedSignature OnSequenceFinished;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	FAbilitySequenceId AllocateSequenceId();
	FAbilitySequenceScheduler* FindScheduler(FAbilitySequenceId SequenceId);
	const FAbilitySequenceScheduler* FindScheduler(FAbilitySequenceId SequenceId) const;
	void BroadcastNodeState(FAbilitySequenceId SequenceId, FAbilitySequenceNodeId NodeId);
	void RetainFinishedSequence(FAbilitySequenceId SequenceId);

private:
	static constexpr int32 MaxRetainedFinishedSequences = 32;

	TMap<FAbilitySequenceId, FAbilitySequenceScheduler> SequenceSchedulers;
	TArray<FAbilitySequenceId> FinishedSequenceIds;
	int64 NextSequenceValue = 1;
};
