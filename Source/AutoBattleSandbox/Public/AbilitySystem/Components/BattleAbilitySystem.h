#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "AbilitySystemComponent.h"
#include "BattleAbilitySystem.generated.h"

class UAutoBattleAbilitySequenceSubsystem;
class UAutoBattleGlobalGameplayEventSubsystem;
class UBattleGameplayAbility;

struct FBattleAbilitySequenceActivation
{
	FAbilitySequenceId SequenceId;
	FAbilitySequenceNodeId NodeId;
	FGlobalGameplayEventId EventId;
};

enum class EBattleDeathSequencePhase : uint8
{
	None,
	BeforeEvent,
	AfterEvent,
	Complete
};

/** Adapts GAS ability lifecycle events to the project-level sequence and global-event systems. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AUTOBATTLESANDBOX_API UBattleAbilitySystem : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UBattleAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "Ability|Global Events")
	bool BroadcastGlobalGameplayEvent(
		FGameplayAbilitySpecHandle SourceAbilityHandle,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData,
		FGlobalGameplayEventDispatch& OutDispatch);

	UFUNCTION(BlueprintCallable, Category = "Ability|Targeting")
	bool TryActivateAbilityWithTarget(
		TSubclassOf<UBattleGameplayAbility> AbilityClass,
		AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Ability|Targeting")
	bool TryActivateAbilityWithTargetingInput(
		TSubclassOf<UBattleGameplayAbility> AbilityClass,
		const FAbilityTargetSelectionInput& TargetingInput);

	UFUNCTION(BlueprintCallable, Category = "Ability|Targeting")
	bool TryActivateAbilityWithTargetAndSequence(
		TSubclassOf<UBattleGameplayAbility> AbilityClass,
		AActor* Target,
		FAbilitySequenceId& OutSequenceId);

	bool GetSequenceActivation(
		FGameplayAbilitySpecHandle AbilityHandle,
		FAbilitySequenceId& OutSequenceId,
		FAbilitySequenceNodeId& OutNodeId) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void NotifyAbilityActivated(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(
		FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability,
		bool bWasCancelled) override;

private:
	void InitializeSequenceIntegration();
	void ShutdownSequenceIntegration();
	void RegisterAbilitySpec(const FGameplayAbilitySpec& AbilitySpec);
	void UnregisterAbilitySpec(FGameplayAbilitySpecHandle AbilityHandle);
	void HandleGlobalAbilityActivationRequested(const FGlobalGameplayAbilityActivationRequest& Request);
	UFUNCTION()
	void HandleSequenceNodeStateChanged(FAbilitySequenceId SequenceId, FAbilitySequenceNode Node);
	UFUNCTION()
	void HandleSequenceFinished(FAbilitySequenceId SequenceId, EAbilitySequenceState FinalState);
	bool CreateRootSequenceActivation(
		const UBattleGameplayAbility& Ability,
		FBattleAbilitySequenceActivation& OutActivation) const;
	void FinishSequenceActivation(
		const FBattleAbilitySequenceActivation& Activation,
		EAbilitySequenceNodeResult Result) const;
	void PushActivationContext(
		FGameplayAbilitySpecHandle AbilityHandle,
		const FBattleAbilityActivationContext& ActivationContext);
	bool ConsumeActivationContext(
		FGameplayAbilitySpecHandle AbilityHandle,
		FBattleAbilityActivationContext& OutActivationContext);
	void DiscardActivationContext(FGameplayAbilitySpecHandle AbilityHandle);
	void TryStartDeathSequence();
	void ContinueDeathSequence();
	bool BroadcastDeathEvent(FGameplayTag EventTag);
	void CompleteDeathSequence();

private:
	TWeakObjectPtr<UAutoBattleAbilitySequenceSubsystem> SequenceSubsystem;
	TWeakObjectPtr<UAutoBattleGlobalGameplayEventSubsystem> GlobalEventSubsystem;
	TMap<FGameplayAbilitySpecHandle, FGlobalGameplayAbilitySubscriptionId> GlobalEventSubscriptions;
	TMap<FGameplayAbilitySpecHandle, FBattleAbilitySequenceActivation> PendingSequenceActivations;
	TMap<FGameplayAbilitySpecHandle, FBattleAbilitySequenceActivation> ActiveSequenceActivations;
	TMap<FGameplayAbilitySpecHandle, FAbilitySequenceId> LastSequenceIds;
	TMap<FGameplayAbilitySpecHandle, TArray<FBattleAbilityActivationContext>> PendingActivationContexts;
	FDelegateHandle ActivationRequestDelegateHandle;
	FAbilitySequenceId DeathSequenceId;
	FAbilitySequenceNodeId DeathSequenceRootNodeId;
	EBattleDeathSequencePhase DeathSequencePhase = EBattleDeathSequencePhase::None;
	bool bDeathResolved = false;
};
