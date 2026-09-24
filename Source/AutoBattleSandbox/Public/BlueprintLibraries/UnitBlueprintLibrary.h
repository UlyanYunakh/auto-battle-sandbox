// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UnitBlueprintLibrary.generated.h"

class AUnitActor;
class UUnitAsset;

/**
 * Blueprint helpers for units.
 */
UCLASS()
class AUTOBATTLESANDBOX_API UUnitBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Unit",
		meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "UnitActorClass"))
	static AUnitActor* SpawnAndInitializeUnitActor(
		const UObject* WorldContextObject,
		TSubclassOf<AUnitActor> UnitActorClass,
		const UUnitAsset* UnitDataAsset,
		FTransform Transform);
};
