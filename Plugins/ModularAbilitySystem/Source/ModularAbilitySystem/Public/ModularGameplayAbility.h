// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameplayAbilityContext.h"
#include "Abilities/GameplayAbility.h"

#include "ModularGameplayAbility.generated.h"

class UModularAbilityTask;

UENUM(BlueprintType)
enum class EModularGameplayAbilityState : uint8
{
	Inactive,
	Active,
	Frozen
};

UENUM(BlueprintType)
enum class EModularGameplayAbilityActivationMode : uint8
{
	Activatable,
	Triggered,
	Passive
};

UENUM(BlueprintType)
enum class EModularGameplayAbilityCompletionPolicy : uint8
{
	EndAbility,
	KeepActive
};

enum class EModularGameplayAbilityContinuation : uint8
{
	None,
	CommitAfterPreActivationEvent
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityFrozen);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityResumed, const FModularGameplayAbilityContext&, AbilityContext);

/**
 * Base class for modular gameplay ability
 */
UCLASS()
class MODULARABILITYSYSTEM_API UModularGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// Begin of UGameplayAbility overrides
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void OnAvatarSet(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

	virtual void PreActivate(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate,
		const FGameplayEventData* TriggerEventData = nullptr) override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	//~End of UGameplayAbility overrides
	
	/**
	 * Freezes this activation without ending it. Ability-specific work, such as
	 * montages and timers, should be paused in OnAbilityFrozen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Freeze")
	bool FreezeAbility();

	/** Resumes a previously frozen activation. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Freeze")
	bool ResumeAbility();

	/** Returns this instance's modular execution state. */
	UFUNCTION(BlueprintPure, Category = "Ability|State")
	EModularGameplayAbilityState GetAbilityState() const { return AbilityState; }

	UFUNCTION(BlueprintPure, Category = "Ability|State")
	bool IsAbilityFrozen() const { return AbilityState == EModularGameplayAbilityState::Frozen; }

	UFUNCTION(BlueprintPure, Category = "Ability|Context")
	FModularGameplayAbilityContext GetAbilityContext() const { return AbilityContext; }

	/** Sets a controlled context payload that will be consumed by the next activation. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Context")
	bool TrySetInitialAbilityContext(const FModularGameplayAbilityContext& InitialAbilityContext);

	/** Updates the activation context before the ability commits. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Context")
	bool TryUpdateAbilityContext(const FModularGameplayAbilityContext& NewAbilityContext);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnAbilityFrozen OnModularAbilityFrozen;
	
	UPROPERTY(BlueprintAssignable)
	FOnAbilityResumed OnModularAbilityResumed;

protected:
	/** Called after the ability enters the Frozen state. */
	UFUNCTION(BlueprintNativeEvent, Category = "Ability|Freeze", meta = (DisplayName = "On Ability Frozen"))
	void OnAbilityFrozen();

	/** Called after the ability returns to the Active state. */
	UFUNCTION(BlueprintNativeEvent, Category = "Ability|Freeze", meta = (DisplayName = "On Ability Resumed"))
	void OnAbilityResumed();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Ability|State")
	EModularGameplayAbilityState AbilityState = EModularGameplayAbilityState::Inactive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Ability", meta = (AllowPrivateAccess))
	EModularGameplayAbilityActivationMode ActivationMode = EModularGameplayAbilityActivationMode::Activatable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Ability", meta = (AllowPrivateAccess))
	EModularGameplayAbilityCompletionPolicy CompletionPolicy = EModularGameplayAbilityCompletionPolicy::EndAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Events", meta = (AllowPrivateAccess))
	bool bHasPreActivationEvent = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Events", meta = (EditCondition = "bHasPreActivationEvent", AllowPrivateAccess))
	FGameplayTag PreActivationEventTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Events", meta = (AllowPrivateAccess))
	bool bHasPostActivationEvent = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Events", meta = (EditCondition = "bHasPostActivationEvent", AllowPrivateAccess))
	FGameplayTag PostActivationEventTag = FGameplayTag::EmptyTag;

	/**
	 * Ordered list of Modular Ability Tasks to be activated before activating ability itself. 
	 * Meant for prep work, such as target selection
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pre Activation Ability Tasks", meta = (AllowPrivateAccess))
	TArray<TSubclassOf<UModularAbilityTask>> PreActivationAbilityTasks;

	/**
	 * Ordered list of Modular Ability Tasks to be activated after pre activation has
	 * completed and the ability has committed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation Ability Tasks", meta = (AllowPrivateAccess))
	TArray<TSubclassOf<UModularAbilityTask>> ActivationAbilityTasks;
	
private:
	UFUNCTION()
	void HandleModularAbilityTaskComplete(FModularGameplayAbilityContext NewAbilityContext);

	UFUNCTION()
	void HandleModularAbilityTaskCanceled();

	void StartPreActivationTasks();
	void FinishPreActivationTasks();
	void ContinueAfterPreActivationEvent();
	void ContinuePendingActivation();
	void StartActivationTasks();
	void FinishActivationTasks();
	void StartTaskList(const TArray<TSubclassOf<UModularAbilityTask>>& AbilityTasks);
	bool StartNextTask();
	void ClearActiveTask(bool bEndTask = false);
	void InitializeAbilityContext(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayEventData* TriggerEventData);
	bool IsAbilityContextValid() const;
	void BroadcastGameplayEvent(FGameplayTag EventTag);
	void CancelAbilityActivation();

	FModularGameplayAbilityContext AbilityContext;
	FModularGameplayAbilityContext PendingInitialAbilityContext;

	UPROPERTY(Transient)
	TObjectPtr<UModularAbilityTask> ActiveModularAbilityTask = nullptr;

	const TArray<TSubclassOf<UModularAbilityTask>>* ActiveAbilityTaskList = nullptr;
	int32 ActiveAbilityTaskIndex = INDEX_NONE;
	bool bRunningPreActivationTasks = false;
	bool bAbilityCommitted = false;
	bool bHasPendingInitialAbilityContext = false;
	EModularGameplayAbilityContinuation PendingContinuation = EModularGameplayAbilityContinuation::None;
};