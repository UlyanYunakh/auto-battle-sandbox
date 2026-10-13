#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "DataAssets/UnitAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnitAssetAbilityDefaultsTest,
	"AutoBattleSandbox.Units.Abilities.AssetDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnitAssetAbilityDefaultsTest::RunTest(const FString&)
{
	UUnitAsset* UnitAsset = NewObject<UUnitAsset>();
	TestNotNull(TEXT("The unit asset supports minimal construction"), UnitAsset);
	if (!UnitAsset)
	{
		return false;
	}

	TestTrue(TEXT("A new unit asset has no abilities"), UnitAsset->Abilities.IsEmpty());
	UnitAsset->Abilities.Add(UBattleGameplayAbility::StaticClass());
	TestEqual(TEXT("Battle ability classes can be configured on a unit asset"), UnitAsset->Abilities.Num(), 1);
	return true;
}

#endif
