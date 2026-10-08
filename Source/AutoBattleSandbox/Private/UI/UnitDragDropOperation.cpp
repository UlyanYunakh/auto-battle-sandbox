// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/UnitDragDropOperation.h"

#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "BlueprintLibraries/UnitBlueprintLibrary.h"
#include "Collision/AutoBattleCollisionChannels.h"
#include "Components/Widget.h"
#include "DataAssets/UnitAsset.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UUnitDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	UpdateHoveredZone(PointerEvent);

	Super::Dragged_Implementation(PointerEvent);
}

void UUnitDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	if (!CommitAtPointer(PointerEvent))
	{
		CancelPreview();
	}

	Super::Drop_Implementation(PointerEvent);
}

void UUnitDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	const FKey EffectingButton = PointerEvent.GetEffectingButton();
	const bool bReleasedOverViewport = PointerEvent.IsTouchEvent()
		|| (EffectingButton.IsValid()
			&& !PointerEvent.IsMouseButtonDown(EffectingButton));
	if (!bReleasedOverViewport || !CommitAtPointer(PointerEvent))
	{
		CancelPreview();
	}

	Super::DragCancelled_Implementation(PointerEvent);
}

APlayerController* UUnitDragDropOperation::ResolvePlayerController_Implementation() const
{
	if (IsValid(PlayerController))
	{
		return PlayerController;
	}

	const UWorld* World = DefaultDragVisual ? DefaultDragVisual->GetWorld() : nullptr;
	return World ? World->GetFirstPlayerController() : nullptr;
}

const UUnitAsset* UUnitDragDropOperation::ResolveUnitDataAsset_Implementation() const
{
	return UnitDataAsset ? UnitDataAsset.Get() : Cast<UUnitAsset>(Payload);
}

bool UUnitDragDropOperation::UpdateHoveredZone(const FPointerEvent& PointerEvent)
{
	APlayerController* ResolvedPlayerController = ResolvePlayerController();
	if (!ResolvedPlayerController)
	{
		ClearHoveredZone();
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UnitDragDrop), true);
	if (IsValid(PreviewUnitActor))
	{
		QueryParams.AddIgnoredActor(PreviewUnitActor);
	}

	FVector2D ScreenPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(
		ResolvedPlayerController, PointerEvent.GetScreenSpacePosition(), ScreenPosition, ViewportPosition);

	FHitResult HitResult;
	const bool bHit = ResolvedPlayerController->GetHitResultAtScreenPosition(
		ScreenPosition, ECC_Zone, QueryParams, HitResult);

	AZoneActor* NewHoveredZone = bHit ? Cast<AZoneActor>(HitResult.GetActor()) : nullptr;
	const int32 SlotIndex = IsValid(NewHoveredZone)
		                        ? NewHoveredZone->GetSlotIndexAtWorldLocation(HitResult.ImpactPoint)
		                        : INDEX_NONE;

	if (!IsValid(NewHoveredZone) || SlotIndex == INDEX_NONE)
	{
		ClearHoveredZone();
		return false;
	}

	if (HoveredZone != NewHoveredZone)
	{
		ClearHoveredZone();
	}

	if (!IsValid(PreviewUnitActor))
	{
		const UUnitAsset* ResolvedUnitDataAsset = ResolveUnitDataAsset();
		if (!UnitActorClass || !ResolvedUnitDataAsset)
		{
			return false;
		}

		const FTransform SpawnTransform(NewHoveredZone->GetActorRotation(), HitResult.ImpactPoint);
		PreviewUnitActor = UUnitBlueprintLibrary::SpawnAndInitializeUnitActor(
			ResolvedPlayerController, UnitActorClass, ResolvedUnitDataAsset, SpawnTransform);
		if (!IsValid(PreviewUnitActor))
		{
			return false;
		}
	}

	PreviewUnitActor->SetActorHiddenInGame(false);
	if (!NewHoveredZone->SetPreviewUnit(PreviewUnitActor, SlotIndex))
	{
		PreviewUnitActor->SetActorHiddenInGame(true);
		return false;
	}

	HoveredZone = NewHoveredZone;
	return true;
}

bool UUnitDragDropOperation::CommitAtPointer(const FPointerEvent& PointerEvent)
{
	if (!UpdateHoveredZone(PointerEvent) || !IsValid(HoveredZone) || !HoveredZone->CommitPreviewUnit())
	{
		return false;
	}

	PreviewUnitActor = nullptr;
	HoveredZone = nullptr;
	return true;
}

void UUnitDragDropOperation::ClearHoveredZone()
{
	if (IsValid(HoveredZone))
	{
		HoveredZone->ClearPreviewUnit();
	}

	HoveredZone = nullptr;
	if (IsValid(PreviewUnitActor))
	{
		PreviewUnitActor->SetActorHiddenInGame(true);
	}
}

void UUnitDragDropOperation::CancelPreview()
{
	ClearHoveredZone();
	if (IsValid(PreviewUnitActor))
	{
		PreviewUnitActor->Destroy();
	}

	PreviewUnitActor = nullptr;
}
