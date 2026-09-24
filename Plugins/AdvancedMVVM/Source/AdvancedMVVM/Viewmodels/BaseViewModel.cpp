#include "BaseViewModel.h"

#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogViewModel)

void UBaseViewModel::OnConstruct(AActor* InActor)
{
	OwnerActor = InActor;

	Widget = GetTypedOuter<UUserWidget>();

	ViewModelInitialize(OwnerActor);
}

void UBaseViewModel::SetupProperties(const FInstancedStruct& InData)
{
	if (!IsSupportedStruct(InData.GetScriptStruct()))
	{
		UE_LOG(LogViewModel, Error, TEXT("Widget: '%s' | Viewmodel '%s' is not supporting chosen struct or struct not valid!"), *GetTypedOuter<UUserWidget>()->GetName(), *GetName());
	}
}

void UBaseViewModel::OnDestruct()
{
	ViewModelClear();

	OwnerActor = nullptr;
}

bool UBaseViewModel::IsSupportedStruct(const UScriptStruct* InData) const
{
	UE_LOG(LogViewModel, Error, TEXT("Viewmodel '%s' | '%hs' should be override!"), *GetClass()->GetName(), __FUNCTION__);
	
	return InData != nullptr;
}
