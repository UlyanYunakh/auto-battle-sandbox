#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "GlobalGameplayEventSubsystem.generated.h"

class UAutoBattleAbilitySequenceSubsystem;
class UBattleAbilitySystem;
class UBattleGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FGlobalGameplayEventBroadcastSignature,
	FGlobalGameplayEventDispatch, Dispatch);

DECLARE_MULTICAST_DELEGATE_OneParam(
	FGlobalGameplayAbilityActivationRequestedSignature,
	const FGlobalGameplayAbilityActivationRequest&);

struct FRegisteredGlobalGameplayAbilitySubscription
{
	FGlobalGameplayAbilitySubscriptionId SubscriptionId;
	TWeakObjectPtr<UBattleAbilitySystem> AbilitySystem;
	FGameplayAbilitySpecHandle AbilityHandle;
	TSubclassOf<UBattleGameplayAbility> AbilityClass;
};

struct FResolvedGlobalGameplayAbilitySubscription
{
	FGlobalGameplayAbilitySubscriptionId SubscriptionId;
	FAbilityTargetSelectionInput TargetingInput;
	int32 Priority = 0;
};

struct FPendingGlobalGameplayAbilityActivation
{
	FGlobalGameplayAbilityActivationRequest Request;
	bool bRequestSent = false;
};

/** World-scoped global gameplay event router and ability subscription registry. */
UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAutoBattleGlobalGameplayEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FGlobalGameplayAbilitySubscriptionId RegisterAbilitySubscription(
		UBattleAbilitySystem* AbilitySystem,
		FGameplayAbilitySpecHandle AbilityHandle,
		TSubclassOf<UBattleGameplayAbility> AbilityClass);

	bool UnregisterAbilitySubscription(FGlobalGameplayAbilitySubscriptionId SubscriptionId);
	void UnregisterAbilitySystem(const UBattleAbilitySystem* AbilitySystem);

	UFUNCTION(BlueprintCallable, Category = "Global Gameplay Event")
	bool BroadcastGlobalEvent(
		FAbilitySequenceId SequenceId,
		FAbilitySequenceNodeId SourceNodeId,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData,
		FGlobalGameplayEventDispatch& OutDispatch);

	UFUNCTION(BlueprintPure, Category = "Global Gameplay Event")
	int32 GetSubscriptionCount() const { return Subscriptions.Num(); }

	FGlobalGameplayAbilityActivationRequestedSignature& OnAbilityActivationRequested()
	{
		return AbilityActivationRequested;
	}

public:
	UPROPERTY(BlueprintAssignable, Category = "Global Gameplay Event")
	FGlobalGameplayEventBroadcastSignature OnGlobalEventBroadcast;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	UFUNCTION()
	void HandleSequenceNodeStateChanged(FAbilitySequenceId SequenceId, FAbilitySequenceNode Node);

	UFUNCTION()
	void HandleSequenceFinished(FAbilitySequenceId SequenceId, EAbilitySequenceState FinalState);

	FGlobalGameplayEventId AllocateEventId();
	FGlobalGameplayAbilitySubscriptionId AllocateSubscriptionId();
	TArray<FResolvedGlobalGameplayAbilitySubscription> ResolveSubscriptions(
		FGameplayTag EventTag,
		const FGameplayEventData& EventData);
	void RequestActivationForNode(FAbilitySequenceId SequenceId, FAbilitySequenceNodeId NodeId);
	void RemovePendingActivation(FAbilitySequenceId SequenceId, FAbilitySequenceNodeId NodeId);
	void InvalidatePendingActivations(FGlobalGameplayAbilitySubscriptionId SubscriptionId);

private:
	TWeakObjectPtr<UAutoBattleAbilitySequenceSubsystem> SequenceSubsystem;
	TMap<FGlobalGameplayAbilitySubscriptionId, FRegisteredGlobalGameplayAbilitySubscription> Subscriptions;
	TMap<FAbilitySequenceId, TMap<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>>
	PendingActivations;
	FGlobalGameplayAbilityActivationRequestedSignature AbilityActivationRequested;
	int64 NextEventValue = 1;
	int64 NextSubscriptionValue = 1;
};
