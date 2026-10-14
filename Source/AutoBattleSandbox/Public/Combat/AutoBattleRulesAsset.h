#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AutoBattleRulesAsset.generated.h"

UENUM(BlueprintType)
enum class EAutoBattleTargetRuleEffect : uint8
{
	Exclude,
	Priority
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAutoBattleAttackOrderRule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Order")
	FGameplayTagQuery UnitQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Order")
	int32 PriorityModifier = 0;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FAutoBattleTargetRule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Targeting")
	FGameplayTagQuery AttackerQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Targeting")
	FGameplayTagQuery TargetQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Targeting")
	EAutoBattleTargetRuleEffect Effect = EAutoBattleTargetRuleEffect::Priority;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Targeting")
	int32 Priority = 0;
};

UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAutoBattleRulesAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Order")
	TArray<FAutoBattleAttackOrderRule> AttackOrderRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Targeting")
	TArray<FAutoBattleTargetRule> TargetRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|State")
	FGameplayTagContainer PreventAttackTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|State")
	FGameplayTagContainer UntargetableTags;

	/** Minimum time between the starts of consecutive attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Battle|Timing", meta = (ClampMin = "0.0"))
	float MinimumAttackInterval = 0.5f;
};
