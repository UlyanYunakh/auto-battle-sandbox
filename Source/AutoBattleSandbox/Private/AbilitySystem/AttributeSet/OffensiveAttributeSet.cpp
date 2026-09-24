// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AttributeSet/OffensiveAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UOffensiveAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UOffensiveAttributeSet, Attack, COND_None, REPNOTIFY_Always);
}

void UOffensiveAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetAttackAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UOffensiveAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetAttackAttribute())
	{
		SetAttack(FMath::Max(GetAttack(), 0.0f));
	}
}

void UOffensiveAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldAttack) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOffensiveAttributeSet, Attack, OldAttack);
}
