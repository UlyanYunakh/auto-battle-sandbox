// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayAbilitySpecHandle.h"

#include "ModularGameplayAbilityContext.generated.h"

/**
 * Context shared between modular gameplay abilities.
 */
USTRUCT(BlueprintType)
struct MODULARABILITYSYSTEM_API FModularGameplayAbilityContext
{
	GENERATED_BODY()

	/** Primary actor targeted by the ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	TObjectPtr<const AActor> Target = nullptr;

	/** Polymorphic targeting information associated with the ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	FGameplayAbilityTargetDataHandle TargetData;

	/** Avatar actor that executes the ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	TObjectPtr<const AActor> Avatar = nullptr;

	/** Actor that owns the ability system component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	TObjectPtr<const AActor> Owner = nullptr;

	/** Actor responsible for initiating the ability activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	TObjectPtr<const AActor> Instigator = nullptr;

	/** Handle of the granted ability associated with this context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Gameplay Ability Context")
	FGameplayAbilitySpecHandle AbilityHandle;
};
