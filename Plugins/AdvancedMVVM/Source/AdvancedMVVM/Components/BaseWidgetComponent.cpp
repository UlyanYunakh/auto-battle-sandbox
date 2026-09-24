#include "BaseWidgetComponent.h"

#include "AdvancedMVVM/Viewmodels/BaseViewModel.h"

#include "View/MVVMView.h"

void UBaseWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestructViewmodel();
}

void UBaseWidgetComponent::UpdateWidget()
{
	Super::UpdateWidget();

	if (HasBegunPlay() && !bViewModelsInitialized)
	{
		bViewModelsInitialized = ConstructViewmodel();
	}
}

bool UBaseWidgetComponent::ConstructViewmodel()
{
	const UUserWidget* UserWidget = GetWidget();
	if (UserWidget == nullptr)
	{
		return false;
	}

	const UMVVMView* MVVMView = Cast<UMVVMView>(UserWidget->GetExtension<UMVVMView>());
	if (MVVMView == nullptr)
	{
		return false;
	}

	bool bResult = false;
	for (const FMVVMView_Source& ViewSource : MVVMView->GetSources())
	{
		if (UBaseViewModel* BaseViewModel = Cast<UBaseViewModel>(ViewSource.Source))
		{
			//BaseViewModel->OnConstruct(this);

			BaseViewModel->OnConstruct(GetOwner());

			bResult = true;
		}
	}

	return bResult;
}

void UBaseWidgetComponent::DestructViewmodel()
{
	const UUserWidget* UserWidget = GetWidget();
	if (UserWidget == nullptr)
	{
		return;
	}

	const UMVVMView* MVVMView = Cast<UMVVMView>(UserWidget->GetExtension<UMVVMView>());
	if (MVVMView == nullptr)
	{
		return;
	}

	for (const FMVVMView_Source& ViewSource : MVVMView->GetSources())
	{
		if (UBaseViewModel* BaseViewModel = Cast<UBaseViewModel>(ViewSource.Source))
		{
			BaseViewModel->OnDestruct();
		}
	}
}