// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintLibraries/UnitBlueprintLibrary.h"

#include "Actors/UnitActor.h"
#include "DataAssets/UnitAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AUnitActor* UUnitBlueprintLibrary::SpawnAndInitializeUnitActor(
	const UObject* WorldContextObject,
	TSubclassOf<AUnitActor> UnitActorClass,
	const UUnitAsset* UnitDataAsset,
	FTransform Transform)
{
	if (!WorldContextObject || !UnitActorClass || !UnitDataAsset)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AUnitActor* UnitActor = World->SpawnActorDeferred<AUnitActor>(
		UnitActorClass,
		Transform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!UnitActor)
	{
		return nullptr;
	}

	UnitActor->InitializeUnit(UnitDataAsset);
	UnitActor->FinishSpawning(Transform);

	return UnitActor;
}
