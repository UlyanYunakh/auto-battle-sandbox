// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "UnitActor.generated.h"

class UBattleAbilitySystem;
class UBattleGameplayAbility;
class UDefensiveAttributeSet;
class UOffensiveAttributeSet;
class UTexture2D;
class UUnitAsset;
class AZoneActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitInitialized);

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

	UFUNCTION(BlueprintPure, Category = "Unit")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Unit")
	TSubclassOf<UBattleGameplayAbility> GetBasicAttackAbility() const { return BasicAttackAbility; }

	UFUNCTION(BlueprintPure, Category = "Unit")
	AZoneActor* GetOwningZone() const { return OwningZone; }

	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Unit")
	FOnUnitInitialized OnInitialized;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	TObjectPtr<UBattleAbilitySystem> UnitAbilitySystem = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	FText UnitName;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	TSoftObjectPtr<UTexture2D> UnitArt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UDefensiveAttributeSet> DefensiveAttributeSet = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UOffensiveAttributeSet> OffensiveAttributeSet = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	TSubclassOf<UBattleGameplayAbility> BasicAttackAbility;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<AZoneActor> OwningZone = nullptr;

private:
	friend class AZoneActor;

	void GrantUnitAbilities(const UUnitAsset& UnitDataAsset);
	void RemoveGrantedUnitAbilities();
	void SetOwningZone(AZoneActor* ZoneActor) { OwningZone = ZoneActor; }

private:
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	FGameplayTagContainer AppliedInitialGameplayTags;
};
