#include "AbilitySystem/Executions/BattleDamageExecution.h"

#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"
#include "AbilitySystem/AttributeSet/OffensiveAttributeSet.h"

namespace
{
	struct FBattleDamageCaptureDefinitions
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

		FBattleDamageCaptureDefinitions()
		{
			AttackProperty = UOffensiveAttributeSet::GetAttackAttribute().GetUProperty();
			AttackDef = FGameplayEffectAttributeCaptureDefinition(
				AttackProperty,
				EGameplayEffectAttributeCaptureSource::Source,
				true);

			ArmorProperty = UDefensiveAttributeSet::GetArmorAttribute().GetUProperty();
			ArmorDef = FGameplayEffectAttributeCaptureDefinition(
				ArmorProperty,
				EGameplayEffectAttributeCaptureSource::Target,
				false);

			HealthProperty = UDefensiveAttributeSet::GetHealthAttribute().GetUProperty();
			HealthDef = FGameplayEffectAttributeCaptureDefinition(
				HealthProperty,
				EGameplayEffectAttributeCaptureSource::Target,
				false);
		}
	};

	const FBattleDamageCaptureDefinitions& GetBattleDamageCaptureDefinitions()
	{
		static const FBattleDamageCaptureDefinitions CaptureDefinitions;
		return CaptureDefinitions;
	}
}

UBattleDamageExecution::UBattleDamageExecution()
{
	const FBattleDamageCaptureDefinitions& CaptureDefinitions = GetBattleDamageCaptureDefinitions();
	RelevantAttributesToCapture.Add(CaptureDefinitions.AttackDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.ArmorDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.HealthDef);
}

void UBattleDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	const FBattleDamageCaptureDefinitions& CaptureDefinitions = GetBattleDamageCaptureDefinitions();
	float Attack = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CaptureDefinitions.AttackDef,
		EvaluationParameters,
		Attack);

	float Armor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CaptureDefinitions.ArmorDef,
		EvaluationParameters,
		Armor);

	float Health = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		CaptureDefinitions.HealthDef,
		EvaluationParameters,
		Health);

	const float Damage = FMath::Max(Attack, 0.0f);
	const float ArmorDamage = FMath::Min(Damage, FMath::Max(Armor, 0.0f));
	const float RemainingDamage = Damage - ArmorDamage;
	const float HealthDamage = FMath::Min(RemainingDamage, FMath::Max(Health, 0.0f));

	if (ArmorDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UDefensiveAttributeSet::GetArmorAttribute(),
			EGameplayModOp::Additive,
			-ArmorDamage));
	}

	if (HealthDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UDefensiveAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-HealthDamage));
	}
}
