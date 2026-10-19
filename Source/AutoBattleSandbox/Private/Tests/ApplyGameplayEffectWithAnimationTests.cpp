#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Animation/ApplyAnimationProvider.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectWithAnimation.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FApplyGameplayEffectWithAnimationBoundaryTest,
	"AutoBattleSandbox.Ability.Targeting.ApplyGameplayEffectWithAnimation.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApplyGameplayEffectWithAnimationBoundaryTest::RunTest(const FString&)
{
	const UAbilityTask_ApplyGameplayEffectWithAnimation* Task =
		NewObject<UAbilityTask_ApplyGameplayEffectWithAnimation>();
	TestNotNull(TEXT("The provider-driven apply-effect task supports minimal construction"), Task);

	TestNull(TEXT("The class factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectWithAnimation::ApplyGameplayEffectWithAnimation(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         nullptr,
		         FInstancedStruct()));

	TestNull(TEXT("The spec factory rejects a missing owning ability"),
	         UAbilityTask_ApplyGameplayEffectWithAnimation::ApplyGameplayEffectSpecWithAnimation(
		         nullptr,
		         FGameplayAbilityTargetDataHandle(),
		         FGameplayEffectSpecHandle(),
		         FInstancedStruct()));

	FInstancedStruct MockProviderDefinition;
	FApplyAnimationMockProviderDefinition& MockConfiguration =
		MockProviderDefinition.InitializeAs<FApplyAnimationMockProviderDefinition>();
	MockConfiguration.ApplyDelaySeconds = 0.0f;
	UApplyAnimationProvider* MockProvider =
		UApplyAnimationProvider::CreateProvider(MockProviderDefinition, GetTransientPackage());
	TestNotNull(TEXT("A valid provider definition creates its runtime provider"), MockProvider);
	if (!MockProvider)
	{
		return false;
	}

	int32 ImpactCount = 0;
	int32 FinishedCount = 0;
	MockProvider->OnImpact.AddLambda([&ImpactCount]() { ++ImpactCount; });
	MockProvider->OnFinished.AddLambda([&FinishedCount]() { ++FinishedCount; });
	TestTrue(TEXT("A zero-delay mock provider starts without a world"),
	         MockProvider->Start(FApplyAnimationContext()));
	TestEqual(TEXT("The mock provider emits one impact"), ImpactCount, 1);
	TestEqual(TEXT("The mock provider emits one completion"), FinishedCount, 1);
	TestFalse(TEXT("A provider instance cannot be started twice"),
	          MockProvider->Start(FApplyAnimationContext()));

	FApplyGameplayEffectWithAnimationTaskParameters Parameters;
	Parameters.AnimationProvider = MockProviderDefinition;
	FInstancedStruct StoredParameters;
	StoredParameters.InitializeAs<FApplyGameplayEffectWithAnimationTaskParameters>(Parameters);
	const FInstancedStruct StoredParametersCopy = StoredParameters;
	const FApplyGameplayEffectWithAnimationTaskParameters* CopiedParameters =
		StoredParametersCopy.GetPtr<FApplyGameplayEffectWithAnimationTaskParameters>();
	TestNotNull(TEXT("Copied task parameters retain their provider definition"), CopiedParameters);
	TestTrue(TEXT("Copied task parameters retain the selected provider type"),
	         CopiedParameters
	         && CopiedParameters->AnimationProvider.GetScriptStruct()
	         == FApplyAnimationMockProviderDefinition::StaticStruct());
	return Task != nullptr;
}

#endif
