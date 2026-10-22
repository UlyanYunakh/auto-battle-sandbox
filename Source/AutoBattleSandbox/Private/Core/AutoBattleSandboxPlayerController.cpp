// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AutoBattleSandboxPlayerController.h"

#include "AbilitySystem/Targeting/BattleTargetSelectionComponent.h"
#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"
#include "Collision/AutoBattleCollisionChannels.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputCoreTypes.h"

AAutoBattleSandboxPlayerController::AAutoBattleSandboxPlayerController()
{
	TargetSelectionComponent = CreateDefaultSubobject<
		UBattleTargetSelectionComponent>(TEXT("TargetSelectionComponent"));
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

bool AAutoBattleSandboxPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (TargetSelectionComponent && TargetSelectionComponent->IsSelecting()
		&& !TargetSelectionComponent->IsSelectionSuspended() && Params.Event == IE_Pressed)
	{
		if (Params.Key == EKeys::LeftMouseButton)
		{
			FHitResult HitResult;
			if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult)
				&& TargetSelectionComponent->SelectTarget(HitResult.GetActor()))
			{
				if (TargetSelectionComponent->ShouldConfirmAfterTargetSelection())
				{
					TargetSelectionComponent->ConfirmSelection();
				}
				return true;
			}
		}
		else if (Params.Key == EKeys::Enter || Params.Key == EKeys::SpaceBar)
		{
			return TargetSelectionComponent->ConfirmSelection();
		}
		else if (Params.Key == EKeys::RightMouseButton || Params.Key == EKeys::Escape)
		{
			return TargetSelectionComponent->CancelSelection();
		}
	}

	return Super::InputKey(Params);
}

void AAutoBattleSandboxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	InitializeUniversalInput();
}

void AAutoBattleSandboxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelUnitMove();
	if (IsValid(SelectedUnit))
	{
		SelectedUnit->SetSelected(false);
	}

	ShutdownUniversalInput();
	Super::EndPlay(EndPlayReason);
}

void AAutoBattleSandboxPlayerController::InitializeUniversalInput()
{
	if (!IsLocalController() || bUniversalInputInitialized)
	{
		return;
	}
	if (!UniversalInputAction || !UniversalInputMappingContext)
	{
		return;
	}
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(
		UniversalInputAction, ETriggerEvent::Started, this, &ThisClass::HandleUniversalInputStarted);
	EnhancedInputComponent->BindAction(
		UniversalInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleUniversalInputTriggered);
	EnhancedInputComponent->BindAction(
		UniversalInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUniversalInputCompleted);
	EnhancedInputComponent->BindAction(
		UniversalInputAction, ETriggerEvent::Canceled, this, &ThisClass::HandleUniversalInputCanceled);

	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(UniversalInputMappingContext, 0);
			bUniversalInputInitialized = true;
		}
	}
}

void AAutoBattleSandboxPlayerController::ShutdownUniversalInput()
{
	if (bUniversalInputInitialized && UniversalInputMappingContext)
	{
		if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RemoveMappingContext(UniversalInputMappingContext);
			}
		}
	}

	bUniversalInputInitialized = false;
	ResetUniversalInputState();
}

void AAutoBattleSandboxPlayerController::HandleUniversalInputStarted(const FInputActionInstance&)
{
	ResetUniversalInputState();
	if (IsAbilityTargetSelectionActive())
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		PressedUnit = FindUnitAtScreenPosition(FVector2D(MouseX, MouseY));
	}
}

void AAutoBattleSandboxPlayerController::HandleUniversalInputTriggered(
	const FInputActionInstance& InputActionInstance)
{
	if (IsAbilityTargetSelectionActive())
	{
		CancelUnitMove();
		ResetUniversalInputState();
		return;
	}

	if (!bHoldTriggered && IsValid(PressedUnit) && InputActionInstance.GetElapsedTime() >= HoldDuration)
	{
		bHoldTriggered = true;
		SelectUnit(PressedUnit);
		BeginUnitMove(PressedUnit);
	}

	if (!IsValid(DraggedUnit))
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		UpdateUnitMove(FVector2D(MouseX, MouseY));
	}
}

