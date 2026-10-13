#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Sequences/AbilitySequenceScheduler.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilitySequenceSchedulerDepthFirstTest,
	"AutoBattleSandbox.AbilitySequence.Scheduler.DepthFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilitySequenceSchedulerDepthFirstTest::RunTest(const FString&)
{
	FAbilitySequenceScheduler Scheduler;
	FAbilitySequenceNodeId RootNodeId;
	FAbilitySequenceNodeRequest RootRequest;
	RootRequest.DebugName = TEXT("Root");

	TestTrue(TEXT("A valid sequence starts"),
	         Scheduler.StartSequence(FAbilitySequenceId(1), RootRequest, RootNodeId));
	TestTrue(TEXT("The root starts active"),
	         Scheduler.FindNode(RootNodeId)->State == EAbilitySequenceNodeState::Active);

	FAbilitySequenceNodeRequest FirstChildRequest;
	FirstChildRequest.DebugName = TEXT("FirstChild");
	FAbilitySequenceNodeRequest SecondChildRequest;
	SecondChildRequest.DebugName = TEXT("SecondChild");

	FAbilitySequenceTransition Transition;
	TestTrue(TEXT("Children can be pushed onto the active root"), Scheduler.PushChildren(
		         RootNodeId,
		         {FirstChildRequest, SecondChildRequest},
		         Transition));

	const FAbilitySequenceNodeId FirstChildNodeId = Transition.AddedNodeIds[0];
	const FAbilitySequenceNodeId SecondChildNodeId = Transition.AddedNodeIds[1];
	TestTrue(TEXT("The first child activates first"), Transition.NodeToActivate == FirstChildNodeId);
	TestTrue(TEXT("The root is suspended"),
	         Scheduler.FindNode(RootNodeId)->State == EAbilitySequenceNodeState::Suspended);
	TestTrue(TEXT("The second child remains queued"),
	         Scheduler.FindNode(SecondChildNodeId)->State == EAbilitySequenceNodeState::Queued);
	TestTrue(TEXT("The first child becomes active"), Scheduler.MarkNodeActive(FirstChildNodeId));

	FAbilitySequenceNodeRequest GrandchildRequest;
	GrandchildRequest.DebugName = TEXT("Grandchild");
	TestTrue(TEXT("The active child accepts a grandchild"), Scheduler.PushChildren(
		         FirstChildNodeId,
		         {GrandchildRequest},
		         Transition));

	const FAbilitySequenceNodeId GrandchildNodeId = Transition.NodeToActivate;
	TestTrue(TEXT("The grandchild becomes active"), Scheduler.MarkNodeActive(GrandchildNodeId));
	TestTrue(TEXT("Finishing the grandchild resumes its parent"), Scheduler.FinishNode(
		         GrandchildNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The first child is selected for resume"), Transition.NodeToResume == FirstChildNodeId);

	TestTrue(TEXT("Finishing the first child advances to its sibling"), Scheduler.FinishNode(
		         FirstChildNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The second child is selected next"), Transition.NodeToActivate == SecondChildNodeId);
	TestTrue(TEXT("The second child becomes active"), Scheduler.MarkNodeActive(SecondChildNodeId));

	TestTrue(TEXT("Finishing the second child resumes the root"), Scheduler.FinishNode(
		         SecondChildNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The root is selected for resume"), Transition.NodeToResume == RootNodeId);

	TestTrue(TEXT("Finishing the root finishes the sequence"), Scheduler.FinishNode(
		         RootNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The finish transition is reported"), Transition.bSequenceFinished);
	TestTrue(TEXT("The sequence is finished"), Scheduler.GetState() == EAbilitySequenceState::Finished);
	TestEqual(TEXT("Every node remains available as tree history"), Scheduler.GetNodes().Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilitySequenceSchedulerCancellationTest,
	"AutoBattleSandbox.AbilitySequence.Scheduler.Cancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilitySequenceSchedulerCancellationTest::RunTest(const FString&)
{
	FAbilitySequenceScheduler Scheduler;
	FAbilitySequenceNodeId RootNodeId;
	FAbilitySequenceNodeRequest RootRequest;
	RootRequest.DebugName = TEXT("Root");
	TestFalse(TEXT("An invalid sequence ID is rejected"),
	          Scheduler.StartSequence(FAbilitySequenceId(), RootRequest, RootNodeId));
	TestTrue(TEXT("A valid sequence starts after rejection"),
	         Scheduler.StartSequence(FAbilitySequenceId(7), RootRequest, RootNodeId));

	FAbilitySequenceNodeRequest ChildRequest;
	ChildRequest.DebugName = TEXT("Child");
	FAbilitySequenceTransition Transition;
	TestTrue(TEXT("A child is pushed"), Scheduler.PushChildren(RootNodeId, {ChildRequest}, Transition));

	TArray<FAbilitySequenceNodeId> NodesToCancel;
	TestTrue(TEXT("The running sequence can be cancelled"), Scheduler.CancelSequence(NodesToCancel));
	TestEqual(TEXT("Active nodes are returned leaf-first"), NodesToCancel.Num(), 2);
	TestTrue(TEXT("The child is cancelled before the root"), NodesToCancel[0] == Transition.NodeToActivate);
	TestTrue(TEXT("The root is cancelled last"), NodesToCancel[1] == RootNodeId);
	TestTrue(TEXT("The sequence reports cancellation"), Scheduler.GetState() == EAbilitySequenceState::Cancelled);

	for (const FAbilitySequenceNode& Node : Scheduler.GetNodes())
	{
		TestTrue(TEXT("Every unfinished node is cancelled"),
		         Node.State == EAbilitySequenceNodeState::Finished
		         && Node.Result == EAbilitySequenceNodeResult::Cancelled);
	}

	Scheduler.Reset();
	TestTrue(TEXT("Reset returns the scheduler to inactive"),
	         Scheduler.GetState() == EAbilitySequenceState::Inactive);
	TestTrue(TEXT("Reset clears the tree"), Scheduler.GetNodes().IsEmpty());
	return true;
}

#endif
