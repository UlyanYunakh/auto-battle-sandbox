#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleAbilitySystemAdapterBoundaryTest,
	"AutoBattleSandbox.Ability.Sequence.Adapter.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleAbilitySystemAdapterBoundaryTest::RunTest(const FString&)
{
	UBattleAbilitySystem* AbilitySystem = NewObject<UBattleAbilitySystem>();
	TestNotNull(TEXT("The battle ability system supports minimal construction"), AbilitySystem);
	if (!AbilitySystem)
	{
		return false;
	}

	TestFalse(TEXT("The lifecycle adapter does not require ticking"), AbilitySystem->PrimaryComponentTick.bCanEverTick);

	FGlobalGameplayEventDispatch Dispatch;
	TestFalse(TEXT("Broadcasting without an active sequence is rejected"),
	          AbilitySystem->BroadcastGlobalGameplayEvent(
		          FGameplayAbilitySpecHandle(),
		          FGameplayTag(),
		          FGameplayEventData(),
		          Dispatch));
	TestFalse(TEXT("A rejected broadcast does not allocate an event ID"), Dispatch.EventId.IsValid());
	return true;
}

#endif
