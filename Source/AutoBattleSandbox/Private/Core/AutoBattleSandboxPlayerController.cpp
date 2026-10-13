// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AutoBattleSandboxPlayerController.h"

#include "AbilitySystem/Targeting/BattleTargetSelectionComponent.h"
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
}
