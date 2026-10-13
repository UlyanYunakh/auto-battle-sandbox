// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/UnitAsset.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UUnitAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (UnitName.IsEmpty())
	{
		Context.AddError(INVTEXT("Unit name must not be empty."));
		Result = EDataValidationResult::Invalid;
	}

	if (UnitArt.IsNull())
	{
		Context.AddError(INVTEXT("Unit art must be assigned."));
		Result = EDataValidationResult::Invalid;
	}

	if (Health <= 0.0f)
	{
		Context.AddError(INVTEXT("Health must be greater than 0."));
		Result = EDataValidationResult::Invalid;
	}

	if (Attack < 0.0f)
	{
		Context.AddError(INVTEXT("Attack must not be negative."));
		Result = EDataValidationResult::Invalid;
	}

	if (Armor < 0.0f)
	{
		Context.AddError(INVTEXT("Armor must not be negative."));
		Result = EDataValidationResult::Invalid;
	}

	if (!BasicAttackAbility)
	{
		Context.AddError(INVTEXT("Basic attack ability must be assigned."));
		Result = EDataValidationResult::Invalid;
	}
	else if (BasicAttackAbility->HasAnyClassFlags(CLASS_Abstract))
	{
		Context.AddError(INVTEXT("Basic attack ability must be concrete."));
		Result = EDataValidationResult::Invalid;
	}

	TSet<TSubclassOf<UBattleGameplayAbility>> SeenAbilityClasses;
	if (BasicAttackAbility)
	{
		SeenAbilityClasses.Add(BasicAttackAbility);
	}
	for (int32 AbilityIndex = 0; AbilityIndex < Abilities.Num(); ++AbilityIndex)
	{
		const TSubclassOf<UBattleGameplayAbility> AbilityClass = Abilities[AbilityIndex];
		if (!AbilityClass)
		{
			Context.AddError(FText::Format(
				INVTEXT("Ability at index {0} must be assigned."),
				AbilityIndex));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		if (AbilityClass->HasAnyClassFlags(CLASS_Abstract))
		{
			Context.AddError(FText::Format(
				INVTEXT("Ability {0} at index {1} must be concrete."),
				FText::FromName(AbilityClass->GetFName()),
				AbilityIndex));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		if (SeenAbilityClasses.Contains(AbilityClass))
		{
			Context.AddError(FText::Format(
				INVTEXT("Ability at index {0} duplicates the basic attack or another ability: {1}."),
				AbilityIndex,
				FText::FromName(AbilityClass->GetFName())));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		SeenAbilityClasses.Add(AbilityClass);
	}

	return Result;
}
#endif
