#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectWithAttackAnimation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FApplyGameplayEffectWithAttackAnimationBoundaryTest,
	"AutoBattleSandbox.Ability.Targeting.ApplyGameplayEffectWithAttackAnimation.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApplyGameplayEffectWithAttackAnimationBoundaryTest::RunTest(const FString&)
{
	const UAbilityTask_ApplyGameplayEffectWithAttackAnimation* Task =
		NewObject<UAbilityTask_ApplyGameplayEffectWithAttackAnimation>();
	TestNotNull(TEXT("The animated apply-effect task supports minimal construction"), Task);

	TestNull(TEXT("The class factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ApplyGameplayEffectWithAttackAnimation(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         nullptr));

	TestNull(TEXT("The spec factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectWithAttackAnimation::ApplyGameplayEffectSpecWithAttackAnimation(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         FGameplayEffectSpecHandle()));
	return Task != nullptr;
}

#endif
