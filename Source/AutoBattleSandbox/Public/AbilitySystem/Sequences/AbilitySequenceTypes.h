#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySequenceTypes.generated.h"

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceId
{
	GENERATED_BODY()

public:
	FAbilitySequenceId() = default;

	explicit FAbilitySequenceId(const int64 InValue)
		: Value(InValue)
	{
	}

	bool IsValid() const { return Value > 0; }

	bool operator==(const FAbilitySequenceId& Other) const { return Value == Other.Value; }
	bool operator!=(const FAbilitySequenceId& Other) const { return !(*this == Other); }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	int64 Value = 0;
};

FORCEINLINE uint32 GetTypeHash(const FAbilitySequenceId& SequenceId)
{
	return GetTypeHash(SequenceId.Value);
}

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceNodeId
{
	GENERATED_BODY()

public:
	FAbilitySequenceNodeId() = default;

	explicit FAbilitySequenceNodeId(const int32 InValue)
		: Value(InValue)
	{
	}

	bool IsValid() const { return Value > 0; }

	bool operator==(const FAbilitySequenceNodeId& Other) const { return Value == Other.Value; }
	bool operator!=(const FAbilitySequenceNodeId& Other) const { return !(*this == Other); }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	int32 Value = 0;
};

FORCEINLINE uint32 GetTypeHash(const FAbilitySequenceNodeId& NodeId)
{
	return GetTypeHash(NodeId.Value);
}

UENUM(BlueprintType)
enum class EAbilitySequenceState : uint8
{
	Inactive,
	Running,
	Finished,
	Cancelled
};

UENUM(BlueprintType)
enum class EAbilitySequenceNodeState : uint8
{
	Queued,
	Activating,
	Active,
	Suspended,
	Finished
};

UENUM(BlueprintType)
enum class EAbilitySequenceNodeResult : uint8
{
	None,
	Succeeded,
	Cancelled,
	ActivationFailed
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceNodeRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Sequence")
	FName DebugName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Sequence")
	FGameplayTag TriggerEvent;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceNode
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceNodeId NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceNodeId ParentNodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	TArray<FAbilitySequenceNodeId> Children;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FName DebugName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FGameplayTag TriggerEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	EAbilitySequenceNodeState State = EAbilitySequenceNodeState::Queued;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	EAbilitySequenceNodeResult Result = EAbilitySequenceNodeResult::None;
};

/** Describes the external action produced by a scheduler transition. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceTransition
{
	GENERATED_BODY()

public:
	bool HasNodeToActivate() const { return NodeToActivate.IsValid(); }
	bool HasNodeToResume() const { return NodeToResume.IsValid(); }

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	TArray<FAbilitySequenceNodeId> AddedNodeIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceNodeId NodeToActivate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceNodeId NodeToResume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	bool bSequenceFinished = false;
};

/** Read-only tree state suitable for UI, debugging, and later replication. */
USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAbilitySequenceSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceId SequenceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	FAbilitySequenceNodeId RootNodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	EAbilitySequenceState State = EAbilitySequenceState::Inactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	TArray<FAbilitySequenceNode> Nodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability Sequence")
	TArray<FAbilitySequenceNodeId> ActivePath;
};
