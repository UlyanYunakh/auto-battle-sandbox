#pragma once

#include "CoreMinimal.h"

#include "StructUtils/InstancedStruct.h"
#include "View/MVVMViewModelContextResolver.h"

#include "BaseViewModelContextResolver.generated.h"

/**
 * Base Viewmodel context resolver class that implements calling construction and destruction events on created viewmodel instance
 */
UCLASS()
class ADVANCEDMVVM_API UBaseViewModelContextResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

protected:

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;

protected:

	/** Flag indicating whether default property values should be overridden */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bOverrideDefaultProperties = false;

	/** Structure containing default property values */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ExcludeBaseStruct, EditCondition="bOverrideDefaultProperties", BaseStruct = "/Script/AdvancedMVVM.DefaultViewModelProperty"))
	FInstancedStruct DefaultProperties;
};
