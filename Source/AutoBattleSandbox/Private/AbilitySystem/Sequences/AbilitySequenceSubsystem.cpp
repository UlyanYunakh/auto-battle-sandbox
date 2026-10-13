#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"

void UAutoBattleAbilitySequenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SequenceSchedulers.Reset();
	FinishedSequenceIds.Reset();
	NextSequenceValue = 1;
}

void UAutoBattleAbilitySequenceSubsystem::Deinitialize()
{
	SequenceSchedulers.Reset();
	FinishedSequenceIds.Reset();
	NextSequenceValue = 1;

	OnSequenceStarted.Clear();
	OnSequenceNodeAdded.Clear();
	OnSequenceNodeStateChanged.Clear();
	OnSequenceFinished.Clear();

	Super::Deinitialize();
}

bool UAutoBattleAbilitySequenceSubsystem::StartSequence(
	const FAbilitySequenceNodeRequest& RootRequest,
	FAbilitySequenceId& OutSequenceId,
	FAbilitySequenceNodeId& OutRootNodeId)
{
	OutSequenceId = FAbilitySequenceId();
	OutRootNodeId = FAbilitySequenceNodeId();

	const FAbilitySequenceId NewSequenceId = AllocateSequenceId();
	if (!NewSequenceId.IsValid())
	{
		return false;
	}

	FAbilitySequenceScheduler NewScheduler;
	if (!NewScheduler.StartSequence(NewSequenceId, RootRequest, OutRootNodeId))
	{
		return false;
	}

	FAbilitySequenceScheduler& StoredScheduler = SequenceSchedulers.Add(NewSequenceId, MoveTemp(NewScheduler));
	const FAbilitySequenceNode* RootNode = StoredScheduler.FindNode(OutRootNodeId);
	if (!RootNode)
	{
		SequenceSchedulers.Remove(NewSequenceId);
		OutRootNodeId = FAbilitySequenceNodeId();
		return false;
	}

	OutSequenceId = NewSequenceId;
	OnSequenceStarted.Broadcast(NewSequenceId, *RootNode);
	OnSequenceNodeAdded.Broadcast(NewSequenceId, *RootNode);
	return true;
}

bool UAutoBattleAbilitySequenceSubsystem::PushChildren(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId ParentNodeId,
	const TArray<FAbilitySequenceNodeRequest>& ChildRequests,
	FAbilitySequenceTransition& OutTransition)
{
	OutTransition = FAbilitySequenceTransition();

	FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	if (!Scheduler || !Scheduler->PushChildren(ParentNodeId, ChildRequests, OutTransition))
	{
		return false;
	}

	for (const FAbilitySequenceNodeId AddedNodeId : OutTransition.AddedNodeIds)
	{
		if (const FAbilitySequenceNode* AddedNode = Scheduler->FindNode(AddedNodeId))
		{
			OnSequenceNodeAdded.Broadcast(SequenceId, *AddedNode);
		}
	}

	BroadcastNodeState(SequenceId, ParentNodeId);
	BroadcastNodeState(SequenceId, OutTransition.NodeToActivate);
	return true;
}

bool UAutoBattleAbilitySequenceSubsystem::MarkNodeActive(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId)
{
	FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	if (!Scheduler || !Scheduler->MarkNodeActive(NodeId))
	{
		return false;
	}

	BroadcastNodeState(SequenceId, NodeId);
	return true;
}

bool UAutoBattleAbilitySequenceSubsystem::FinishNode(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId,
	const EAbilitySequenceNodeResult Result,
	FAbilitySequenceTransition& OutTransition)
{
	OutTransition = FAbilitySequenceTransition();

	FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	if (!Scheduler || !Scheduler->FinishNode(NodeId, Result, OutTransition))
	{
		return false;
	}

	BroadcastNodeState(SequenceId, NodeId);
	BroadcastNodeState(SequenceId, OutTransition.NodeToActivate);
	BroadcastNodeState(SequenceId, OutTransition.NodeToResume);

	if (OutTransition.bSequenceFinished)
	{
		OnSequenceFinished.Broadcast(SequenceId, Scheduler->GetState());
		RetainFinishedSequence(SequenceId);
	}

	return true;
}

bool UAutoBattleAbilitySequenceSubsystem::CancelSequence(
	const FAbilitySequenceId SequenceId,
	TArray<FAbilitySequenceNodeId>& OutNodesToCancel)
{
	OutNodesToCancel.Reset();

	FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	if (!Scheduler)
	{
		return false;
	}

	TArray<FAbilitySequenceNodeId> ChangedNodeIds;
	for (const FAbilitySequenceNode& Node : Scheduler->GetNodes())
	{
		if (Node.State != EAbilitySequenceNodeState::Finished)
		{
			ChangedNodeIds.Add(Node.NodeId);
		}
	}

	if (!Scheduler->CancelSequence(OutNodesToCancel))
	{
		return false;
	}

	for (const FAbilitySequenceNodeId ChangedNodeId : ChangedNodeIds)
	{
		BroadcastNodeState(SequenceId, ChangedNodeId);
	}

	OnSequenceFinished.Broadcast(SequenceId, EAbilitySequenceState::Cancelled);
	RetainFinishedSequence(SequenceId);
	return true;
}

