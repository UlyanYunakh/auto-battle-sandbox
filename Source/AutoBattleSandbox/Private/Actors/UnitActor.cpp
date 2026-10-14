// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/UnitActor.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"
#include "AbilitySystem/AttributeSet/OffensiveAttributeSet.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectWithAttackAnimation.h"
#include "AbilitySystem/Targeting/AbilityTargetingSubsystem.h"
#include "Actors/ZoneActor.h"
#include "DataAssets/UnitAsset.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"


AUnitActor::AUnitActor()
{
	PrimaryActorTick.bCanEverTick = true;

	UnitAbilitySystem = CreateDefaultSubobject<UBattleAbilitySystem>(TEXT("UnitAbilitySystem"));
	DefensiveAttributeSet = CreateDefaultSubobject<UDefensiveAttributeSet>(TEXT("DefensiveAttributeSet"));
	OffensiveAttributeSet = CreateDefaultSubobject<UOffensiveAttributeSet>(TEXT("OffensiveAttributeSet"));
}

void AUnitActor::InitializeUnit(const UUnitAsset* UnitDataAsset)
{
	if (!UnitDataAsset || !UnitAbilitySystem || !DefensiveAttributeSet || !OffensiveAttributeSet)
	{
		return;
	}

	if (HasAuthority())
	{
		RemoveGrantedUnitAbilities();
	}
	if (!AppliedInitialGameplayTags.IsEmpty())
	{
		UnitAbilitySystem->RemoveLooseGameplayTags(AppliedInitialGameplayTags);
		AppliedInitialGameplayTags.Reset();
	}

	UnitAbilitySystem->RemoveAllSpawnedAttributes();
	UnitAbilitySystem->InitAbilityActorInfo(this, this);
	UnitAbilitySystem->AddAttributeSetSubobject(DefensiveAttributeSet.Get());
	UnitAbilitySystem->AddAttributeSetSubobject(OffensiveAttributeSet.Get());

	UnitName = UnitDataAsset->UnitName;
	UnitArt = UnitDataAsset->UnitArt;
	BasicAttackAbility = UnitDataAsset->BasicAttackAbility;
	AppliedInitialGameplayTags = UnitDataAsset->InitialGameplayTags;
	UnitAbilitySystem->AddLooseGameplayTags(AppliedInitialGameplayTags);

	DefensiveAttributeSet->SetDefaultHealth(UnitDataAsset->Health);
	DefensiveAttributeSet->SetHealth(UnitDataAsset->Health);
	DefensiveAttributeSet->SetDefaultArmor(UnitDataAsset->Armor);
	DefensiveAttributeSet->SetArmor(UnitDataAsset->Armor);
	OffensiveAttributeSet->SetAttack(UnitDataAsset->Attack);

	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetDefaultHealthAttribute(),
	                                           UnitDataAsset->Health);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetHealthAttribute(), UnitDataAsset->Health);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetDefaultArmorAttribute(),
	                                           UnitDataAsset->Armor);
	UnitAbilitySystem->SetNumericAttributeBase(UDefensiveAttributeSet::GetArmorAttribute(), UnitDataAsset->Armor);
	UnitAbilitySystem->SetNumericAttributeBase(UOffensiveAttributeSet::GetAttackAttribute(), UnitDataAsset->Attack);

	if (HasAuthority())
	{
		GrantUnitAbilities(*UnitDataAsset);
	}

	OnInitialized.Broadcast();
}

UBattleAbilitySystem* AUnitActor::GetUnitAbilitySystem() const
{
	return UnitAbilitySystem;
}

FText AUnitActor::GetUnitName() const
{
	return UnitName;
}

TSoftObjectPtr<UTexture2D> AUnitActor::GetUnitArt() const
{
	return UnitArt;
}

bool AUnitActor::IsAlive() const
{
	return DefensiveAttributeSet && DefensiveAttributeSet->GetHealth() > 0.0f;
}

void AUnitActor::StartDeathAnimation()
{
	if (bDeathAnimationPlaying || IsActorBeingDestroyed())
	{
		return;
	}

	bDeathAnimationPlaying = true;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ThisClass::BeginDeathAnimationPlayback));
		return;
	}

	BeginDeathAnimationPlayback();
}

void AUnitActor::BeginDeathAnimationPlayback()
{
	if (!bDeathAnimationPlaying || IsActorBeingDestroyed())
	{
		return;
	}

	PlayDeathAnimation();
}

void AUnitActor::PlayDeathAnimation_Implementation()
{
	FinishDeathAnimation();
}

