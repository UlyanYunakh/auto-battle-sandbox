// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AutoBattleSandboxPlayerController.h"

AAutoBattleSandboxPlayerController::AAutoBattleSandboxPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AAutoBattleSandboxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}
