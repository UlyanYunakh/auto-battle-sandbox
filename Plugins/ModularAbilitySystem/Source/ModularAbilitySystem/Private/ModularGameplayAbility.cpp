// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "ModularAbilityTask.h"

bool UModularGameplayAbility::FreezeAbility()
{
	// Runtime state must never be stored on a non-instanced ability's CDO.
	if (!IsInstantiated() || !IsActive() || AbilityState != EModularGameplayAbilityState::Active)
	{
		return false;
	}

	AbilityState = EModularGameplayAbilityState::Frozen;
	
	OnAbilityFrozen();
	OnModularAbilityFrozen.Broadcast();
	
	return true;
}

bool UModularGameplayAbility::ResumeAbility()
{
	if (!IsInstantiated() || !IsActive() || AbilityState != EModularGameplayAbilityState::Frozen)
	{
		return false;
	}

	AbilityState = EModularGameplayAbilityState::Active;
	
	OnAbilityResumed();
	OnModularAbilityResumed.Broadcast(AbilityContext);
	ContinuePendingActivation();
	
	return true;
}

bool UModularGameplayAbility::TrySetInitialAbilityContext(const FModularGameplayAbilityContext& InitialAbilityContext)
{
	if (!IsInstantiated() || bAbilityCommitted)
	{
		return false;
	}

	PendingInitialAbilityContext = InitialAbilityContext;
	bHasPendingInitialAbilityContext = true;
	return true;
}

bool UModularGameplayAbility::TryUpdateAbilityContext(const FModularGameplayAbilityContext& NewAbilityContext)
{
	if (!IsInstantiated() || !IsActive() || bAbilityCommitted)
	{
		return false;
	}

	AbilityContext = NewAbilityContext;
	return true;
}

void UModularGameplayAbility::OnAbilityResumed_Implementation()
{
}

void UModularGameplayAbility::OnAbilityFrozen_Implementation()
{
}

bool UModularGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// InstancedPerActor calls this on its primary instance. InstancedPerExecution
	// calls it on the CDO, so inspect every live instance belonging to the spec.
	if (IsAbilityFrozen())
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystemComponent =
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec =
		AbilitySystemComponent ? AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;

	if (Spec)
	{
		for (const UGameplayAbility* Instance : Spec->GetAbilityInstances())
		{
			const UModularGameplayAbility* ModularInstance = Cast<UModularGameplayAbility>(Instance);
			if (ModularInstance && ModularInstance->IsAbilityFrozen())
			{
				return false;
			}
		}
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UModularGameplayAbility::OnAvatarSet(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (ActivationMode != EModularGameplayAbilityActivationMode::Passive)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->TryActivateAbility(Spec.Handle);
}

void UModularGameplayAbility::PreActivate(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate,
	const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);

	if (IsInstantiated())
	{
		AbilityState = EModularGameplayAbilityState::Active;
		InitializeAbilityContext(Handle, ActorInfo, TriggerEventData);
	}
}

void UModularGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!IsInstantiated())
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	bAbilityCommitted = false;
	PendingContinuation = EModularGameplayAbilityContinuation::None;
	InitializeAbilityContext(Handle, ActorInfo, TriggerEventData);
	StartPreActivationTasks();
}

void UModularGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	ClearActiveTask(true);
	ActiveAbilityTaskList = nullptr;
	ActiveAbilityTaskIndex = INDEX_NONE;
	bRunningPreActivationTasks = false;
	bAbilityCommitted = false;
	bHasPendingInitialAbilityContext = false;
	PendingContinuation = EModularGameplayAbilityContinuation::None;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// EndAbility can be deferred by a GAS scope lock. In that case IsActive()
	// remains true and the queued call will reset the state when it actually ends.
	if (!IsInstantiated() || !IsActive())
	{
		AbilityState = EModularGameplayAbilityState::Inactive;
	}
}

void UModularGameplayAbility::HandleModularAbilityTaskComplete(FModularGameplayAbilityContext NewAbilityContext)
{
	AbilityContext = NewAbilityContext;
	ClearActiveTask();

	if (StartNextTask())
	{
		return;
	}

	if (bRunningPreActivationTasks)
	{
		FinishPreActivationTasks();
		return;
	}

	FinishActivationTasks();
}

void UModularGameplayAbility::HandleModularAbilityTaskCanceled()
{
	CancelAbilityActivation();
}

void UModularGameplayAbility::StartPreActivationTasks()
{
	bRunningPreActivationTasks = true;
	StartTaskList(PreActivationAbilityTasks);
}

void UModularGameplayAbility::FinishPreActivationTasks()
{
	bRunningPreActivationTasks = false;
	ActiveAbilityTaskList = nullptr;
	ActiveAbilityTaskIndex = INDEX_NONE;

	if (bHasPreActivationEvent && PreActivationEventTag.IsValid())
	{
		BroadcastGameplayEvent(PreActivationEventTag);
	}

	if (IsAbilityFrozen())
	{
		PendingContinuation = EModularGameplayAbilityContinuation::CommitAfterPreActivationEvent;
		return;
	}

	ContinueAfterPreActivationEvent();
}

