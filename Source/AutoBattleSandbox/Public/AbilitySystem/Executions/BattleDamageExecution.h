#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BattleDamageExecution.generated.h"

/** Calculates basic battle damage from the source Attack and target Armor attributes. */
UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UBattleDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UBattleDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
