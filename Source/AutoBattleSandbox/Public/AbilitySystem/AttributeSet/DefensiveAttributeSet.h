// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "DefensiveAttributeSet.generated.h"

#define AUTOBATTLESANDBOX_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectModCallbackData;

/**
 * Defensive attributes for actors
 */
UCLASS()
class AUTOBATTLESANDBOX_API UDefensiveAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	AUTOBATTLESANDBOX_ATTRIBUTE_ACCESSORS(UDefensiveAttributeSet, Health)
	AUTOBATTLESANDBOX_ATTRIBUTE_ACCESSORS(UDefensiveAttributeSet, DefaultHealth)
	AUTOBATTLESANDBOX_ATTRIBUTE_ACCESSORS(UDefensiveAttributeSet, Armor)
	AUTOBATTLESANDBOX_ATTRIBUTE_ACCESSORS(UDefensiveAttributeSet, DefaultArmor)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Defensive", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "Defensive", ReplicatedUsing = OnRep_DefaultHealth)
	FGameplayAttributeData DefaultHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Defensive", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;

	UPROPERTY(BlueprintReadOnly, Category = "Defensive", ReplicatedUsing = OnRep_DefaultArmor)
	FGameplayAttributeData DefaultArmor;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	void OnRep_DefaultHealth(const FGameplayAttributeData& OldDefaultHealth) const;

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldArmor) const;

	UFUNCTION()
	void OnRep_DefaultArmor(const FGameplayAttributeData& OldDefaultArmor) const;
};
