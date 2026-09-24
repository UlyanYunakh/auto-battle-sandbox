#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "MVVMViewModelBase.h"

#include "BaseViewModel.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogViewModel, All, All);

class UWidgetComponent;

/**
 * Base view model class that implements construction and destruction events
 */
UCLASS(Abstract, Blueprintable)
class ADVANCEDMVVM_API UBaseViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:

	/** Custom event construct. Called on create viewmodel instance */
	void OnConstruct(AActor* InActor);

	/** Init default view model properties */
	virtual void SetupProperties(const FInstancedStruct& InData);

	/** Custom event destruct. Called on remove viewmodel instance */
	void OnDestruct();

	/** Return widget that this viewmodel is attached  */
	UUserWidget* GetWidget() const { return Widget; }

	/** Return actor this viewmodel is tracking */
	AActor* GetOwner() const { return OwnerActor; }

protected:

	/** Called after OnConstruct */
	virtual void ViewModelInitialize(AActor* InActor) {}

	/** Called after OnDestruct */
	virtual void ViewModelClear() {}

	/** Checking the viewmodel support of the selected instantiated structure */
	virtual bool IsSupportedStruct(const UScriptStruct* InData) const;

private:

	/** Actor this viewmodel is tracking */
	UPROPERTY()
	AActor* OwnerActor = nullptr;

	/** Widget that this viewmodel is attached  */
	UPROPERTY()
	UUserWidget* Widget = nullptr;
};
