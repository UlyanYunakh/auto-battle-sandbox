#include "ControllerViewModel.h"

void UControllerViewModel::ViewModelInitialize(AActor* InActor)
{
	Super::ViewModelInitialize(InActor);

	PlayerController = Cast<APlayerController>(InActor);
	if (PlayerController != nullptr)
	{
		PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);

		HandlePossessedPawnChanged(nullptr, PlayerController->GetPawn());
	}
}

void UControllerViewModel::ViewModelClear()
{
	if (PlayerController != nullptr)
	{
		PlayerController->OnPossessedPawnChanged.RemoveAll(this);
		PlayerController = nullptr;
		HandlePossessedPawnChanged(ControlledPawn, nullptr);
	}

	ControlledPawn = nullptr;
	Super::ViewModelClear();
}

void UControllerViewModel::ControlledPawnChanged(APawn* NewPawn)
{
	ControlledPawn = NewPawn;
}

void UControllerViewModel::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (OldPawn != NewPawn)
	{
		ControlledPawnChanged(NewPawn);
	}
}
