#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleGameplayAbilityDefaultsTest,
	"AutoBattleSandbox.Ability.GlobalEvents.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleGameplayAbilityDefaultsTest::RunTest(const FString&)
{
	const UBattleGameplayAbility* Ability = NewObject<UBattleGameplayAbility>();
	TestNotNull(TEXT("The project ability base is constructible"), Ability);
	if (!Ability)
	{
		return false;
	}

	TestTrue(TEXT("Battle abilities are instanced per actor"),
	         Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor);
	TestTrue(TEXT("Battle abilities have no global triggers by default"),
	         Ability->GetGlobalEventTriggers().IsEmpty());
	TestNull(TEXT("An invalid event tag cannot select a trigger"),
	         Ability->FindGlobalEventTrigger(FGameplayTag()));

	int32 Priority = INDEX_NONE;
	TestFalse(TEXT("An invalid event tag has no trigger priority"),
	          Ability->TryGetGlobalEventTriggerPriority(FGameplayTag(), Priority));
	TestEqual(TEXT("A failed priority lookup resets the output"), Priority, 0);

	FGlobalGameplayEventDispatch Dispatch;
	TestFalse(TEXT("An ability without actor info cannot broadcast a global event"),
	          Ability->BroadcastGlobalGameplayEvent(
		          FGameplayTag(),
		          FGameplayEventData(),
		          Dispatch));
	TestFalse(TEXT("A rejected ability broadcast does not allocate an event ID"),
	          Dispatch.EventId.IsValid());
	return true;
}

#endif
