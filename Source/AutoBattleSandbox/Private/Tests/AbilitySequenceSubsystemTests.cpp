#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilitySequenceSubsystemLifecycleTest,
	"AutoBattleSandbox.AbilitySequence.Subsystem.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilitySequenceSubsystemLifecycleTest::RunTest(const FString&)
{
	UAutoBattleAbilitySequenceSubsystem* Subsystem = NewObject<UAutoBattleAbilitySequenceSubsystem>();
	TestNotNull(TEXT("The subsystem can be used with minimal construction"), Subsystem);
	if (!Subsystem)
	{
		return false;
	}

	FAbilitySequenceNodeRequest RootRequest;
	RootRequest.DebugName = TEXT("Root");
	FAbilitySequenceId SequenceId;
	FAbilitySequenceNodeId RootNodeId;
	TestTrue(TEXT("The subsystem starts a sequence"),
	         Subsystem->StartSequence(RootRequest, SequenceId, RootNodeId));
	TestTrue(TEXT("The subsystem assigns a sequence ID"), SequenceId.IsValid());
	TestTrue(TEXT("The new sequence is running"), Subsystem->IsSequenceRunning(SequenceId));

	FAbilitySequenceNodeRequest ChildRequest;
	ChildRequest.DebugName = TEXT("Child");
	FAbilitySequenceTransition Transition;
	TestTrue(TEXT("The subsystem schedules a child"),
	         Subsystem->PushChildren(SequenceId, RootNodeId, {ChildRequest}, Transition));
	const FAbilitySequenceNodeId ChildNodeId = Transition.NodeToActivate;
	TestTrue(TEXT("The scheduled child becomes active"),
	         Subsystem->MarkNodeActive(SequenceId, ChildNodeId));
	TestTrue(TEXT("Finishing the child resumes the root"), Subsystem->FinishNode(
		         SequenceId,
		         ChildNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The root is selected for resume"), Transition.NodeToResume == RootNodeId);

	TestTrue(TEXT("Finishing the root finishes the sequence"), Subsystem->FinishNode(
		         SequenceId,
		         RootNodeId,
		         EAbilitySequenceNodeResult::Succeeded,
		         Transition));
	TestTrue(TEXT("The finished sequence is no longer running"), !Subsystem->IsSequenceRunning(SequenceId));

	FAbilitySequenceSnapshot Snapshot;
	TestTrue(TEXT("Finished tree history remains queryable"),
	         Subsystem->GetSequenceSnapshot(SequenceId, Snapshot));
	TestTrue(TEXT("The snapshot reports completion"), Snapshot.State == EAbilitySequenceState::Finished);
	TestEqual(TEXT("The snapshot retains both nodes"), Snapshot.Nodes.Num(), 2);

	Subsystem->ClearFinishedSequences();
	TestFalse(TEXT("Clearing history removes finished sequences"),
	          Subsystem->GetSequenceSnapshot(SequenceId, Snapshot));
	return true;
}

#endif