void AAutoBattleSandboxPlayerController::HandleUniversalInputCompleted(const FInputActionInstance&)
{
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const bool bHasMousePosition = GetMousePosition(MouseX, MouseY);

	if (IsValid(DraggedUnit))
	{
		if (bHasMousePosition)
		{
			CompleteUnitMove(FVector2D(MouseX, MouseY));
		}
		else
		{
			CancelUnitMove();
		}
	}
	else if (!bHoldTriggered && !IsAbilityTargetSelectionActive() && IsValid(PressedUnit))
	{
		SelectUnit(PressedUnit);
	}

	ResetUniversalInputState();
}

void AAutoBattleSandboxPlayerController::HandleUniversalInputCanceled(const FInputActionInstance&)
{
	CancelUnitMove();
	ResetUniversalInputState();
}

bool AAutoBattleSandboxPlayerController::IsAbilityTargetSelectionActive() const
{
	return TargetSelectionComponent && TargetSelectionComponent->IsSelecting()
		&& !TargetSelectionComponent->IsSelectionSuspended();
}

AUnitActor* AAutoBattleSandboxPlayerController::FindUnitAtScreenPosition(const FVector2D& ScreenPosition) const
{
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindUnitAtScreenPosition), true);
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AZoneActor> ZoneIterator(World); ZoneIterator; ++ZoneIterator)
		{
			QueryParams.AddIgnoredActor(*ZoneIterator);
		}
	}

	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, QueryParams, HitResult))
	{
		return nullptr;
	}

	AActor* HitActor = HitResult.GetActor();
	while (IsValid(HitActor))
	{
		if (AUnitActor* UnitActor = Cast<AUnitActor>(HitActor))
		{
			return UnitActor;
		}
		HitActor = HitActor->GetOwner();
	}

	return nullptr;
}

void AAutoBattleSandboxPlayerController::SelectUnit(AUnitActor* UnitActor)
{
	if (!IsValid(UnitActor) || SelectedUnit == UnitActor)
	{
		return;
	}

	if (IsValid(SelectedUnit))
	{
		SelectedUnit->SetSelected(false);
	}

	SelectedUnit = UnitActor;
	SelectedUnit->SetSelected(true);
}

bool AAutoBattleSandboxPlayerController::BeginUnitMove(AUnitActor* UnitActor)
{
	if (IsValid(DraggedUnit) || !IsValid(UnitActor))
	{
		return false;
	}

	AZoneActor* UnitZone = UnitActor->GetOwningZone();
	if (!IsValid(UnitZone) || UnitZone->IsBattleLocked())
	{
		return false;
	}

	const int32 UnitSlotIndex = UnitZone->GetUnits().IndexOfByKey(UnitActor);
	if (UnitSlotIndex == INDEX_NONE || !UnitZone->RemoveUnit(UnitActor))
	{
		return false;
	}

	DraggedUnit = UnitActor;
	OriginalUnitZone = UnitZone;
	OriginalUnitSlotIndex = UnitSlotIndex;
	UnitDragPlaneHeight = UnitActor->GetActorLocation().Z;
	return true;
}

void AAutoBattleSandboxPlayerController::UpdateUnitMove(const FVector2D& ScreenPosition)
{
	if (!IsValid(DraggedUnit))
	{
		return;
	}

	AZoneActor* NewHoveredZone = nullptr;
	int32 NewSlotIndex = INDEX_NONE;
	if (!FindZoneAtScreenPosition(ScreenPosition, NewHoveredZone, NewSlotIndex))
	{
		ClearHoveredZone();
		MoveDraggedUnitToCursor(ScreenPosition);
		return;
	}

	if (HoveredUnitZone != NewHoveredZone)
	{
		ClearHoveredZone();
	}

	if (NewHoveredZone->SetPreviewUnit(DraggedUnit, NewSlotIndex))
	{
		HoveredUnitZone = NewHoveredZone;
		return;
	}

	ClearHoveredZone();
	MoveDraggedUnitToCursor(ScreenPosition);
}

