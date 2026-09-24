#include "BaseViewModelContextResolver.h"

#include "AdvancedMVVM/Interfaces/AdvancedMVVMInterface.h"

#include "Blueprint/UserWidget.h"

#include "BaseViewModel.h"

UObject* UBaseViewModelContextResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (UserWidget == nullptr || ExpectedType == nullptr)
	{
		return nullptr;
	}

	UBaseViewModel* NewBaseViewModel = NewObject<UBaseViewModel>(const_cast<UUserWidget*>(UserWidget), ExpectedType, NAME_None, RF_Transient);
	if (NewBaseViewModel != nullptr)
	{
		if (bOverrideDefaultProperties)
		{
			NewBaseViewModel->SetupProperties(DefaultProperties);
		}
		else if (UserWidget->Implements<UAdvancedMVVMInterface>())
		{
			NewBaseViewModel->SetupProperties(IAdvancedMVVMInterface::Execute_GetSettingsStruct(UserWidget));
		}

		// Outer widget it is UWorld when the widget was created from Widget Component.
		if (!UserWidget->GetOuter()->IsA<UWorld>())
		{
			NewBaseViewModel->OnConstruct(UserWidget->GetOwningPlayer());
		}
	}

	return NewBaseViewModel;
}

void UBaseViewModelContextResolver::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UBaseViewModel* BaseViewModel = Cast<UBaseViewModel>(ViewModel))
	{
		BaseViewModel->OnDestruct();
	}
}
