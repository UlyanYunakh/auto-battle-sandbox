#pragma once

#include "CoreMinimal.h"

#include "Components/WidgetComponent.h"

#include "BaseWidgetComponent.generated.h"

/** After initializing the widget, calls the construct function on the widget's view model */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class ADVANCEDMVVM_API UBaseWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

protected:

	// Begin UWidgetComponent override
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UpdateWidget() override;
	//~End UWidgetComponent override

	/** Takes the viewmodel out of the widget and calls OnConstruct() */
	bool ConstructViewmodel();

	/** Takes the viewmodel out of the widget and calls OnDestruct() */
	void DestructViewmodel();
	
private:
	
	/** Signals that view models have been initialized */
	bool bViewModelsInitialized = false;
};
