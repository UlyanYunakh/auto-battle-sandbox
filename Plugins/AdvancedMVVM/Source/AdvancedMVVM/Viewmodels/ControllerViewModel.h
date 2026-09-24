#pragma once

#include "CoreMinimal.h"

#include "BaseViewModel.h"

#include "ControllerViewModel.generated.h"

UCLASS(Abstract)
class ADVANCEDMVVM_API UControllerViewModel : public UBaseViewModel
{
	GENERATED_BODY()

protected:

	// Begin UBaseViewModel override
	virtual void ViewModelInitialize(AActor* InActor) override;
	virtual void ViewModelClear() override;
	//~End UBaseViewModel override

	/** Called when controller changed pawn */
	virtual void ControlledPawnChanged(APawn* NewPawn);

	/** Return pawn this viewmodel is tracking */	
	APawn* GetControlledPawn() const { return ControlledPawn; }

	/** Return controller this viewmodel is tracking */
	APlayerController* GetPlayerController() const { return PlayerController; }

private:

	/** Called when controller changed pawn */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

private:

	/** PlayerController this viewmodel is tracking */
	UPROPERTY()
	APlayerController* PlayerController = nullptr;

	/** Pawn this viewmodel is tracking */
	UPROPERTY()
	APawn* ControlledPawn = nullptr;
};
