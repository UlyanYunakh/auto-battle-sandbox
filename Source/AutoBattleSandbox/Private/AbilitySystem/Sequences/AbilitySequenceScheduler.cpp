#include "AbilitySystem/Sequences/AbilitySequenceScheduler.h"

bool FAbilitySequenceScheduler::StartSequence(
	const FAbilitySequenceId InSequenceId,
	const FAbilitySequenceNodeRequest& RootRequest,
	FAbilitySequenceNodeId& OutRootNodeId)
{
	OutRootNodeId = FAbilitySequenceNodeId();

	if (!InSequenceId.IsValid() || State != EAbilitySequenceState::Inactive)
	{
		return false;
	}

	FAbilitySequenceNode RootNode;
	RootNode.NodeId = AllocateNodeId();
	RootNode.DebugName = RootRequest.DebugName;
	RootNode.TriggerEvent = RootRequest.TriggerEvent;
	RootNode.State = EAbilitySequenceNodeState::Active;

	if (!RootNode.NodeId.IsValid())
	{
		return false;
	}

	SequenceId = InSequenceId;
	RootNodeId = RootNode.NodeId;
	State = EAbilitySequenceState::Running;
	OutRootNodeId = RootNode.NodeId;

	NodeIndices.Add(RootNode.NodeId, Nodes.Add(MoveTemp(RootNode)));
	ActivePath.Add(RootNodeId);
	return true;
}

bool FAbilitySequenceScheduler::PushChildren(
	const FAbilitySequenceNodeId ParentNodeId,
	const TArray<FAbilitySequenceNodeRequest>& ChildRequests,
	FAbilitySequenceTransition& OutTransition)
{
	OutTransition = FAbilitySequenceTransition();

	if (State != EAbilitySequenceState::Running || ChildRequests.IsEmpty() || ActivePath.IsEmpty()
		|| ActivePath.Last() != ParentNodeId)
	{
		return false;
	}

	FAbilitySequenceNode* ParentNode = FindMutableNode(ParentNodeId);
	if (!ParentNode || ParentNode->State != EAbilitySequenceNodeState::Active)
	{
		return false;
	}

	const int64 LastRequiredNodeValue = NextNodeValue + ChildRequests.Num() - 1;
	if (LastRequiredNodeValue > TNumericLimits<int32>::Max())
	{
		return false;
	}

	const int32 ParentNodeIndex = NodeIndices.FindChecked(ParentNodeId);
	Nodes[ParentNodeIndex].State = EAbilitySequenceNodeState::Suspended;

	for (const FAbilitySequenceNodeRequest& ChildRequest : ChildRequests)
	{
		FAbilitySequenceNode ChildNode;
		ChildNode.NodeId = AllocateNodeId();
		ChildNode.ParentNodeId = ParentNodeId;
		ChildNode.DebugName = ChildRequest.DebugName;
		ChildNode.TriggerEvent = ChildRequest.TriggerEvent;

		const FAbilitySequenceNodeId ChildNodeId = ChildNode.NodeId;
		NodeIndices.Add(ChildNodeId, Nodes.Add(MoveTemp(ChildNode)));
		Nodes[ParentNodeIndex].Children.Add(ChildNodeId);
		OutTransition.AddedNodeIds.Add(ChildNodeId);
	}

	const FAbilitySequenceNodeId FirstChildNodeId = OutTransition.AddedNodeIds[0];
	FAbilitySequenceNode* FirstChildNode = FindMutableNode(FirstChildNodeId);
	FirstChildNode->State = EAbilitySequenceNodeState::Activating;
	ActivePath.Add(FirstChildNodeId);
	OutTransition.NodeToActivate = FirstChildNodeId;
	return true;
}

bool FAbilitySequenceScheduler::MarkNodeActive(const FAbilitySequenceNodeId NodeId)
{
	if (State != EAbilitySequenceState::Running || ActivePath.IsEmpty() || ActivePath.Last() != NodeId)
	{
		return false;
	}

	FAbilitySequenceNode* Node = FindMutableNode(NodeId);
	if (!Node || Node->State != EAbilitySequenceNodeState::Activating)
	{
		return false;
	}

	Node->State = EAbilitySequenceNodeState::Active;
	return true;
}

