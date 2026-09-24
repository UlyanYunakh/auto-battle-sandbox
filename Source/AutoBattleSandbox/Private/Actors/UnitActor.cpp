// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/UnitActor.h"

#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"
#include "AbilitySystem/AttributeSet/OffensiveAttributeSet.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "DataAssets/UnitAsset.h"


AUnitActor::AUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;

	UnitAbilitySystem = CreateDefaultSubobject<UBattleAbilitySystem>(TEXT("UnitAbilitySystem"));
	DefensiveAttributeSet = CreateDefaultSubobject<UDefensiveAttributeSet>(TEXT("DefensiveAttributeSet"));
	OffensiveAttributeSet = CreateDefaultSubobject<UOffensiveAttributeSet>(TEXT("OffensiveAttributeSet"));
}

void AUnitActor::InitializeUnit(const UUnitAsset* UnitDataAsset)
{
	if (!UnitDataAsset || !UnitAbilitySystem || !DefensiveAttributeSet || !OffensiveAttributeSet)
	{
		return;
	}

	UnitAbilitySystem->ClearAllAbilities();
	UnitAbilitySystem->RemoveAllSpawnedAttributes();
	UnitAbilitySystem->InitAbilityActorInfo(this, this);
	UnitAbilitySystem->AddAttributeSetSubobject(DefensiveAttributeSet.Get());
	UnitAbilitySystem->AddAttributeSetSubobject(OffensiveAttributeSet.Get());

	UnitName = UnitDataAsset->UnitName;
	UnitArt = UnitDataAsset->UnitArt;

	DefensiveAttributeSet->SetDefaultHealth(UnitDataAsset->Health);
	DefensiveAttributeSet->SetHealth(UnitDataAsset->Health);
	DefensiveAttributeSet->SetDefaultArmor(UnitDataAsset->Armor);
	DefensiveAttributeSet->SetArmor(UnitDataAsset->Armor);
	OffensiveAttributeSet->SetAttack(UnitDataAsset->Attack);

	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetDefaultHealthAttribute(),
	                                           UnitDataAsset->Health);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetHealthAttribute(), UnitDataAsset->Health);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetDefaultArmorAttribute(),
	                                           UnitDataAsset->Armor);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetArmorAttribute(), UnitDataAsset->Armor);
	UnitAbilitySystem->SetNumericAttributeBase(UOffensiveAttributeSet::GetAttackAttribute(), UnitDataAsset->Attack);
}

UBattleAbilitySystem* AUnitActor::GetUnitAbilitySystem() const
{
	return UnitAbilitySystem;
}

FText AUnitActor::GetUnitName() const
{
	return UnitName;
}

TSoftObjectPtr<UTexture2D> AUnitActor::GetUnitArt() const
{
	return UnitArt;
}

void AUnitActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
