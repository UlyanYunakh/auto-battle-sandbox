#pragma once

#include "CoreMinimal.h"
#include "ModularAbilityTask.h"
#include "StructUtils/InstancedStruct.h"
#include "ModularAbilityTaskDefinition.generated.h"

/** Base type for designer-configurable modular ability task parameters. */
USTRUCT(BlueprintType)
struct MODULARABILITYSYSTEM_API FModularAbilityTaskParameters
{
	GENERATED_BODY()
};

/** A modular ability task class paired with the parameters required by that class. */
USTRUCT(BlueprintType)
struct MODULARABILITYSYSTEM_API FModularAbilityTaskDefinition
{
	GENERATED_BODY()

public:
	/** Reinitializes Parameters when TaskClass expects a different parameter struct. */
	void SynchronizeParameters();
	bool IsValid() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Ability Task")
	TSubclassOf<UModularAbilityTask> TaskClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Ability Task",
		meta = (BaseStruct = "/Script/ModularAbilitySystem.ModularAbilityTaskParameters",
			ExcludeBaseStruct, StructTypeConst))
	FInstancedStruct Parameters;
};
