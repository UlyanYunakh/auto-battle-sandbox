#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "AbilityTargetingTypes.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

UENUM(BlueprintType)
enum class EAbilityTargetSelectionMode : uint8
{
	Automatic,
	Manual,
	ManualWithAutomaticFallback
};

UENUM(BlueprintType)
enum class EAbilityTargetInputPolicy : uint8
{
	NormalDefinition,
	RequireProvidedTargets,
	PreferProvidedTargets
};

UENUM(BlueprintType)
enum class EAbilityTargetSelectionFailure : uint8
{
	None,
	MissingProvidedTargets,
	InvalidProvidedTargets,
	NoValidTargets,
	NotEnoughTargets,
	MissingAutomaticStrategy,
	NoManualSelectionProvider,
	ManualSelectionCancelled,
	InvalidDefinition
};

UENUM(BlueprintType)
enum class EAbilityTargetTeamRelationship : uint8
{
	Friendly,
	Opposing
};

UENUM(BlueprintType)
enum class EAbilityTargetAdjacentDirection : uint8
{
	Left,
	Right,
	Both
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilityTargetSelectionInput
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	EAbilityTargetInputPolicy Policy = EAbilityTargetInputPolicy::NormalDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	FGameplayAbilityTargetDataHandle ProvidedTargetData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	TArray<TObjectPtr<AActor>> ProvidedActors;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilityTargetingContext
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Ability Targeting")
	TObjectPtr<UGameplayAbility> Ability = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Ability Targeting")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Ability Targeting")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem = nullptr;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilityTargetCandidate
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Ability Targeting")
	TObjectPtr<AActor> Actor = nullptr;
};

USTRUCT(BlueprintType, meta = (Hidden))
struct AUTOBATTLESANDBOX_API FAbilityTargetCriterion
{
	GENERATED_BODY()

public:
	virtual ~FAbilityTargetCriterion() = default;

	virtual bool IsTargetValid(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const
	{
		return false;
	}
};

USTRUCT(BlueprintType, meta = (Hidden))
struct AUTOBATTLESANDBOX_API FAbilityTargetSelectionStrategy
{
	GENERATED_BODY()

public:
	virtual ~FAbilityTargetSelectionStrategy() = default;

	virtual void OrderCandidates(
		const FAbilityTargetingContext& Context,
		TArray<FAbilityTargetCandidate>& Candidates) const;

	virtual float ScoreTarget(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const
	{
		return 0.0f;
	}
};

USTRUCT(BlueprintType, meta = (DisplayName = "Team Relationship"))
struct AUTOBATTLESANDBOX_API FAbilityTargetTeamCriterion : public FAbilityTargetCriterion
{
	GENERATED_BODY()

public:
	virtual bool IsTargetValid(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	EAbilityTargetTeamRelationship Relationship = EAbilityTargetTeamRelationship::Opposing;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Self"))
struct AUTOBATTLESANDBOX_API FAbilityTargetSelfCriterion : public FAbilityTargetCriterion
{
	GENERATED_BODY()

public:
	virtual bool IsTargetValid(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const override;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Adjacent Unit"))
struct AUTOBATTLESANDBOX_API FAbilityTargetAdjacentUnitCriterion : public FAbilityTargetCriterion
{
	GENERATED_BODY()

public:
	virtual bool IsTargetValid(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Targeting")
	EAbilityTargetAdjacentDirection Direction = EAbilityTargetAdjacentDirection::Both;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Random"))
struct AUTOBATTLESANDBOX_API FAbilityTargetRandomSelectionStrategy : public FAbilityTargetSelectionStrategy
{
	GENERATED_BODY()

public:
	virtual void OrderCandidates(
		const FAbilityTargetingContext& Context,
		TArray<FAbilityTargetCandidate>& Candidates) const override;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Weakest"))
struct AUTOBATTLESANDBOX_API FAbilityTargetWeakestSelectionStrategy : public FAbilityTargetSelectionStrategy
{
	GENERATED_BODY()

public:
	virtual float ScoreTarget(
		const FAbilityTargetingContext& Context,
		const FAbilityTargetCandidate& Candidate) const override;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilityTargetSelectionResult
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	FGameplayAbilityTargetDataHandle TargetData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	EAbilityTargetSelectionFailure Failure = EAbilityTargetSelectionFailure::None;

	bool IsSuccessful() const
	{
		return Failure == EAbilityTargetSelectionFailure::None && TargetData.IsValid(0);
	}
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FBattleAbilityActivationContext
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	FAbilityTargetSelectionInput TargetingInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	bool bHasTargetingInput = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	FGameplayEventData EventData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Targeting")
	bool bHasEventData = false;
};
