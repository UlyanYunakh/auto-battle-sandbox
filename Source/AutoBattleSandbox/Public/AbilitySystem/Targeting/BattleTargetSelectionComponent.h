#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "Components/ActorComponent.h"
#include "BattleTargetSelectionComponent.generated.h"

DECLARE_DELEGATE_OneParam(
	FAbilityManualTargetSelectionCompleted,
	const FGameplayAbilityTargetDataHandle&);
DECLARE_DELEGATE(FAbilityManualTargetSelectionCancelled);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAbilityManualSelectionActiveChanged,
	bool, bIsActive);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAbilityManualTargetVisualStateChanged,
	AActor*, Target,
	bool, bIsSelected);

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilityTargetSelectionRequestHandle
{
	GENERATED_BODY()

public:
	bool IsValid() const { return Value > 0; }
	bool operator==(const FAbilityTargetSelectionRequestHandle& Other) const { return Value == Other.Value; }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	int64 Value = 0;
};

USTRUCT()
struct FAbilityManualSelectionRequest
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Candidates;

	UPROPERTY()
	int32 MinimumTargets = 1;

	UPROPERTY()
	int32 MaximumTargets = 1;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AUTOBATTLESANDBOX_API UBattleTargetSelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBattleTargetSelectionComponent();

	bool CanBeginSelection() const;
	FAbilityTargetSelectionRequestHandle BeginSelection(
		const FAbilityManualSelectionRequest& Request,
		FAbilityManualTargetSelectionCompleted OnCompleted,
		FAbilityManualTargetSelectionCancelled OnCancelled);

	UFUNCTION(BlueprintCallable, Category = "Ability Targeting")
	bool SelectTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Ability Targeting")
	bool ConfirmSelection();

	UFUNCTION(BlueprintCallable, Category = "Ability Targeting")
	bool CancelSelection();

	bool SuspendSelection(FAbilityTargetSelectionRequestHandle Handle);
	bool ResumeSelection(FAbilityTargetSelectionRequestHandle Handle);
	bool CancelSelection(FAbilityTargetSelectionRequestHandle Handle, bool bNotifyCancelled);

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	bool IsSelecting() const { return ActiveRequestHandle.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	bool IsSelectionSuspended() const { return bSuspended; }

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	int32 GetSelectedTargetCount() const { return SelectedTargets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Ability Targeting")
	bool ShouldConfirmAfterTargetSelection() const { return ActiveRequest.MaximumTargets == 1; }

public:
	UPROPERTY(BlueprintAssignable, Category = "Ability Targeting")
	FAbilityManualSelectionActiveChanged OnSelectionActiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ability Targeting")
	FAbilityManualTargetVisualStateChanged OnTargetVisualStateChanged;

private:
	void ResetSelection();
	FGameplayAbilityTargetDataHandle BuildSelectedTargetData() const;

private:
	UPROPERTY(Transient)
	FAbilityManualSelectionRequest ActiveRequest;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SelectedTargets;

	FAbilityTargetSelectionRequestHandle ActiveRequestHandle;
	FAbilityManualTargetSelectionCompleted CompletedDelegate;
	FAbilityManualTargetSelectionCancelled CancelledDelegate;
	int64 NextRequestValue = 1;
	bool bSuspended = false;
};
