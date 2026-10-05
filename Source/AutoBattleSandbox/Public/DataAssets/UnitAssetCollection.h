// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitAssetCollection.generated.h"

class UUnitAsset;

/**
 * Collection of units available to UI lists and selection widgets.
 */
UCLASS()
class AUTOBATTLESANDBOX_API UUnitAssetCollection : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
	TArray<TObjectPtr<UUnitAsset>> UnitAssets;
};
