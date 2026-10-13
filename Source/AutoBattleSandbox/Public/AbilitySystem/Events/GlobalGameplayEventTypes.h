#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Sequences/AbilitySequenceTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "GlobalGameplayEventTypes.generated.h"

class UBattleAbilitySystem;
class UBattleGameplayAbility;

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventId
{
	GENERATED_BODY()

public:
	FGlobalGameplayEventId() = default;

	explicit FGlobalGameplayEventId(const int64 InValue)
		: Value(InValue)
	{
	}

	bool IsValid() const { return Value > 0; }
	bool operator==(const FGlobalGameplayEventId& Other) const { return Value == Other.Value; }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	int64 Value = 0;
};

FORCEINLINE uint32 GetTypeHash(const FGlobalGameplayEventId& EventId)
{
	return GetTypeHash(EventId.Value);
}

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayAbilitySubscriptionId
{
	GENERATED_BODY()

public:
	FGlobalGameplayAbilitySubscriptionId() = default;

	explicit FGlobalGameplayAbilitySubscriptionId(const int64 InValue)
		: Value(InValue)
	{
	}

	bool IsValid() const { return Value > 0; }
	bool operator==(const FGlobalGameplayAbilitySubscriptionId& Other) const { return Value == Other.Value; }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	int64 Value = 0;
};

FORCEINLINE uint32 GetTypeHash(const FGlobalGameplayAbilitySubscriptionId& SubscriptionId)
{
	return GetTypeHash(SubscriptionId.Value);
}

UENUM(BlueprintType)
enum class EGlobalGameplayEventMatchPolicy : uint8
{
	Exact,
	IncludeChildTags
};

/** Describes one global event that can trigger a granted battle ability. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayAbilityTrigger
{
	GENERATED_BODY()

public:
	bool IsValid() const { return EventTag.IsValid(); }
	bool MatchesEvent(FGameplayTag BroadcastEventTag) const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event")
	EGlobalGameplayEventMatchPolicy MatchPolicy = EGlobalGameplayEventMatchPolicy::Exact;

	/** Higher-priority responders execute before lower-priority responders. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event")
	int32 Priority = 0;
};

/** Observable summary of one global event routing operation. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventDispatch
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGlobalGameplayEventId EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGameplayTag EventTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilitySequenceId SequenceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilitySequenceNodeId SourceNodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	TArray<FGlobalGameplayAbilitySubscriptionId> MatchingSubscriptions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilitySequenceTransition SequenceTransition;
};

/** Exact activation work associated with a sequence node. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayAbilityActivationRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Global Gameplay Event")
	TObjectPtr<UBattleAbilitySystem> AbilitySystem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	TSubclassOf<UBattleGameplayAbility> AbilityClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGameplayAbilitySpecHandle AbilityHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGlobalGameplayAbilitySubscriptionId SubscriptionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGlobalGameplayEventId EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilitySequenceId SequenceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilitySequenceNodeId NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGameplayEventData EventData;
};
