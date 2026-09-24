// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/UnitAsset.h"

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

	return Result;
}
#endif