void AUnitActor::FinishDeathAnimation()
{
	if (IsActorBeingDestroyed())
	{
		return;
	}

	bDeathAnimationPlaying = false;
	if (OwningZone)
	{
		OwningZone->RemoveUnitInternal(this);
	}
	Destroy();
}

bool AUnitActor::StartAttackAnimation(
	UAbilityTask_ApplyGameplayEffectWithAttackAnimation* AttackTask,
	const FVector& TargetPosition)
{
	if (!IsValid(AttackTask) || ActiveAttackAnimationTask.IsValid() || IsActorBeingDestroyed())
	{
		return false;
	}

	ActiveAttackAnimationTask = AttackTask;
	PlayAttackAnimation(TargetPosition);
	return true;
}

void AUnitActor::PlayAttackAnimation_Implementation(const FVector)
{
	ApplyAttackEffect();
	FinishAttackAnimation();
}

void AUnitActor::ApplyAttackEffect()
{
	if (UAbilityTask_ApplyGameplayEffectWithAttackAnimation* AttackTask = ActiveAttackAnimationTask.Get())
	{
		AttackTask->ApplyEffectAtAnimationImpact();
	}
}

void AUnitActor::FinishAttackAnimation()
{
	UAbilityTask_ApplyGameplayEffectWithAttackAnimation* AttackTask = ActiveAttackAnimationTask.Get();
	ActiveAttackAnimationTask.Reset();
	if (AttackTask)
	{
		AttackTask->CompleteAttackAnimation();
	}
}

void AUnitActor::ClearAttackAnimationTask(
	const UAbilityTask_ApplyGameplayEffectWithAttackAnimation* AttackTask)
{
	if (ActiveAttackAnimationTask.Get() == AttackTask)
	{
		ActiveAttackAnimationTask.Reset();
	}
}

void AUnitActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UAbilityTargetingSubsystem* TargetingSubsystem = World->GetSubsystem<UAbilityTargetingSubsystem>())
		{
			TargetingSubsystem->RegisterTargetableActor(this);
		}
	}
}

void AUnitActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UAbilityTargetingSubsystem* TargetingSubsystem = World->GetSubsystem<UAbilityTargetingSubsystem>())
		{
			TargetingSubsystem->UnregisterTargetableActor(this);
		}
	}

	if (HasAuthority())
	{
		RemoveGrantedUnitAbilities();
	}
	if (UnitAbilitySystem && !AppliedInitialGameplayTags.IsEmpty())
	{
		UnitAbilitySystem->RemoveLooseGameplayTags(AppliedInitialGameplayTags);
		AppliedInitialGameplayTags.Reset();
	}
	OwningZone = nullptr;

	Super::EndPlay(EndPlayReason);
}

void AUnitActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AUnitActor::GrantUnitAbilities(const UUnitAsset& UnitDataAsset)
{
	if (!UnitAbilitySystem)
	{
		return;
	}

	TSet<TSubclassOf<UBattleGameplayAbility>> GrantedAbilityClasses;
	if (UnitDataAsset.BasicAttackAbility && !UnitDataAsset.BasicAttackAbility->HasAnyClassFlags(CLASS_Abstract))
	{
		GrantedAbilityClasses.Add(UnitDataAsset.BasicAttackAbility);
		const FGameplayAbilitySpecHandle AbilityHandle = UnitAbilitySystem->GiveAbility(
			FGameplayAbilitySpec(UnitDataAsset.BasicAttackAbility, 1, INDEX_NONE, this));
		if (AbilityHandle.IsValid())
		{
			GrantedAbilityHandles.Add(AbilityHandle);
		}
	}
	for (const TSubclassOf<UBattleGameplayAbility> AbilityClass : UnitDataAsset.Abilities)
	{
		if (!AbilityClass || AbilityClass->HasAnyClassFlags(CLASS_Abstract)
			|| GrantedAbilityClasses.Contains(AbilityClass))
		{
			continue;
		}

		GrantedAbilityClasses.Add(AbilityClass);
		const FGameplayAbilitySpecHandle AbilityHandle = UnitAbilitySystem->GiveAbility(
			FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
		if (AbilityHandle.IsValid())
		{
			GrantedAbilityHandles.Add(AbilityHandle);
		}
	}
}

void AUnitActor::RemoveGrantedUnitAbilities()
{
	if (UnitAbilitySystem)
	{
		for (const FGameplayAbilitySpecHandle AbilityHandle : GrantedAbilityHandles)
		{
			if (AbilityHandle.IsValid())
			{
				UnitAbilitySystem->ClearAbility(AbilityHandle);
			}
		}
	}

	GrantedAbilityHandles.Reset();
}
