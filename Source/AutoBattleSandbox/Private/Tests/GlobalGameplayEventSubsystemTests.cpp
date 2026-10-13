#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Events/GlobalGameplayEventSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGlobalGameplayEventSubsystemBoundaryTest,
	"AutoBattleSandbox.Ability.GlobalEvents.Subsystem.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGlobalGameplayEventSubsystemBoundaryTest::RunTest(const FString&)
{
	UAutoBattleGlobalGameplayEventSubsystem* Subsystem =
		NewObject<UAutoBattleGlobalGameplayEventSubsystem>();
	TestNotNull(TEXT("The global event subsystem supports minimal construction"), Subsystem);
	if (!Subsystem)
	{
		return false;
	}

	TestFalse(TEXT("An invalid subscription cannot be removed"),
	          Subsystem->UnregisterAbilitySubscription(FGlobalGameplayAbilitySubscriptionId()));
	TestEqual(TEXT("A new registry is empty"), Subsystem->GetSubscriptionCount(), 0);

	FGlobalGameplayEventDispatch Dispatch;
	TestFalse(TEXT("An invalid event cannot be broadcast"), Subsystem->BroadcastGlobalEvent(
		          FAbilitySequenceId(),
		          FAbilitySequenceNodeId(),
		          FGameplayTag(),
		          FGameplayEventData(),
		          Dispatch));
	TestFalse(TEXT("A rejected event does not allocate an event ID"), Dispatch.EventId.IsValid());
	return true;
}

#endif
