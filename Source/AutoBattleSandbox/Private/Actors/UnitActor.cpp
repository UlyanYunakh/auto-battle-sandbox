// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/UnitActor.h"

#include "AbilitySystem/BattleAbilitySystem.h"


AUnitActor::AUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;

	UnitAbilitySystem = CreateDefaultSubobject<UBattleAbilitySystem>(TEXT("UnitAbilitySystem"));
}

void AUnitActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
