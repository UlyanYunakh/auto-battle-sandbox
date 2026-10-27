#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Sequences/AbilitySequenceTypes.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "GlobalGameplayEventTypes.generated.h"

class AActor;
class UBattleAbilitySystem;
class UBattleGameplayAbility;

struct FGlobalGameplayEventFilterContext
{
	const AActor* ListeningActor = nullptr;
	const FGameplayEventData* EventData = nullptr;
};

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

UENUM(BlueprintType)
enum class EGlobalGameplayEventParticipant : uint8
{
	Instigator,
	Target
};

UENUM(BlueprintType)
enum class EGlobalGameplayEventTeamRelationship : uint8
{
	Friendly,
	Opposing
};

USTRUCT(BlueprintType, meta = (Hidden))
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventFilter
{
	GENERATED_BODY()

public:
	virtual ~FGlobalGameplayEventFilter() = default;

	virtual bool Matches(const FGlobalGameplayEventFilterContext& Context) const
	{
		return false;
	}
};

/** Matches an event participant's team relative to the ability owner receiving the event. */
USTRUCT(BlueprintType, meta = (DisplayName = "Team Relationship"))
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventTeamFilter : public FGlobalGameplayEventFilter
{
	GENERATED_BODY()

public:
	virtual bool Matches(const FGlobalGameplayEventFilterContext& Context) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Gameplay Event")
	EGlobalGameplayEventParticipant Participant = EGlobalGameplayEventParticipant::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Gameplay Event")
	EGlobalGameplayEventTeamRelationship Relationship = EGlobalGameplayEventTeamRelationship::Friendly;
};

/** Matches when the selected event participant owns the ability receiving the event. */
USTRUCT(BlueprintType, meta = (DisplayName = "Self"))
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventSelfFilter : public FGlobalGameplayEventFilter
{
	GENERATED_BODY()

public:
	virtual bool Matches(const FGlobalGameplayEventFilterContext& Context) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Gameplay Event")
	EGlobalGameplayEventParticipant Participant = EGlobalGameplayEventParticipant::Instigator;
};

USTRUCT(BlueprintType, meta = (Hidden))
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventTargetResolver
{
	GENERATED_BODY()

public:
	virtual ~FGlobalGameplayEventTargetResolver() = default;

	virtual bool Resolve(
		const FGlobalGameplayEventFilterContext& Context,
		FAbilityTargetSelectionInput& OutTargetingInput) const
	{
		return false;
	}
};

/** Uses the event instigator or event target as the triggered ability's provided target. */
USTRUCT(BlueprintType, meta = (DisplayName = "Event Participant"))
struct AUTOBATTLESANDBOX_API FGlobalGameplayEventParticipantTargetResolver
	: public FGlobalGameplayEventTargetResolver
{
	GENERATED_BODY()

public:
	virtual bool Resolve(
		const FGlobalGameplayEventFilterContext& Context,
		FAbilityTargetSelectionInput& OutTargetingInput) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Gameplay Event")
	EGlobalGameplayEventParticipant Participant = EGlobalGameplayEventParticipant::Target;
};

/** Describes one global event that can trigger a granted battle ability. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FGlobalGameplayAbilityTrigger
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool AreFiltersValid() const;
	bool IsTargetResolverValid() const;
	bool ResolveTargetingInput(
		const FGlobalGameplayEventFilterContext& Context,
		FAbilityTargetSelectionInput& OutTargetingInput) const;
	bool MatchesEvent(FGameplayTag BroadcastEventTag) const;
	bool MatchesEvent(
		FGameplayTag BroadcastEventTag,
		const FGlobalGameplayEventFilterContext& FilterContext) const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event")
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event")
	EGlobalGameplayEventMatchPolicy MatchPolicy = EGlobalGameplayEventMatchPolicy::Exact;

	/** Every configured filter must match. Use multiple trigger entries to express alternatives. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FGlobalGameplayEventFilter>> Filters;

	/** Chooses what the triggered ability targets. If unset, the event target is used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Global Gameplay Event", meta = (ExcludeBaseStruct))
	TInstancedStruct<FGlobalGameplayEventTargetResolver> TargetResolver;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Global Gameplay Event")
	FAbilityTargetSelectionInput TargetingInput;
};
