// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitAsset.generated.h"

class UTexture2D;

/**
 * Unit configuration data.
 */
UCLASS()
class AUTOBATTLESANDBOX_API UUnitAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit")
	FText UnitName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit")
	TSoftObjectPtr<UTexture2D> UnitArt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Health = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Attack = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Armor = 0.0f;
};
