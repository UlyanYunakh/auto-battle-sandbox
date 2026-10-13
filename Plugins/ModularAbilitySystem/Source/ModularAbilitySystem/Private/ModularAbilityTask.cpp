// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularAbilityTask.h"

#include "ModularAbilityTaskDefinition.h"
#include "ModularGameplayAbility.h"

UModularAbilityTask* UModularAbilityTask::CreateModularAbilityTask(
	TSubclassOf<UModularAbilityTask> AbilityTaskClass,
	UModularGameplayAbility* OwningAbility,
	const FModularGameplayAbilityContext& AbilityContext)
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

	Task->InitializeModularTask(*OwningAbility, AbilityContext);

	return Task;
}

UModularAbilityTask* UModularAbilityTask::CreateConfiguredModularAbilityTask(
	const FModularAbilityTaskDefinition& TaskDefinition,
	UModularGameplayAbility* OwningAbility,
	const FModularGameplayAbilityContext& AbilityContext)
{
	if (!OwningAbility || !TaskDefinition.IsValid())
	{
		return nullptr;
	}

	UModularAbilityTask* Task = NewObject<UModularAbilityTask>(OwningAbility, TaskDefinition.TaskClass);
	if (!Task)
	{
		return nullptr;
	}

	Task->InitTask(*OwningAbility, OwningAbility->GetGameplayTaskDefaultPriority());
	UAbilityTask::DebugRecordAbilityTaskCreatedByAbility(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, AbilityContext);

	if (!Task->InitializeFromTaskParameters(TaskDefinition.Parameters))
	{
		Task->EndTask();
		return nullptr;
	}

	return Task;
}

UScriptStruct* UModularAbilityTask::GetTaskParametersStruct() const
{
	return FModularAbilityTaskParameters::StaticStruct();
}

void UModularAbilityTask::Activate()
{
	Super::Activate();
	if (bModularTaskFinished)
	{
		return;
	}

	BindOnAbilityFrozenEvent();
	BindOnAbilityResumedEvent();

	if (!CanActivateTask() || !ActivateModularAbilityTask())
	{
		CancelModularAbilityTask();
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
	CompleteModularAbilityTask(CachedAbilityContext);
	return true;
}

bool UModularAbilityTask::CanActivateTask() const
{
	return OwningModularAbility.IsValid();
}

bool UModularAbilityTask::InitializeFromTaskParameters(const FInstancedStruct& Parameters)
{
	return Parameters.GetScriptStruct() == GetTaskParametersStruct();
}

void UModularAbilityTask::InitializeModularTask(
	UModularGameplayAbility& InOwningAbility,
	const FModularGameplayAbilityContext& AbilityContext)
{
	OwningModularAbility = &InOwningAbility;
	CachedAbilityContext = AbilityContext;
	bModularTaskFinished = false;
}

void UModularAbilityTask::CompleteModularAbilityTask(
	const FModularGameplayAbilityContext& NewAbilityContext)
{
	if (bModularTaskFinished)
	{
		return;
	}

	bModularTaskFinished = true;
	CachedAbilityContext = NewAbilityContext;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnModularAbilityTaskComplete.Broadcast(CachedAbilityContext);
	}
	EndTask();
}

void UModularAbilityTask::CancelModularAbilityTask()
{
	if (bModularTaskFinished)
	{
		return;
	}

	bModularTaskFinished = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnModularAbilityTaskCanceled.Broadcast();
	}
	EndTask();
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
		OwningModularAbility->OnModularAbilityFrozen.AddUniqueDynamic(this, &ThisClass::OnOwningAbilityFrozen);
	}
}

void UModularAbilityTask::BindOnAbilityResumedEvent()
{
	if (OwningModularAbility.IsValid())
	{
		OwningModularAbility->OnModularAbilityResumed.AddUniqueDynamic(this, &ThisClass::OnOwningAbilityResumed);
	}
}
