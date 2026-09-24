// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitActor.generated.h"

class UBattleAbilitySystem;
class UDefensiveAttributeSet;
class UOffensiveAttributeSet;
class UTexture2D;
class UUnitAsset;

UCLASS()
class AUTOBATTLESANDBOX_API AUnitActor : public AActor
{
	GENERATED_BODY()

public:
	AUnitActor();

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void InitializeUnit(const UUnitAsset* UnitDataAsset);

	UFUNCTION(BlueprintPure, Category = "Unit")
	UBattleAbilitySystem* GetUnitAbilitySystem() const;

	UFUNCTION(BlueprintPure, Category = "Unit")
	FText GetUnitName() const;

	UFUNCTION(BlueprintPure, Category = "Unit")
	TSoftObjectPtr<UTexture2D> GetUnitArt() const;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UBattleAbilitySystem* UnitAbilitySystem = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	FText UnitName;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	TSoftObjectPtr<UTexture2D> UnitArt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UDefensiveAttributeSet> DefensiveAttributeSet = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UOffensiveAttributeSet> OffensiveAttributeSet = nullptr;
};
