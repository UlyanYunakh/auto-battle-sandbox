// Fill out your copyright notice in the Description page of Project Settings.

#include "ViewModels/UnitViewModel.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"
#include "AbilitySystem/AttributeSet/OffensiveAttributeSet.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "AbilitySystemComponent.h"
#include "Actors/UnitActor.h"
#include "DataAssets/UnitAsset.h"
#include "GameplayAbilitySpec.h"

namespace
{
	FText FormatStatText(const float Value)
	{
		return FText::AsNumber(Value);
	}

	TSubclassOf<UBattleGameplayAbility> FindFirstNonAttackAbility(
		const UBattleAbilitySystem& AbilitySystem,
		const TSubclassOf<UBattleGameplayAbility> BasicAttackAbility)
	{
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystem.GetActivatableAbilities())
		{
			if (const UBattleGameplayAbility* Ability = Cast<UBattleGameplayAbility>(AbilitySpec.Ability);
				Ability && Ability->GetClass() != BasicAttackAbility)
			{
				return Ability->GetClass();
			}
		}

		return nullptr;
	}
}

void UUnitViewModel::SetUnitActor(AUnitActor* InUnitActor)
{
	UnbindAttributeChangeDelegates();
	UnbindUnitInitializedDelegate();
	UnbindAbilityActivatedDelegate();

	UnitActor = InUnitActor;

	if (UnitActor == nullptr)
	{
		ClearUnit();
		return;
	}

	BindUnitInitializedDelegate();
	BindAbilityActivatedDelegate();
	RefreshFromUnitActor();
	BindAttributeChangeDelegates();
}

void UUnitViewModel::InitializeFromAsset(const UUnitAsset* InUnitAsset)
{
	UnbindAttributeChangeDelegates();
	UnbindUnitInitializedDelegate();
	UnbindAbilityActivatedDelegate();
	UnitActor = nullptr;

	if (InUnitAsset == nullptr)
	{
		ClearUnit();
		return;
	}

	SetUnitName(InUnitAsset->UnitName);
	SetUnitArt(InUnitAsset->UnitArt);
	SetAbilityPresentation(InUnitAsset->Abilities.IsValidIndex(0) ? InUnitAsset->Abilities[0] : nullptr);
	SetHealth(InUnitAsset->Health);
	SetAttack(InUnitAsset->Attack);
	SetArmor(InUnitAsset->Armor);
}

void UUnitViewModel::ViewModelInitialize(AActor* InActor)
{
	Super::ViewModelInitialize(InActor);

	if (AUnitActor* InUnitActor = Cast<AUnitActor>(InActor))
	{
		SetUnitActor(InUnitActor);
	}
}

void UUnitViewModel::ViewModelClear()
{
	UnbindAttributeChangeDelegates();
	UnbindUnitInitializedDelegate();
	UnbindAbilityActivatedDelegate();
	UnitActor = nullptr;

	Super::ViewModelClear();
}

void UUnitViewModel::SetupProperties(const FInstancedStruct& InData)
{
	Super::SetupProperties(InData);

	if (const FUnitViewModelProperties* UnitProperties = InData.GetPtr<FUnitViewModelProperties>())
	{
		InitializeFromAsset(UnitProperties->UnitAsset);
	}
}

bool UUnitViewModel::IsSupportedStruct(const UScriptStruct* InData) const
{
	return InData == FUnitViewModelProperties::StaticStruct();
}

void UUnitViewModel::SetUnitName(const FText& InUnitName)
{
	UE_MVVM_SET_PROPERTY_VALUE(UnitName, InUnitName);
}

void UUnitViewModel::SetUnitArt(const TSoftObjectPtr<UTexture2D>& InUnitArt)
{
	UE_MVVM_SET_PROPERTY_VALUE(UnitArt, InUnitArt);
}

void UUnitViewModel::SetAbilityPresentation(const TSubclassOf<UBattleGameplayAbility> InAbilityClass)
{
	const UBattleGameplayAbility* Ability = InAbilityClass
		                                        ? InAbilityClass->GetDefaultObject<UBattleGameplayAbility>()
		                                        : nullptr;
	SetAbilityIcon(Ability ? Ability->AbilityIcon : nullptr);
	SetAbilityText(Ability ? Ability->AbilityText : FText::GetEmpty());
}

void UUnitViewModel::SetAbilityIcon(const TSoftObjectPtr<UTexture2D>& InAbilityIcon)
{
	UE_MVVM_SET_PROPERTY_VALUE(AbilityIcon, InAbilityIcon);
}

void UUnitViewModel::SetAbilityText(const FText& InAbilityText)
{
	UE_MVVM_SET_PROPERTY_VALUE(AbilityText, InAbilityText);
}

void UUnitViewModel::AdvanceNonAttackAbilityActivationSerial()
{
	const int32 NextSerial = NonAttackAbilityActivationSerial == MAX_int32
		                         ? 1
		                         : NonAttackAbilityActivationSerial + 1;
	UE_MVVM_SET_PROPERTY_VALUE(NonAttackAbilityActivationSerial, NextSerial);
}

void UUnitViewModel::SetHealth(const float InHealth)
{
	UE_MVVM_SET_PROPERTY_VALUE(Health, InHealth);
	UE_MVVM_SET_PROPERTY_VALUE(HealthText, FormatStatText(Health));
}

