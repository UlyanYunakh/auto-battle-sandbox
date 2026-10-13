#include "AbilitySystem/Tasks/AbilityTask_BroadcastGlobalEventAndWait.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "Engine/World.h"

UAbilityTask_BroadcastGlobalEventAndWait*
UAbilityTask_BroadcastGlobalEventAndWait::BroadcastGlobalEventAndWait(
	UBattleGameplayAbility* OwningAbility,
	const FGameplayTag EventTag,
	const FGameplayEventData EventData)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_BroadcastGlobalEventAndWait* Task =
		NewAbilityTask<UAbilityTask_BroadcastGlobalEventAndWait>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->EventTag = EventTag;
	Task->EventData = EventData;
	return Task;
}

UScriptStruct* UAbilityTask_BroadcastGlobalEventAndWait::GetTaskParametersStruct() const
{
	return FBroadcastGlobalEventAndWaitTaskParameters::StaticStruct();
}

bool UAbilityTask_BroadcastGlobalEventAndWait::InitializeFromTaskParameters(const FInstancedStruct& Parameters)
{
	const FBroadcastGlobalEventAndWaitTaskParameters* TaskParameters =
		Parameters.GetPtr<FBroadcastGlobalEventAndWaitTaskParameters>();
	if (!TaskParameters || !TaskParameters->EventTag.IsValid())
	{
		return false;
	}

	EventTag = TaskParameters->EventTag;
	EventData = FGameplayEventData();
	EventData.EventTag = EventTag;
	EventData.Instigator = CachedAbilityContext.Instigator.Get();
	EventData.Target = CachedAbilityContext.Target.Get();
	EventData.TargetData = CachedAbilityContext.TargetData;
	return true;
}

bool UAbilityTask_BroadcastGlobalEventAndWait::ActivateModularAbilityTask()
{
	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility || !EventTag.IsValid()
		|| !BattleAbility->BroadcastGlobalGameplayEvent(EventTag, EventData, Dispatch))
	{
		FinishTask(false);
		return true;
	}

	if (Dispatch.SequenceTransition.AddedNodeIds.IsEmpty())
	{
		FinishTask(true);
		return true;
	}

	WaitingSequenceId = Dispatch.SequenceId;
	WaitingSourceNodeId = Dispatch.SourceNodeId;
	const UWorld* World = BattleAbility->GetWorld();
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = World
		                                                                 ? World->GetSubsystem<
			                                                                 UAutoBattleAbilitySequenceSubsystem>()
		                                                                 : nullptr;
	if (!SequenceSubsystemInstance)
	{
		FinishTask(false);
		return true;
	}

	BindSequenceDelegates(*SequenceSubsystemInstance);
	EvaluateCurrentSequenceState();
	return true;
}

void UAbilityTask_BroadcastGlobalEventAndWait::OnDestroy(const bool bAbilityEnding)
{
	UnbindSequenceDelegates();
	Super::OnDestroy(bAbilityEnding);
}

void UAbilityTask_BroadcastGlobalEventAndWait::HandleSequenceNodeStateChanged(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNode Node)
{
	if (SequenceId != WaitingSequenceId || Node.NodeId != WaitingSourceNodeId)
	{
		return;
	}

	if (Node.State == EAbilitySequenceNodeState::Active)
	{
		FinishTask(true);
	}
	else if (Node.State == EAbilitySequenceNodeState::Finished)
	{
		FinishTask(false);
	}
}

void UAbilityTask_BroadcastGlobalEventAndWait::HandleSequenceFinished(
	const FAbilitySequenceId SequenceId,
	const EAbilitySequenceState)
{
	if (SequenceId == WaitingSequenceId)
	{
		FinishTask(false);
	}
}

void UAbilityTask_BroadcastGlobalEventAndWait::BindSequenceDelegates(
	UAutoBattleAbilitySequenceSubsystem& InSequenceSubsystem)
{
	SequenceSubsystem = &InSequenceSubsystem;
	InSequenceSubsystem.OnSequenceNodeStateChanged.AddDynamic(
		this,
		&ThisClass::HandleSequenceNodeStateChanged);
	InSequenceSubsystem.OnSequenceFinished.AddDynamic(this, &ThisClass::HandleSequenceFinished);
}

void UAbilityTask_BroadcastGlobalEventAndWait::UnbindSequenceDelegates()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);
	}

	SequenceSubsystem.Reset();
}

void UAbilityTask_BroadcastGlobalEventAndWait::EvaluateCurrentSequenceState()
{
	const UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (!SequenceSubsystemInstance)
	{
		FinishTask(false);
		return;
	}

	FAbilitySequenceSnapshot Snapshot;
	if (!SequenceSubsystemInstance->GetSequenceSnapshot(WaitingSequenceId, Snapshot))
	{
		FinishTask(false);
		return;
	}

	const FAbilitySequenceNode* SourceNode = Snapshot.Nodes.FindByPredicate(
		[this](const FAbilitySequenceNode& Node)
		{
			return Node.NodeId == WaitingSourceNodeId;
		});
	if (!SourceNode || Snapshot.State != EAbilitySequenceState::Running)
	{
		FinishTask(false);
		return;
	}

	if (SourceNode->State == EAbilitySequenceNodeState::Active)
	{
		FinishTask(true);
	}
	else if (SourceNode->State != EAbilitySequenceNodeState::Suspended)
	{
		FinishTask(false);
	}
}

void UAbilityTask_BroadcastGlobalEventAndWait::FinishTask(const bool bSucceeded)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	UnbindSequenceDelegates();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bSucceeded)
		{
			OnCompleted.Broadcast(Dispatch);
		}
		else
		{
			OnFailed.Broadcast(Dispatch);
		}
	}

	if (bSucceeded)
	{
		FModularGameplayAbilityContext UpdatedContext = CachedAbilityContext;
		UpdatedContext.Target = EventData.Target.Get();
		UpdatedContext.Instigator = EventData.Instigator.Get();
		UpdatedContext.TargetData = EventData.TargetData;
		CompleteModularAbilityTask(UpdatedContext);
	}
	else
	{
		CancelModularAbilityTask();
	}
}
