#include "AbilitySystem/Targeting/AbilityTargetingDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UAbilityTargetingDefinition::IsDefinitionValid(
	const bool bRequireAutomaticStrategy,
	EAbilityTargetSelectionFailure& OutFailure) const
{
	OutFailure = EAbilityTargetSelectionFailure::None;
	if (MinimumTargets < 0 || MaximumTargets < MinimumTargets)
	{
		OutFailure = EAbilityTargetSelectionFailure::InvalidDefinition;
		return false;
	}

	for (const TInstancedStruct<FAbilityTargetCriterion>& Criterion : Criteria)
	{
		if (!Criterion.IsValid() || Criterion.GetScriptStruct() == FAbilityTargetCriterion::StaticStruct())
		{
			OutFailure = EAbilityTargetSelectionFailure::InvalidDefinition;
			return false;
		}
	}

	if (bRequireAutomaticStrategy
		&& (!AutomaticStrategy.IsValid()
			|| AutomaticStrategy.GetScriptStruct() == FAbilityTargetSelectionStrategy::StaticStruct()))
	{
		OutFailure = EAbilityTargetSelectionFailure::MissingAutomaticStrategy;
		return false;
	}

	return true;
}

#if WITH_EDITOR
EDataValidationResult UAbilityTargetingDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	EAbilityTargetSelectionFailure Failure;
	const bool bRequiresStrategy = SelectionMode == EAbilityTargetSelectionMode::Automatic;
	if (!IsDefinitionValid(bRequiresStrategy, Failure))
	{
		Context.AddError(FText::Format(
			INVTEXT("Invalid ability targeting definition ({0})."),
			FText::FromString(UEnum::GetValueAsString(Failure))));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
