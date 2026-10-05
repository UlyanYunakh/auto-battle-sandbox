// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AutoBattleSandboxPlayerController.generated.h"

/**
 * Player controller for UI-driven auto battle interactions.
 */
UCLASS()
class AUTOBATTLESANDBOX_API AAutoBattleSandboxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAutoBattleSandboxPlayerController();

protected:
	virtual void BeginPlay() override;
};
