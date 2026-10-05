// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AutoBattleSandboxGameMode.h"

#include "Core/AutoBattleSandboxGameState.h"
#include "Core/AutoBattleSandboxHUD.h"
#include "Core/AutoBattleSandboxPlayerController.h"
#include "Core/AutoBattleSandboxPlayerState.h"

AAutoBattleSandboxGameMode::AAutoBattleSandboxGameMode()
{
	GameStateClass = AAutoBattleSandboxGameState::StaticClass();
	PlayerControllerClass = AAutoBattleSandboxPlayerController::StaticClass();
	PlayerStateClass = AAutoBattleSandboxPlayerState::StaticClass();
	HUDClass = AAutoBattleSandboxHUD::StaticClass();
	DefaultPawnClass = nullptr;
}
