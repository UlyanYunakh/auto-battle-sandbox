// Fill out your copyright notice in the Description page of Project Settings.

#include "DataAssets/UnitAssetCollection.h"

#if WITH_EDITOR
#include "DataAssets/UnitAsset.h"
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UUnitAssetCollection::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<const UUnitAsset*> UniqueUnitAssets;
	for (int32 UnitIndex = 0; UnitIndex < UnitAssets.Num(); ++UnitIndex)
	{
		const UUnitAsset* UnitAsset = UnitAssets[UnitIndex];
		if (UnitAsset == nullptr)
		{
			Context.AddError(FText::Format(INVTEXT("Unit asset at index {0} must not be empty."), UnitIndex));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		if (UniqueUnitAssets.Contains(UnitAsset))
		{
			Context.AddError(FText::Format(
				INVTEXT("Unit asset '{0}' is listed more than once."),
				FText::FromString(UnitAsset->GetName())));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		UniqueUnitAssets.Add(UnitAsset);
	}

	return Result;
}
#endif
