// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "OffensiveAttributeSet.generated.h"

#define AUTOBATTLESANDBOX_OFFENSIVE_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectModCallbackData;

/**
 * Offensive attributes for actors
 */
UCLASS()
class AUTOBATTLESANDBOX_API UOffensiveAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	AUTOBATTLESANDBOX_OFFENSIVE_ATTRIBUTE_ACCESSORS(UOffensiveAttributeSet, Attack)

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Offensive", ReplicatedUsing = OnRep_Attack)
	FGameplayAttributeData Attack;

protected:
	UFUNCTION()
	void OnRep_Attack(const FGameplayAttributeData& OldAttack) const;
};
