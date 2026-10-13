#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Sequences/AbilitySequenceTypes.h"

/**
 * World-independent state machine for one depth-first ability sequence.
 * It owns ordering and state only; gameplay objects are maintained by its caller.
 */
class AUTOBATTLESANDBOX_API FAbilitySequenceScheduler
{
public:
	bool StartSequence(
		FAbilitySequenceId InSequenceId,
		const FAbilitySequenceNodeRequest& RootRequest,
		FAbilitySequenceNodeId& OutRootNodeId);

	bool PushChildren(
		FAbilitySequenceNodeId ParentNodeId,
		const TArray<FAbilitySequenceNodeRequest>& ChildRequests,
		FAbilitySequenceTransition& OutTransition);

	bool MarkNodeActive(FAbilitySequenceNodeId NodeId);

	bool FinishNode(
		FAbilitySequenceNodeId NodeId,
		EAbilitySequenceNodeResult Result,
		FAbilitySequenceTransition& OutTransition);

	bool CancelSequence(TArray<FAbilitySequenceNodeId>& OutNodesToCancel);
	void Reset();

	FAbilitySequenceId GetSequenceId() const { return SequenceId; }
	FAbilitySequenceNodeId GetRootNodeId() const { return RootNodeId; }
	EAbilitySequenceState GetState() const { return State; }
	const TArray<FAbilitySequenceNode>& GetNodes() const { return Nodes; }
	const TArray<FAbilitySequenceNodeId>& GetActivePath() const { return ActivePath; }
	const FAbilitySequenceNode* FindNode(FAbilitySequenceNodeId NodeId) const;

private:
	FAbilitySequenceNodeId AllocateNodeId();
	FAbilitySequenceNode* FindMutableNode(FAbilitySequenceNodeId NodeId);
	FAbilitySequenceNodeId FindNextQueuedChild(const FAbilitySequenceNode& ParentNode) const;

private:
	FAbilitySequenceId SequenceId;
	FAbilitySequenceNodeId RootNodeId;
	EAbilitySequenceState State = EAbilitySequenceState::Inactive;
	TArray<FAbilitySequenceNode> Nodes;
	TMap<FAbilitySequenceNodeId, int32> NodeIndices;
	TArray<FAbilitySequenceNodeId> ActivePath;
	int64 NextNodeValue = 1;
};
