// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularAbilityTask.h"

UModularAbilityTask* UModularAbilityTask::CreateModularAbilityTask(
	TSubclassOf<UModularAbilityTask> AbilityTaskClass,
	UModularGameplayAbility* OwningAbility,
	FModularGameplayAbilityContext& AbilityContext)
{
	if (!OwningAbility || !AbilityTaskClass || AbilityTaskClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	UModularAbilityTask* Task = NewObject<UModularAbilityTask>(OwningAbility, AbilityTaskClass);
	if (!Task)
	{
		return nullptr;
	}

	Task->InitTask(*OwningAbility, OwningAbility->GetGameplayTaskDefaultPriority());
	UAbilityTask::DebugRecordAbilityTaskCreatedByAbility(OwningAbility);
	
	Task->OwningModularAbility = OwningAbility;
	Task->CachedAbilityContext = AbilityContext;

	return Task;
}

void UModularAbilityTask::Activate()
{
	Super::Activate();
	
	BindOnAbilityFrozenEvent();
	BindOnAbilityResumedEvent();
	
	if (!CanActivateTask() || !ActivateModularAbilityTask())
	{
		EndTask();
		
		OnModularAbilityTaskCanceled.Broadcast();
	}
}

void UModularAbilityTask::OnDestroy(bool bInOwnerFinished)
{
	if (OwningModularAbility.IsValid())
	{
		OwningModularAbility->OnModularAbilityFrozen.RemoveDynamic(this, &ThisClass::OnOwningAbilityFrozen);
		OwningModularAbility->OnModularAbilityResumed.RemoveDynamic(this, &ThisClass::OnOwningAbilityResumed);
	}

	Super::OnDestroy(bInOwnerFinished);
}

bool UModularAbilityTask::ActivateModularAbilityTask()
{
	EndTask();
	
	OnModularAbilityTaskComplete.Broadcast(CachedAbilityContext);
	
	return true;
}

void UModularAbilityTask::OnOwningAbilityFrozen()
{
	
}

void UModularAbilityTask::OnOwningAbilityResumed(const FModularGameplayAbilityContext& AbilityContext)
{
	CachedAbilityContext = AbilityContext;
}

void UModularAbilityTask::BindOnAbilityFrozenEvent()
{
	if (OwningModularAbility.IsValid())
	{
		OwningModularAbility->OnModularAbilityFrozen.AddDynamic(this, &ThisClass::OnOwningAbilityFrozen);
	}
}

void UModularAbilityTask::BindOnAbilityResumedEvent()
{
	if (OwningModularAbility.IsValid())
	{
		OwningModularAbility->OnModularAbilityResumed.AddDynamic(this, &ThisClass::OnOwningAbilityResumed);
	}
}