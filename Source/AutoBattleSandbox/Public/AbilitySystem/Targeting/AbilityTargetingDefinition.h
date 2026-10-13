#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "Engine/DataAsset.h"
#include "AbilityTargetingDefinition.generated.h"

UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAbilityTargetingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	bool IsDefinitionValid(bool bRequireAutomaticStrategy, EAbilityTargetSelectionFailure& OutFailure) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Targeting")
	EAbilityTargetSelectionMode SelectionMode = EAbilityTargetSelectionMode::Manual;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Targeting", meta = (ClampMin = "0"))
	int32 MinimumTargets = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Targeting", meta = (ClampMin = "0"))
	int32 MaximumTargets = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Targeting", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FAbilityTargetCriterion>> Criteria;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Targeting", meta = (ExcludeBaseStruct))
	TInstancedStruct<FAbilityTargetSelectionStrategy> AutomaticStrategy;
};
