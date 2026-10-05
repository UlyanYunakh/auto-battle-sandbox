// Fill out your copyright notice in the Description page of Project Settings.

#include "ViewModels/GameStateSettingsViewModel.h"

#include "DataAssets/UnitAssetCollection.h"
#include "Engine/World.h"

void UGameStateSettingsViewModel::SetGameState(AAutoBattleSandboxGameState* InGameState)
{
	GameState = InGameState;

	if (GameState == nullptr)
	{
		ClearSettings();
		return;
	}

	SetSettings(GameState->GetSettings());
}

void UGameStateSettingsViewModel::InitializeFromSettings(const FAutoBattleSandboxGameStateSettings& InSettings)
{
	GameState = nullptr;
	SetSettings(InSettings);
}

void UGameStateSettingsViewModel::SetupProperties(const FInstancedStruct& InData)
{
	Super::SetupProperties(InData);

	if (const FAutoBattleSandboxGameStateSettings* GameStateSettings = InData.GetPtr<
		FAutoBattleSandboxGameStateSettings>())
	{
		InitializeFromSettings(*GameStateSettings);
	}
}

void UGameStateSettingsViewModel::ViewModelInitialize(AActor* InActor)
{
	Super::ViewModelInitialize(InActor);

	if (AAutoBattleSandboxGameState* InGameState = Cast<AAutoBattleSandboxGameState>(InActor))
	{
		SetGameState(InGameState);
		return;
	}

	if (InActor != nullptr && InActor->GetWorld() != nullptr)
	{
		SetGameState(InActor->GetWorld()->GetGameState<AAutoBattleSandboxGameState>());
	}
}

void UGameStateSettingsViewModel::ViewModelClear()
{
	GameState = nullptr;
	ClearSettings();

	Super::ViewModelClear();
}

bool UGameStateSettingsViewModel::IsSupportedStruct(const UScriptStruct* InData) const
{
	return InData == FAutoBattleSandboxGameStateSettings::StaticStruct();
}

void UGameStateSettingsViewModel::SetSettings(const FAutoBattleSandboxGameStateSettings& InSettings)
{
	UE_MVVM_SET_PROPERTY_VALUE(Settings, InSettings);
	SetSpawnableUnits(Settings.SpawnableUnits);
}

void UGameStateSettingsViewModel::SetSpawnableUnits(UUnitAssetCollection* InSpawnableUnits)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(SpawnableUnits, InSpawnableUnits))
	{
		RefreshUnitAssets();
		return;
	}

	RefreshUnitAssets();
}

void UGameStateSettingsViewModel::RefreshUnitAssets()
{
	if (SpawnableUnits == nullptr)
	{
		UE_MVVM_SET_PROPERTY_VALUE(UnitAssets, TArray<TObjectPtr<UUnitAsset>>());
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(UnitAssets, SpawnableUnits->UnitAssets);
}

void UGameStateSettingsViewModel::ClearSettings()
{
	SetSettings(FAutoBattleSandboxGameStateSettings());
}
