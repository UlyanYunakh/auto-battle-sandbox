#include "AbilitySystem/Targeting/BattleTargetSelectionComponent.h"

#include "GameFramework/PlayerController.h"

UBattleTargetSelectionComponent::UBattleTargetSelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UBattleTargetSelectionComponent::CanBeginSelection() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	return PlayerController && PlayerController->IsLocalController() && !ActiveRequestHandle.IsValid();
}

FAbilityTargetSelectionRequestHandle UBattleTargetSelectionComponent::BeginSelection(
	const FAbilityManualSelectionRequest& Request,
	FAbilityManualTargetSelectionCompleted OnCompleted,
	FAbilityManualTargetSelectionCancelled OnCancelled)
{
	if (!CanBeginSelection() || Request.MinimumTargets < 0
		|| Request.MaximumTargets < Request.MinimumTargets)
	{
		return FAbilityTargetSelectionRequestHandle();
	}

	ActiveRequest = Request;
	SelectedTargets.Reset();
	CompletedDelegate = MoveTemp(OnCompleted);
	CancelledDelegate = MoveTemp(OnCancelled);
	ActiveRequestHandle.Value = NextRequestValue++;
	if (NextRequestValue <= 0)
	{
		NextRequestValue = 1;
	}

	bSuspended = false;
	OnSelectionActiveChanged.Broadcast(true);
	for (AActor* Candidate : ActiveRequest.Candidates)
	{
		OnTargetVisualStateChanged.Broadcast(Candidate, false);
	}
	return ActiveRequestHandle;
}

bool UBattleTargetSelectionComponent::SelectTarget(AActor* Target)
{
	if (!ActiveRequestHandle.IsValid() || bSuspended || !IsValid(Target)
		|| !ActiveRequest.Candidates.Contains(Target))
	{
		return false;
	}

	if (SelectedTargets.Remove(Target) > 0)
	{
		OnTargetVisualStateChanged.Broadcast(Target, false);
		return true;
	}

	if (SelectedTargets.Num() >= ActiveRequest.MaximumTargets)
	{
		return false;
	}

	SelectedTargets.Add(Target);
	OnTargetVisualStateChanged.Broadcast(Target, true);
	return true;
}

bool UBattleTargetSelectionComponent::ConfirmSelection()
{
	if (!ActiveRequestHandle.IsValid() || bSuspended
		|| SelectedTargets.Num() < ActiveRequest.MinimumTargets
		|| SelectedTargets.Num() > ActiveRequest.MaximumTargets)
	{
		return false;
	}

	const FGameplayAbilityTargetDataHandle TargetData = BuildSelectedTargetData();
	FAbilityManualTargetSelectionCompleted Completion = MoveTemp(CompletedDelegate);
	ResetSelection();
	Completion.ExecuteIfBound(TargetData);
	return true;
}

bool UBattleTargetSelectionComponent::CancelSelection()
{
	return CancelSelection(ActiveRequestHandle, true);
}

bool UBattleTargetSelectionComponent::SuspendSelection(const FAbilityTargetSelectionRequestHandle Handle)
{
	if (!Handle.IsValid() || Handle != ActiveRequestHandle || bSuspended)
	{
		return false;
	}

	bSuspended = true;
	OnSelectionActiveChanged.Broadcast(false);
	return true;
}

bool UBattleTargetSelectionComponent::ResumeSelection(const FAbilityTargetSelectionRequestHandle Handle)
{
	if (!Handle.IsValid() || Handle != ActiveRequestHandle || !bSuspended)
	{
		return false;
	}

	bSuspended = false;
	OnSelectionActiveChanged.Broadcast(true);
	return true;
}

bool UBattleTargetSelectionComponent::CancelSelection(
	const FAbilityTargetSelectionRequestHandle Handle,
	const bool bNotifyCancelled)
{
	if (!Handle.IsValid() || Handle != ActiveRequestHandle)
	{
		return false;
	}

	FAbilityManualTargetSelectionCancelled Cancellation = MoveTemp(CancelledDelegate);
	ResetSelection();
	if (bNotifyCancelled)
	{
		Cancellation.ExecuteIfBound();
	}
	return true;
}

void UBattleTargetSelectionComponent::ResetSelection()
{
	for (AActor* Candidate : ActiveRequest.Candidates)
	{
		OnTargetVisualStateChanged.Broadcast(Candidate, false);
	}

	ActiveRequest = FAbilityManualSelectionRequest();
	SelectedTargets.Reset();
	ActiveRequestHandle = FAbilityTargetSelectionRequestHandle();
	CompletedDelegate.Unbind();
	CancelledDelegate.Unbind();
	bSuspended = false;
	OnSelectionActiveChanged.Broadcast(false);
}

FGameplayAbilityTargetDataHandle UBattleTargetSelectionComponent::BuildSelectedTargetData() const
{
	FGameplayAbilityTargetDataHandle TargetData;
	FGameplayAbilityTargetData_ActorArray* ActorData = new FGameplayAbilityTargetData_ActorArray();
	for (AActor* Target : SelectedTargets)
	{
		ActorData->TargetActorArray.Add(Target);
	}
	TargetData.Add(ActorData);
	return TargetData;
}
