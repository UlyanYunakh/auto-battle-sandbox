// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedMVVM/Viewmodels/BaseViewModel.h"
#include "Core/AutoBattleSandboxGameState.h"
#include "GameStateSettingsViewModel.generated.h"

class UUnitAsset;
class UUnitAssetCollection;

UCLASS(BlueprintType, Blueprintable)
class AUTOBATTLESANDBOX_API UGameStateSettingsViewModel : public UBaseViewModel
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetGameState(AAutoBattleSandboxGameState* InGameState);

	UFUNCTION(BlueprintPure, Category = "Settings")
	AAutoBattleSandboxGameState* GetGameState() const { return GameState; }

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void InitializeFromSettings(const FAutoBattleSandboxGameStateSettings& InSettings);

	virtual void SetupProperties(const FInstancedStruct& InData) override;

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Settings")
	FAutoBattleSandboxGameStateSettings Settings;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Units")
	TObjectPtr<UUnitAssetCollection> SpawnableUnits = nullptr;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Units")
	TArray<TObjectPtr<UUnitAsset>> UnitAssets;

protected:
	virtual void ViewModelInitialize(AActor* InActor) override;
	virtual void ViewModelClear() override;
	virtual bool IsSupportedStruct(const UScriptStruct* InData) const override;

private:
	void SetSettings(const FAutoBattleSandboxGameStateSettings& InSettings);
	void SetSpawnableUnits(UUnitAssetCollection* InSpawnableUnits);
	void RefreshUnitAssets();
	void ClearSettings();

private:
	UPROPERTY()
	TObjectPtr<AAutoBattleSandboxGameState> GameState = nullptr;
};
