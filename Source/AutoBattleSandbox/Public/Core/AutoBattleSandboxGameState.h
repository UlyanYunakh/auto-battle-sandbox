// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedMVVM/Definitions/ViewModelDefinitions.h"
#include "GameFramework/GameStateBase.h"
#include "AutoBattleSandboxGameState.generated.h"

class UUnitAssetCollection;

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAutoBattleSandboxGameStateSettings : public FDefaultViewModelProperty
{
	GENERATED_BODY()

public:
	bool operator==(const FAutoBattleSandboxGameStateSettings& Other) const
	{
		return SpawnableUnits == Other.SpawnableUnits;
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	TObjectPtr<UUnitAssetCollection> SpawnableUnits = nullptr;
};

/**
 * Replicated match state for Auto Battle Sandbox.
 */
UCLASS()
class AUTOBATTLESANDBOX_API AAutoBattleSandboxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Settings")
	const FAutoBattleSandboxGameStateSettings& GetSettings() const { return Settings; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	FAutoBattleSandboxGameStateSettings Settings;
};
