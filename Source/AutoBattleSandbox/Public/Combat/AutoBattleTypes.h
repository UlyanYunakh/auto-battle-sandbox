#pragma once

#include "CoreMinimal.h"
#include "AutoBattleTypes.generated.h"

UENUM(BlueprintType)
enum class EAutoBattleState : uint8
{
	Inactive,
	Preparing,
	SelectingAttacker,
	WaitingForSequence,
	ResolvingDefeats,
	Finished,
	Cancelled
};

UENUM(BlueprintType)
enum class EAutoBattleResult : uint8
{
	None,
	TeamA,
	TeamB,
	Draw,
	Cancelled
};
