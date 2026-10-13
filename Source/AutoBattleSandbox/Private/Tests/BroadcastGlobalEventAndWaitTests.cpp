#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Tasks/AbilityTask_BroadcastGlobalEventAndWait.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBroadcastGlobalEventAndWaitBoundaryTest,
	"AutoBattleSandbox.Ability.GlobalEvents.BroadcastAndWait.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBroadcastGlobalEventAndWaitBoundaryTest::RunTest(const FString&)
{
	const UAbilityTask_BroadcastGlobalEventAndWait* Task =
		NewObject<UAbilityTask_BroadcastGlobalEventAndWait>();
	TestNotNull(TEXT("The broadcast-and-wait task supports minimal construction"), Task);
	TestNull(TEXT("The task factory rejects a missing owning ability"),
	         UAbilityTask_BroadcastGlobalEventAndWait::BroadcastGlobalEventAndWait(
		         nullptr,
		         FGameplayTag(),
		         FGameplayEventData()));
	return Task != nullptr;
}

#endif
