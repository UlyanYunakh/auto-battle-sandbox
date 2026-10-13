#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Targeting/AbilityTargetingDefinition.h"
#include "BlueprintLibraries/AbilityTargetingBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityTargetingDefinitionBoundaryTest,
	"AutoBattleSandbox.Ability.Targeting.Definition.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityTargetingDefinitionBoundaryTest::RunTest(const FString&)
{
	UAbilityTargetingDefinition* Definition = NewObject<UAbilityTargetingDefinition>();
	TestNotNull(TEXT("The targeting definition supports minimal construction"), Definition);
	if (!Definition)
	{
		return false;
	}

	EAbilityTargetSelectionFailure Failure = EAbilityTargetSelectionFailure::None;
	TestTrue(TEXT("A manual definition needs no criteria or strategy"),
	         Definition->IsDefinitionValid(false, Failure));
	TestEqual(TEXT("A valid definition has no failure"), Failure, EAbilityTargetSelectionFailure::None);

	TestFalse(TEXT("Automatic selection requires a concrete strategy"),
	          Definition->IsDefinitionValid(true, Failure));
	TestEqual(TEXT("The missing strategy is reported"),
	          Failure,
	          EAbilityTargetSelectionFailure::MissingAutomaticStrategy);

	Definition->MinimumTargets = 2;
	Definition->MaximumTargets = 1;
	TestFalse(TEXT("An inverted target-count range is rejected"),
	          Definition->IsDefinitionValid(false, Failure));
	TestEqual(TEXT("Invalid target counts report an invalid definition"),
	          Failure,
	          EAbilityTargetSelectionFailure::InvalidDefinition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityTargetingInputHelpersTest,
	"AutoBattleSandbox.Ability.Targeting.Input.Helpers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityTargetingInputHelpersTest::RunTest(const FString&)
{
	const FAbilityTargetSelectionInput NormalInput =
		UAbilityTargetingBlueprintLibrary::MakeNormalTargetInput();
	TestEqual(TEXT("Normal input uses the definition"),
	          NormalInput.Policy,
	          EAbilityTargetInputPolicy::NormalDefinition);

	const FAbilityTargetSelectionInput RequiredInput =
		UAbilityTargetingBlueprintLibrary::MakeRequiredTargetInputFromActor(nullptr);
	TestEqual(TEXT("Required helper preserves its policy without a target"),
	          RequiredInput.Policy,
	          EAbilityTargetInputPolicy::RequireProvidedTargets);
	TestTrue(TEXT("A null actor is not added as a provided target"), RequiredInput.ProvidedActors.IsEmpty());

	const FAbilityTargetSelectionInput PreferredInput =
		UAbilityTargetingBlueprintLibrary::MakePreferredTargetInputFromActor(nullptr);
	TestEqual(TEXT("Preferred helper preserves its policy without a target"),
	          PreferredInput.Policy,
	          EAbilityTargetInputPolicy::PreferProvidedTargets);
	return true;
}

#endif
