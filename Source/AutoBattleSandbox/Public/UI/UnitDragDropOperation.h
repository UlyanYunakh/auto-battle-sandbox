// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "UnitDragDropOperation.generated.h"

class APlayerController;
class AUnitActor;
class AZoneActor;
class UUnitAsset;

/**
 * Spawns and places a unit while it is dragged from the UI over a zone.
 */
UCLASS(BlueprintType, Blueprintable)
class AUTOBATTLESANDBOX_API UUnitDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	virtual void Dragged_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Drag and Drop", meta = (ExposeOnSpawn = "true"))
	TSubclassOf<AUnitActor> UnitActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Drag and Drop", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UUnitAsset> UnitDataAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Drag and Drop", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<APlayerController> PlayerController = nullptr;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Unit Drag and Drop")
	APlayerController* ResolvePlayerController() const;
	virtual APlayerController* ResolvePlayerController_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Unit Drag and Drop")
	const UUnitAsset* ResolveUnitDataAsset() const;
	virtual const UUnitAsset* ResolveUnitDataAsset_Implementation() const;

private:
	bool UpdateHoveredZone(const FPointerEvent& PointerEvent);
	bool CommitAtPointer(const FPointerEvent& PointerEvent);
	void ClearHoveredZone();
	void CancelPreview();

private:
	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> PreviewUnitActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AZoneActor> HoveredZone = nullptr;
};