void UUnitViewModel::SetAttack(const float InAttack)
{
	UE_MVVM_SET_PROPERTY_VALUE(Attack, InAttack);
	UE_MVVM_SET_PROPERTY_VALUE(AttackText, FormatStatText(Attack));
}

void UUnitViewModel::SetArmor(const float InArmor)
{
	UE_MVVM_SET_PROPERTY_VALUE(Armor, InArmor);
	UE_MVVM_SET_PROPERTY_VALUE(ArmorText, FormatStatText(Armor));
}

void UUnitViewModel::RefreshFromUnitActor()
{
	if (UnitActor == nullptr)
	{
		ClearUnit();
		return;
	}

	SetUnitName(UnitActor->GetUnitName());
	SetUnitArt(UnitActor->GetUnitArt());

	const UBattleAbilitySystem* UnitAbilitySystem = UnitActor->GetUnitAbilitySystem();
	if (UnitAbilitySystem == nullptr)
	{
		SetAbilityPresentation(nullptr);
		SetHealth(0.0f);
		SetAttack(0.0f);
		SetArmor(0.0f);
		return;
	}

	SetAbilityPresentation(FindFirstNonAttackAbility(*UnitAbilitySystem, UnitActor->GetBasicAttackAbility()));
	SetHealth(UnitAbilitySystem->GetNumericAttribute(UDefensiveAttributeSet::GetHealthAttribute()));
	SetAttack(UnitAbilitySystem->GetNumericAttribute(UOffensiveAttributeSet::GetAttackAttribute()));
	SetArmor(UnitAbilitySystem->GetNumericAttribute(UDefensiveAttributeSet::GetArmorAttribute()));
}

void UUnitViewModel::BindAttributeChangeDelegates()
{
	if (UnitActor == nullptr)
	{
		return;
	}

	UBattleAbilitySystem* UnitAbilitySystem = UnitActor->GetUnitAbilitySystem();
	if (UnitAbilitySystem == nullptr)
	{
		return;
	}

	HealthChangedHandle = UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UDefensiveAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	AttackChangedHandle = UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UOffensiveAttributeSet::GetAttackAttribute()).AddUObject(this, &ThisClass::HandleAttackChanged);
	ArmorChangedHandle = UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UDefensiveAttributeSet::GetArmorAttribute()).AddUObject(this, &ThisClass::HandleArmorChanged);
}

void UUnitViewModel::UnbindAttributeChangeDelegates()
{
	if (UnitActor == nullptr)
	{
		HealthChangedHandle.Reset();
		AttackChangedHandle.Reset();
		ArmorChangedHandle.Reset();
		return;
	}

	UBattleAbilitySystem* UnitAbilitySystem = UnitActor->GetUnitAbilitySystem();
	if (UnitAbilitySystem == nullptr)
	{
		HealthChangedHandle.Reset();
		AttackChangedHandle.Reset();
		ArmorChangedHandle.Reset();
		return;
	}

	UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UDefensiveAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UOffensiveAttributeSet::GetAttackAttribute()).Remove(AttackChangedHandle);
	UnitAbilitySystem->GetGameplayAttributeValueChangeDelegate(
		UDefensiveAttributeSet::GetArmorAttribute()).Remove(ArmorChangedHandle);

	HealthChangedHandle.Reset();
	AttackChangedHandle.Reset();
	ArmorChangedHandle.Reset();
}

void UUnitViewModel::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	SetHealth(Data.NewValue);
}

void UUnitViewModel::HandleAttackChanged(const FOnAttributeChangeData& Data)
{
	SetAttack(Data.NewValue);
}

void UUnitViewModel::HandleArmorChanged(const FOnAttributeChangeData& Data)
{
	SetArmor(Data.NewValue);
}

void UUnitViewModel::BindUnitInitializedDelegate()
{
	if (UnitActor != nullptr)
	{
		UnitActor->OnInitialized.AddUniqueDynamic(this, &ThisClass::HandleUnitInitialized);
	}
}

void UUnitViewModel::UnbindUnitInitializedDelegate()
{
	if (UnitActor != nullptr)
	{
		UnitActor->OnInitialized.RemoveDynamic(this, &ThisClass::HandleUnitInitialized);
	}
}

void UUnitViewModel::BindAbilityActivatedDelegate()
{
	if (UnitActor != nullptr)
	{
		UnitActor->OnNonAttackAbilityActivated.AddUniqueDynamic(
			this,
			&ThisClass::HandleNonAttackAbilityActivated);
	}
}

void UUnitViewModel::UnbindAbilityActivatedDelegate()
{
	if (UnitActor != nullptr)
	{
		UnitActor->OnNonAttackAbilityActivated.RemoveDynamic(
			this,
			&ThisClass::HandleNonAttackAbilityActivated);
	}
}

void UUnitViewModel::HandleUnitInitialized()
{
	RefreshFromUnitActor();
}

void UUnitViewModel::HandleNonAttackAbilityActivated(UBattleGameplayAbility* Ability)
{
	if (Ability == nullptr)
	{
		return;
	}

	SetAbilityPresentation(Ability->GetClass());
	AdvanceNonAttackAbilityActivationSerial();
	OnNonAttackAbilityActivated.Broadcast(Ability);
}

void UUnitViewModel::ClearUnit()
{
	SetUnitName(FText::GetEmpty());
	SetUnitArt(nullptr);
	SetAbilityPresentation(nullptr);
	SetHealth(0.0f);
	SetAttack(0.0f);
	SetArmor(0.0f);
}