void UAutoBattleAbilitySequenceSubsystem::ClearFinishedSequences()
{
	for (const FAbilitySequenceId FinishedSequenceId : FinishedSequenceIds)
	{
		SequenceSchedulers.Remove(FinishedSequenceId);
	}

	FinishedSequenceIds.Reset();
}

bool UAutoBattleAbilitySequenceSubsystem::GetSequenceSnapshot(
	const FAbilitySequenceId SequenceId,
	FAbilitySequenceSnapshot& OutSnapshot) const
{
	OutSnapshot = FAbilitySequenceSnapshot();

	const FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	if (!Scheduler)
	{
		return false;
	}

	OutSnapshot.SequenceId = Scheduler->GetSequenceId();
	OutSnapshot.RootNodeId = Scheduler->GetRootNodeId();
	OutSnapshot.State = Scheduler->GetState();
	OutSnapshot.Nodes = Scheduler->GetNodes();
	OutSnapshot.ActivePath = Scheduler->GetActivePath();
	return true;
}

TArray<FAbilitySequenceSnapshot> UAutoBattleAbilitySequenceSubsystem::GetAllSequenceSnapshots() const
{
	TArray<FAbilitySequenceSnapshot> Snapshots;
	Snapshots.Reserve(SequenceSchedulers.Num());

	for (const TPair<FAbilitySequenceId, FAbilitySequenceScheduler>& SequencePair : SequenceSchedulers)
	{
		FAbilitySequenceSnapshot& Snapshot = Snapshots.AddDefaulted_GetRef();
		Snapshot.SequenceId = SequencePair.Value.GetSequenceId();
		Snapshot.RootNodeId = SequencePair.Value.GetRootNodeId();
		Snapshot.State = SequencePair.Value.GetState();
		Snapshot.Nodes = SequencePair.Value.GetNodes();
		Snapshot.ActivePath = SequencePair.Value.GetActivePath();
	}

	Snapshots.Sort([](const FAbilitySequenceSnapshot& Left, const FAbilitySequenceSnapshot& Right)
	{
		return Left.SequenceId.Value < Right.SequenceId.Value;
	});
	return Snapshots;
}

bool UAutoBattleAbilitySequenceSubsystem::IsSequenceRunning(const FAbilitySequenceId SequenceId) const
{
	const FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	return Scheduler && Scheduler->GetState() == EAbilitySequenceState::Running;
}

bool UAutoBattleAbilitySequenceSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

FAbilitySequenceId UAutoBattleAbilitySequenceSubsystem::AllocateSequenceId()
{
	if (NextSequenceValue <= 0)
	{
		return FAbilitySequenceId();
	}

	const FAbilitySequenceId AllocatedId(NextSequenceValue);
	NextSequenceValue = NextSequenceValue == TNumericLimits<int64>::Max() ? 0 : NextSequenceValue + 1;
	return AllocatedId;
}

FAbilitySequenceScheduler* UAutoBattleAbilitySequenceSubsystem::FindScheduler(
	const FAbilitySequenceId SequenceId)
{
	return SequenceSchedulers.Find(SequenceId);
}

const FAbilitySequenceScheduler* UAutoBattleAbilitySequenceSubsystem::FindScheduler(
	const FAbilitySequenceId SequenceId) const
{
	return SequenceSchedulers.Find(SequenceId);
}

void UAutoBattleAbilitySequenceSubsystem::BroadcastNodeState(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId)
{
	if (!NodeId.IsValid())
	{
		return;
	}

	const FAbilitySequenceScheduler* Scheduler = FindScheduler(SequenceId);
	const FAbilitySequenceNode* Node = Scheduler ? Scheduler->FindNode(NodeId) : nullptr;
	if (Node)
	{
		OnSequenceNodeStateChanged.Broadcast(SequenceId, *Node);
	}
}

void UAutoBattleAbilitySequenceSubsystem::RetainFinishedSequence(const FAbilitySequenceId SequenceId)
{
	if (FinishedSequenceIds.Contains(SequenceId))
	{
		return;
	}

	FinishedSequenceIds.Add(SequenceId);
	while (FinishedSequenceIds.Num() > MaxRetainedFinishedSequences)
	{
		SequenceSchedulers.Remove(FinishedSequenceIds[0]);
		FinishedSequenceIds.RemoveAt(0, EAllowShrinking::No);
	}
}
