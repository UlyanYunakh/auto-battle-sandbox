#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectToTargets.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FApplyGameplayEffectToTargetsBoundaryTest,
	"AutoBattleSandbox.Ability.Targeting.ApplyGameplayEffect.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApplyGameplayEffectToTargetsBoundaryTest::RunTest(const FString&)
{
	const UAbilityTask_ApplyGameplayEffectToTargets* Task =
		NewObject<UAbilityTask_ApplyGameplayEffectToTargets>();
	TestNotNull(TEXT("The apply-effect task supports minimal construction"), Task);

	TestNull(TEXT("The class factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectToTargets::ApplyGameplayEffectToTargets(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         nullptr));

	TestNull(TEXT("The spec factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectToTargets::ApplyGameplayEffectSpecToTargets(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         FGameplayEffectSpecHandle()));
	return Task != nullptr;
}

#endif
