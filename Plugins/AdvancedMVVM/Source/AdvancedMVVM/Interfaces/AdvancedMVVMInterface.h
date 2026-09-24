#pragma once

#include "CoreMinimal.h"

#include "StructUtils/InstancedStruct.h"

#include "UObject/Interface.h"

#include "AdvancedMVVMInterface.generated.h"

UINTERFACE()
class ADVANCEDMVVM_API UAdvancedMVVMInterface : public UInterface
{
	GENERATED_BODY()
};

class ADVANCEDMVVM_API IAdvancedMVVMInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "AdvancedMVVMInterface")
	FInstancedStruct GetSettingsStruct() const;
};
