#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "GameFramework/Actor.h"
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

	FGlobalGameplayAbilityTrigger Trigger;
	TestTrue(TEXT("A trigger without filters has a valid filter configuration"), Trigger.AreFiltersValid());
	Trigger.Filters.Add(TInstancedStruct<FGlobalGameplayEventFilter>::Make<FGlobalGameplayEventTeamFilter>());
	TestTrue(TEXT("A concrete team filter is a valid filter configuration"), Trigger.AreFiltersValid());
	TestFalse(TEXT("A team filter rejects a missing event context"),
	          Trigger.Filters[0].Get().Matches(FGlobalGameplayEventFilterContext()));
	TestTrue(TEXT("An unset target resolver is a valid legacy configuration"),
	         Trigger.IsTargetResolverValid());

	AActor* Instigator = NewObject<AActor>();
	AActor* Target = NewObject<AActor>();
	FGameplayEventData EventData;
	EventData.Instigator = Instigator;
	EventData.Target = Target;
	FGlobalGameplayEventFilterContext EventContext;
	EventContext.EventData = &EventData;

	FAbilityTargetSelectionInput TargetingInput;
	TestTrue(TEXT("An unset resolver defaults to the event target"),
	         Trigger.ResolveTargetingInput(EventContext, TargetingInput));
	TestTrue(TEXT("A resolver does not override the selection task's input policy"),
	         TargetingInput.Policy == EAbilityTargetInputPolicy::NormalDefinition);
	TestEqual(TEXT("The default resolver provides one target"), TargetingInput.ProvidedActors.Num(), 1);
	if (TargetingInput.ProvidedActors.Num() == 1)
	{
		TestTrue(TEXT("The default resolver provides the attacked unit"),
		         TargetingInput.ProvidedActors[0].Get() == Target);
	}

	FGlobalGameplayEventParticipantTargetResolver ParticipantResolver;
	ParticipantResolver.Participant = EGlobalGameplayEventParticipant::Instigator;
	TestTrue(TEXT("The participant resolver can resolve the event instigator"),
	         ParticipantResolver.Resolve(EventContext, TargetingInput));
	TestEqual(TEXT("The instigator resolver provides one target"), TargetingInput.ProvidedActors.Num(), 1);
	if (TargetingInput.ProvidedActors.Num() == 1)
	{
		TestTrue(TEXT("The instigator resolver provides the attacking unit"),
		         TargetingInput.ProvidedActors[0].Get() == Instigator);
	}

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
