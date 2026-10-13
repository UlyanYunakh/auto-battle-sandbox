// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AutoBattleSandboxPlayerController.generated.h"

class UBattleTargetSelectionComponent;

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

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	UBattleTargetSelectionComponent* GetTargetSelectionComponent() const { return TargetSelectionComponent; }

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	TObjectPtr<UBattleTargetSelectionComponent> TargetSelectionComponent = nullptr;
};
