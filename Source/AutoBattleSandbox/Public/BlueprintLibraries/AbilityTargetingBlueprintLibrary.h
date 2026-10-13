#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityTargetingBlueprintLibrary.generated.h"

UCLASS()
class AUTOBATTLESANDBOX_API UAbilityTargetingBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static FAbilityTargetSelectionInput MakeNormalTargetInput();

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static FAbilityTargetSelectionInput MakeRequiredTargetInputFromActor(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static FAbilityTargetSelectionInput MakePreferredTargetInputFromActor(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static FAbilityTargetSelectionInput MakeTargetInputFromActors(
		const TArray<AActor*>& Targets,
		EAbilityTargetInputPolicy Policy);

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static FAbilityTargetSelectionInput MakeTargetInputFromEventData(
		const FGameplayEventData& EventData,
		EAbilityTargetInputPolicy Policy = EAbilityTargetInputPolicy::RequireProvidedTargets);

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	static TArray<AActor*> GetActorsFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData);
};