bool FAbilitySequenceScheduler::FinishNode(
	const FAbilitySequenceNodeId NodeId,
	const EAbilitySequenceNodeResult Result,
	FAbilitySequenceTransition& OutTransition)
{
	OutTransition = FAbilitySequenceTransition();

	if (State != EAbilitySequenceState::Running || Result == EAbilitySequenceNodeResult::None
		|| ActivePath.IsEmpty() || ActivePath.Last() != NodeId)
	{
		return false;
	}

	FAbilitySequenceNode* Node = FindMutableNode(NodeId);
	if (!Node || (Node->State != EAbilitySequenceNodeState::Active
		&& Node->State != EAbilitySequenceNodeState::Activating))
	{
		return false;
	}

	Node->State = EAbilitySequenceNodeState::Finished;
	Node->Result = Result;
	ActivePath.Pop(EAllowShrinking::No);

	if (ActivePath.IsEmpty())
	{
		State = Result == EAbilitySequenceNodeResult::Cancelled
			        ? EAbilitySequenceState::Cancelled
			        : EAbilitySequenceState::Finished;
		OutTransition.bSequenceFinished = true;
		return true;
	}

	FAbilitySequenceNode* ParentNode = FindMutableNode(ActivePath.Last());
	if (!ParentNode || ParentNode->State != EAbilitySequenceNodeState::Suspended)
	{
		return false;
	}

	const FAbilitySequenceNodeId NextChildNodeId = FindNextQueuedChild(*ParentNode);
	if (NextChildNodeId.IsValid())
	{
		FAbilitySequenceNode* NextChildNode = FindMutableNode(NextChildNodeId);
		NextChildNode->State = EAbilitySequenceNodeState::Activating;
		ActivePath.Add(NextChildNodeId);
		OutTransition.NodeToActivate = NextChildNodeId;
		return true;
	}

	ParentNode->State = EAbilitySequenceNodeState::Active;
	OutTransition.NodeToResume = ParentNode->NodeId;
	return true;
}

bool FAbilitySequenceScheduler::CancelSequence(TArray<FAbilitySequenceNodeId>& OutNodesToCancel)
{
	OutNodesToCancel.Reset();

	if (State != EAbilitySequenceState::Running)
	{
		return false;
	}

	for (int32 PathIndex = ActivePath.Num() - 1; PathIndex >= 0; --PathIndex)
	{
		OutNodesToCancel.Add(ActivePath[PathIndex]);
	}

	for (FAbilitySequenceNode& Node : Nodes)
	{
		if (Node.State != EAbilitySequenceNodeState::Finished)
		{
			Node.State = EAbilitySequenceNodeState::Finished;
			Node.Result = EAbilitySequenceNodeResult::Cancelled;
		}
	}

	ActivePath.Reset();
	State = EAbilitySequenceState::Cancelled;
	return true;
}

void FAbilitySequenceScheduler::Reset()
{
	SequenceId = FAbilitySequenceId();
	RootNodeId = FAbilitySequenceNodeId();
	State = EAbilitySequenceState::Inactive;
	Nodes.Reset();
	NodeIndices.Reset();
	ActivePath.Reset();
	NextNodeValue = 1;
}

const FAbilitySequenceNode* FAbilitySequenceScheduler::FindNode(const FAbilitySequenceNodeId NodeId) const
{
	const int32* NodeIndex = NodeIndices.Find(NodeId);
	return NodeIndex ? &Nodes[*NodeIndex] : nullptr;
}

FAbilitySequenceNodeId FAbilitySequenceScheduler::AllocateNodeId()
{
	if (NextNodeValue <= 0)
	{
		return FAbilitySequenceNodeId();
	}

	if (NextNodeValue > TNumericLimits<int32>::Max())
	{
		return FAbilitySequenceNodeId();
	}

	return FAbilitySequenceNodeId(static_cast<int32>(NextNodeValue++));
}

FAbilitySequenceNode* FAbilitySequenceScheduler::FindMutableNode(const FAbilitySequenceNodeId NodeId)
{
	const int32* NodeIndex = NodeIndices.Find(NodeId);
	return NodeIndex ? &Nodes[*NodeIndex] : nullptr;
}

FAbilitySequenceNodeId FAbilitySequenceScheduler::FindNextQueuedChild(
	const FAbilitySequenceNode& ParentNode) const
{
	for (const FAbilitySequenceNodeId ChildNodeId : ParentNode.Children)
	{
		const FAbilitySequenceNode* ChildNode = FindNode(ChildNodeId);
		if (ChildNode && ChildNode->State == EAbilitySequenceNodeState::Queued)
		{
			return ChildNodeId;
		}
	}

	return FAbilitySequenceNodeId();
}