void AAutoBattleSandboxPlayerController::CompleteUnitMove(const FVector2D& ScreenPosition)
{
	if (!IsValid(DraggedUnit))
	{
		return;
	}

	UpdateUnitMove(ScreenPosition);
	if (IsValid(HoveredUnitZone) && HoveredUnitZone->CommitPreviewUnit())
	{
		ResetUnitMoveState();
		return;
	}

	RestoreDraggedUnit();
}

void AAutoBattleSandboxPlayerController::CancelUnitMove()
{
	if (IsValid(DraggedUnit))
	{
		RestoreDraggedUnit();
		return;
	}

	ResetUnitMoveState();
}

bool AAutoBattleSandboxPlayerController::FindZoneAtScreenPosition(
	const FVector2D& ScreenPosition,
	AZoneActor*& OutZone,
	int32& OutSlotIndex) const
{
	OutZone = nullptr;
	OutSlotIndex = INDEX_NONE;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindZoneAtScreenPosition), true);
	QueryParams.AddIgnoredActor(DraggedUnit);

	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPosition, ECC_Zone, QueryParams, HitResult))
	{
		return false;
	}

	AZoneActor* ZoneActor = Cast<AZoneActor>(HitResult.GetActor());
	if (!IsValid(ZoneActor))
	{
		return false;
	}

	const int32 SlotIndex = ZoneActor->GetSlotIndexAtWorldLocation(HitResult.ImpactPoint);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}

	OutZone = ZoneActor;
	OutSlotIndex = SlotIndex;
	return true;
}

void AAutoBattleSandboxPlayerController::ClearHoveredZone()
{
	if (IsValid(HoveredUnitZone))
	{
		HoveredUnitZone->ClearPreviewUnit();
	}

	HoveredUnitZone = nullptr;
}

void AAutoBattleSandboxPlayerController::MoveDraggedUnitToCursor(const FVector2D& ScreenPosition) const
{
	if (!IsValid(DraggedUnit))
	{
		return;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldOrigin, WorldDirection)
		|| FMath::IsNearlyZero(WorldDirection.Z))
	{
		return;
	}

	const float Distance = (UnitDragPlaneHeight - WorldOrigin.Z) / WorldDirection.Z;
	if (Distance >= 0.0f)
	{
		DraggedUnit->SetActorLocation(WorldOrigin + WorldDirection * Distance);
	}
}

void AAutoBattleSandboxPlayerController::RestoreDraggedUnit()
{
	AUnitActor* UnitActor = DraggedUnit;
	AZoneActor* UnitZone = OriginalUnitZone;
	const int32 UnitSlotIndex = OriginalUnitSlotIndex;

	ClearHoveredZone();
	if (IsValid(UnitActor) && IsValid(UnitZone))
	{
		const bool bRestoredThroughPreview = UnitZone->SetPreviewUnit(UnitActor, UnitSlotIndex)
			&& UnitZone->CommitPreviewUnit();
		if (!bRestoredThroughPreview)
		{
			UnitZone->ClearPreviewUnit();
			UnitZone->RestoreUnitAfterRearrangement(UnitActor, UnitSlotIndex);
		}
	}

	ResetUnitMoveState();
}

void AAutoBattleSandboxPlayerController::ResetUniversalInputState()
{
	PressedUnit = nullptr;
	bHoldTriggered = false;
}

void AAutoBattleSandboxPlayerController::ResetUnitMoveState()
{
	DraggedUnit = nullptr;
	OriginalUnitZone = nullptr;
	HoveredUnitZone = nullptr;
	OriginalUnitSlotIndex = INDEX_NONE;
	UnitDragPlaneHeight = 0.0f;
}
