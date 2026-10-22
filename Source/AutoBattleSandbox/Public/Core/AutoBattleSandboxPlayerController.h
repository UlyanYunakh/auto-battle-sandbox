// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AutoBattleSandboxPlayerController.generated.h"

struct FInputActionInstance;
class AUnitActor;
class AZoneActor;
class UBattleTargetSelectionComponent;
class UInputAction;
class UInputMappingContext;

/**
 * Player controller for UI-driven auto battle interactions.
 */
UCLASS()
class AUTOBATTLESANDBOX_API AAutoBattleSandboxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAutoBattleSandboxPlayerController();

	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	UBattleTargetSelectionComponent* GetTargetSelectionComponent() const { return TargetSelectionComponent; }

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	TObjectPtr<UBattleTargetSelectionComponent> TargetSelectionComponent = nullptr;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Interaction")
	TObjectPtr<UInputAction> UniversalInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Interaction")
	TObjectPtr<UInputMappingContext> UniversalInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Interaction", meta = (ClampMin = "0.0"))
	float HoldDuration = 0.25f;

private:
	void InitializeUniversalInput();
	void ShutdownUniversalInput();
	void HandleUniversalInputStarted(const FInputActionInstance& InputActionInstance);
	void HandleUniversalInputTriggered(const FInputActionInstance& InputActionInstance);
	void HandleUniversalInputCompleted(const FInputActionInstance& InputActionInstance);
	void HandleUniversalInputCanceled(const FInputActionInstance& InputActionInstance);
	bool IsAbilityTargetSelectionActive() const;
	AUnitActor* FindUnitAtScreenPosition(const FVector2D& ScreenPosition) const;
	void SelectUnit(AUnitActor* UnitActor);
	bool BeginUnitMove(AUnitActor* UnitActor);
	void UpdateUnitMove(const FVector2D& ScreenPosition);
	void CompleteUnitMove(const FVector2D& ScreenPosition);
	void CancelUnitMove();
	bool FindZoneAtScreenPosition(const FVector2D& ScreenPosition, AZoneActor*& OutZone, int32& OutSlotIndex) const;
	void ClearHoveredZone();
	void MoveDraggedUnitToCursor(const FVector2D& ScreenPosition) const;
	void RestoreDraggedUnit();
	void ResetUniversalInputState();
	void ResetUnitMoveState();

private:
	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> PressedUnit = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> SelectedUnit = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> DraggedUnit = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AZoneActor> OriginalUnitZone = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AZoneActor> HoveredUnitZone = nullptr;

	int32 OriginalUnitSlotIndex = INDEX_NONE;
	float UnitDragPlaneHeight = 0.0f;
	bool bHoldTriggered = false;
	bool bUniversalInputInitialized = false;
};
