// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UDefensiveAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDefensiveAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDefensiveAttributeSet, DefaultHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDefensiveAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDefensiveAttributeSet, DefaultArmor, COND_None, REPNOTIFY_Always);
}

void UDefensiveAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute() || Attribute == GetDefaultHealthAttribute() ||
		Attribute == GetArmorAttribute() || Attribute == GetDefaultArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UDefensiveAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Max(GetHealth(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetDefaultHealthAttribute())
	{
		SetDefaultHealth(FMath::Max(GetDefaultHealth(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetArmorAttribute())
	{
		SetArmor(FMath::Max(GetArmor(), 0.0f));
	}
	else if (Data.EvaluatedData.Attribute == GetDefaultArmorAttribute())
	{
		SetDefaultArmor(FMath::Max(GetDefaultArmor(), 0.0f));
	}
}

void UDefensiveAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDefensiveAttributeSet, Health, OldHealth);
}

void UDefensiveAttributeSet::OnRep_DefaultHealth(const FGameplayAttributeData& OldDefaultHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDefensiveAttributeSet, DefaultHealth, OldDefaultHealth);
}

void UDefensiveAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDefensiveAttributeSet, Armor, OldArmor);
}

void UDefensiveAttributeSet::OnRep_DefaultArmor(const FGameplayAttributeData& OldDefaultArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDefensiveAttributeSet, DefaultArmor, OldDefaultArmor);
}
