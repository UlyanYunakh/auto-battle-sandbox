// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitActor.generated.h"

class UBattleAbilitySystem;

UCLASS()
class AUTOBATTLESANDBOX_API AUnitActor : public AActor
{
	GENERATED_BODY()

public:
	AUnitActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UBattleAbilitySystem* UnitAbilitySystem = nullptr;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
