#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Targeting/AbilityTargetingDefinition.h"
#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"
#include "BlueprintLibraries/AbilityTargetingBlueprintLibrary.h"
#include "DataAssets/UnitAsset.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityTargetSelfCriterionTest,
	"AutoBattleSandbox.Ability.Targeting.Criterion.Self",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityTargetSelfCriterionTest::RunTest(const FString&)
{
	AUnitActor* AbilityOwner = NewObject<AUnitActor>();
	AUnitActor* OtherUnit = NewObject<AUnitActor>();
	if (!AbilityOwner || !OtherUnit)
	{
		AddError(TEXT("The self-target criterion test could not create its units."));
		return false;
	}

	FAbilityTargetingContext Context;
	Context.SourceActor = AbilityOwner;

	FAbilityTargetCandidate OwnerCandidate;
	OwnerCandidate.Actor = AbilityOwner;
	FAbilityTargetCandidate OtherCandidate;
	OtherCandidate.Actor = OtherUnit;

	const FAbilityTargetSelfCriterion Criterion;
	TestTrue(TEXT("The ability owner is a valid self target"),
	         Criterion.IsTargetValid(Context, OwnerCandidate));
	TestFalse(TEXT("Another unit is not a valid self target"),
	          Criterion.IsTargetValid(Context, OtherCandidate));

	Context.SourceActor = nullptr;
	TestFalse(TEXT("A missing ability owner cannot match a self target"),
	          Criterion.IsTargetValid(Context, OwnerCandidate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityTargetAdjacentUnitCriterionTest,
	"AutoBattleSandbox.Ability.Targeting.Criterion.AdjacentUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityTargetAdjacentUnitCriterionTest::RunTest(const FString&)
{
	AZoneActor* Zone = NewObject<AZoneActor>();
	AZoneActor* OtherZone = NewObject<AZoneActor>();
	AUnitActor* LeftUnit = NewObject<AUnitActor>();
	AUnitActor* AbilityOwner = NewObject<AUnitActor>();
	AUnitActor* RightUnit = NewObject<AUnitActor>();
	AUnitActor* OtherZoneUnit = NewObject<AUnitActor>();
	if (!Zone || !OtherZone || !LeftUnit || !AbilityOwner || !RightUnit || !OtherZoneUnit)
	{
		AddError(TEXT("The adjacent-unit criterion test could not create its actors."));
		return false;
	}

	if (!Zone->AddUnit(LeftUnit) || !Zone->AddUnit(AbilityOwner) || !Zone->AddUnit(RightUnit)
		|| !OtherZone->AddUnit(OtherZoneUnit))
	{
		AddError(TEXT("The adjacent-unit criterion test could not arrange its units."));
		return false;
	}

	FAbilityTargetingContext Context;
	Context.SourceActor = AbilityOwner;
	FAbilityTargetCandidate LeftCandidate;
	LeftCandidate.Actor = LeftUnit;
	FAbilityTargetCandidate RightCandidate;
	RightCandidate.Actor = RightUnit;
	FAbilityTargetCandidate OtherZoneCandidate;
	OtherZoneCandidate.Actor = OtherZoneUnit;

	FAbilityTargetAdjacentUnitCriterion Criterion;
	TestTrue(TEXT("Both sides include the immediate left unit"),
	         Criterion.IsTargetValid(Context, LeftCandidate));
	TestTrue(TEXT("Both sides include the immediate right unit"),
	         Criterion.IsTargetValid(Context, RightCandidate));
	TestFalse(TEXT("A unit in another zone is never adjacent"),
	          Criterion.IsTargetValid(Context, OtherZoneCandidate));

	Criterion.Direction = EAbilityTargetAdjacentDirection::Left;
	TestTrue(TEXT("Left includes the unit in the previous slot"),
	         Criterion.IsTargetValid(Context, LeftCandidate));
	TestFalse(TEXT("Left excludes the unit in the next slot"),
	          Criterion.IsTargetValid(Context, RightCandidate));

	Criterion.Direction = EAbilityTargetAdjacentDirection::Right;
	TestFalse(TEXT("Right excludes the unit in the previous slot"),
	          Criterion.IsTargetValid(Context, LeftCandidate));
	TestTrue(TEXT("Right includes the unit in the next slot"),
	         Criterion.IsTargetValid(Context, RightCandidate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityTargetWeakestSelectionStrategyTest,
	"AutoBattleSandbox.Ability.Targeting.Strategy.Weakest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityTargetWeakestSelectionStrategyTest::RunTest(const FString&)
{
	AUnitActor* StrongerUnit = NewObject<AUnitActor>();
	AUnitActor* WeakerUnit = NewObject<AUnitActor>();
	UUnitAsset* StrongerUnitData = NewObject<UUnitAsset>();
	UUnitAsset* WeakerUnitData = NewObject<UUnitAsset>();
	if (!StrongerUnit || !WeakerUnit || !StrongerUnitData || !WeakerUnitData)
	{
		AddError(TEXT("The weakest-target strategy test could not create its units."));
		return false;
	}

	StrongerUnitData->Health = 75.0f;
	WeakerUnitData->Health = 20.0f;
	StrongerUnit->InitializeUnit(StrongerUnitData);
	WeakerUnit->InitializeUnit(WeakerUnitData);

	TArray<FAbilityTargetCandidate> Candidates;
	Candidates.AddDefaulted_GetRef().Actor = StrongerUnit;
	Candidates.AddDefaulted_GetRef().Actor = WeakerUnit;

	const FAbilityTargetWeakestSelectionStrategy Strategy;
	Strategy.OrderCandidates(FAbilityTargetingContext(), Candidates);

	TestEqual(TEXT("The strategy preserves every candidate"), Candidates.Num(), 2);
	if (Candidates.Num() == 2)
	{
		TestTrue(TEXT("The unit with the lowest current health is selected first"),
		         Candidates[0].Actor == WeakerUnit);
		TestTrue(TEXT("The healthier unit is selected afterward"),
		         Candidates[1].Actor == StrongerUnit);
	}

	return true;
}

#endif
