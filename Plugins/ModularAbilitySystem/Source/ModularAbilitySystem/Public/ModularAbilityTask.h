// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ModularAbilityTask.generated.h"

/**
 * Base class for Modular ability tasks
 */
UCLASS()
class MODULARABILITYSYSTEM_API UModularAbilityTask : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ModularAbility|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", DeterminesOutputType = "AbilityTaskClass"))
	static UModularAbilityTask* CreateModularAbilityTask(
		TSubclassOf<UModularAbilityTask> AbilityTaskClass,
		UModularGameplayAbility* OwningAbility,
		FModularGameplayAbilityContext& AbilityContext);

	// Begin of UAbilityTask override
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~End of UAbilityTask override

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModularAbilityTaskCanceled);
	UPROPERTY(BlueprintAssignable)
	FOnModularAbilityTaskCanceled OnModularAbilityTaskCanceled;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModularAbilityTaskComplete, FModularGameplayAbilityContext, NewAbilityContext);
	UPROPERTY(BlueprintAssignable)
	FOnModularAbilityTaskComplete OnModularAbilityTaskComplete;

protected:
	// Begin of UAbilityTask override
	virtual void Activate() override;
	//~End of UAbilityTask override
	
	virtual bool ActivateModularAbilityTask();
	virtual bool CanActivateTask() const;
	
	UFUNCTION()
	virtual void OnOwningAbilityFrozen();

	UFUNCTION()
	virtual void OnOwningAbilityResumed(const FModularGameplayAbilityContext& AbilityContext);
	
private:
	void BindOnAbilityFrozenEvent();
	void BindOnAbilityResumedEvent();
	
protected:
	TWeakObjectPtr<UModularGameplayAbility> OwningModularAbility = nullptr;
	FModularGameplayAbilityContext CachedAbilityContext;
};