void UModularGameplayAbility::ContinueAfterPreActivationEvent()
{
	if (!IsAbilityContextValid())
	{
		CancelAbilityActivation();
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		CancelAbilityActivation();
		return;
	}

	bAbilityCommitted = true;
	StartActivationTasks();
}

void UModularGameplayAbility::ContinuePendingActivation()
{
	if (IsAbilityFrozen() || PendingContinuation == EModularGameplayAbilityContinuation::None)
	{
		return;
	}

	const EModularGameplayAbilityContinuation ContinuationToRun = PendingContinuation;
	PendingContinuation = EModularGameplayAbilityContinuation::None;

	switch (ContinuationToRun)
	{
	case EModularGameplayAbilityContinuation::CommitAfterPreActivationEvent:
		ContinueAfterPreActivationEvent();
		break;
	case EModularGameplayAbilityContinuation::None:
	default:
		break;
	}
}

void UModularGameplayAbility::StartActivationTasks()
{
	StartTaskList(ActivationAbilityTasks);
}

void UModularGameplayAbility::FinishActivationTasks()
{
	ActiveAbilityTaskList = nullptr;
	ActiveAbilityTaskIndex = INDEX_NONE;

	if (bHasPostActivationEvent && PostActivationEventTag.IsValid())
	{
		BroadcastGameplayEvent(PostActivationEventTag);
	}

	if (CompletionPolicy == EModularGameplayAbilityCompletionPolicy::EndAbility)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UModularGameplayAbility::StartTaskList(const TArray<TSubclassOf<UModularAbilityTask>>& AbilityTasks)
{
	ClearActiveTask(true);
	ActiveAbilityTaskList = &AbilityTasks;
	ActiveAbilityTaskIndex = INDEX_NONE;

	if (!StartNextTask())
	{
		if (bRunningPreActivationTasks)
		{
			FinishPreActivationTasks();
			return;
		}

		FinishActivationTasks();
	}
}

bool UModularGameplayAbility::StartNextTask()
{
	if (!ActiveAbilityTaskList)
	{
		return false;
	}

	++ActiveAbilityTaskIndex;
	if (!ActiveAbilityTaskList->IsValidIndex(ActiveAbilityTaskIndex))
	{
		return false;
	}

	UModularAbilityTask* NewTask = UModularAbilityTask::CreateModularAbilityTask(
		(*ActiveAbilityTaskList)[ActiveAbilityTaskIndex],
		this,
		AbilityContext);
	if (!NewTask)
	{
		CancelAbilityActivation();
		return true;
	}

	ActiveModularAbilityTask = NewTask;
	ActiveModularAbilityTask->OnModularAbilityTaskComplete.AddDynamic(this, &ThisClass::HandleModularAbilityTaskComplete);
	ActiveModularAbilityTask->OnModularAbilityTaskCanceled.AddDynamic(this, &ThisClass::HandleModularAbilityTaskCanceled);
	ActiveModularAbilityTask->ReadyForActivation();

	return true;
}

void UModularGameplayAbility::ClearActiveTask(const bool bEndTask)
{
	if (!ActiveModularAbilityTask)
	{
		return;
	}

	ActiveModularAbilityTask->OnModularAbilityTaskComplete.RemoveDynamic(this, &ThisClass::HandleModularAbilityTaskComplete);
	ActiveModularAbilityTask->OnModularAbilityTaskCanceled.RemoveDynamic(this, &ThisClass::HandleModularAbilityTaskCanceled);

	if (bEndTask)
	{
		ActiveModularAbilityTask->EndTask();
	}

	ActiveModularAbilityTask = nullptr;
}

void UModularGameplayAbility::InitializeAbilityContext(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayEventData* TriggerEventData)
{
	AbilityContext = bHasPendingInitialAbilityContext ? PendingInitialAbilityContext : FModularGameplayAbilityContext();
	bHasPendingInitialAbilityContext = false;
	AbilityContext.AbilityHandle = Handle;

	if (ActorInfo)
	{
		AbilityContext.Avatar = ActorInfo->AvatarActor.Get();
		AbilityContext.Owner = ActorInfo->OwnerActor.Get();
	}

	if (TriggerEventData)
	{
		AbilityContext.Target = TriggerEventData->Target.Get();
		AbilityContext.Instigator = TriggerEventData->Instigator.Get();
		AbilityContext.TargetData = TriggerEventData->TargetData;
	}
}

bool UModularGameplayAbility::IsAbilityContextValid() const
{
	return IsValid(AbilityContext.Target.Get()) || AbilityContext.TargetData.Num() > 0;
}

void UModularGameplayAbility::BroadcastGameplayEvent(const FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->HandleGameplayEvent(EventTag, nullptr);
}

void UModularGameplayAbility::CancelAbilityActivation()
{
	ClearActiveTask(true);
	ActiveAbilityTaskList = nullptr;
	ActiveAbilityTaskIndex = INDEX_NONE;
	bRunningPreActivationTasks = false;
	bAbilityCommitted = false;
	bHasPendingInitialAbilityContext = false;
	PendingContinuation = EModularGameplayAbilityContinuation::None;

	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}