#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"

#include "AbilitySystem/AttributeSet/DefensiveAttributeSet.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"

void FAbilityTargetSelectionStrategy::OrderCandidates(
	const FAbilityTargetingContext& Context,
	TArray<FAbilityTargetCandidate>& Candidates) const
{
	Candidates.StableSort([this, &Context](
		const FAbilityTargetCandidate& Left,
		const FAbilityTargetCandidate& Right)
		{
			return ScoreTarget(Context, Left) > ScoreTarget(Context, Right);
		});
}

bool FAbilityTargetTeamCriterion::IsTargetValid(
	const FAbilityTargetingContext& Context,
	const FAbilityTargetCandidate& Candidate) const
{
	const AUnitActor* SourceUnit = Cast<AUnitActor>(Context.SourceActor);
	const AUnitActor* CandidateUnit = Cast<AUnitActor>(Candidate.Actor);
	const AZoneActor* SourceZone = SourceUnit ? SourceUnit->GetOwningZone() : nullptr;
	const AZoneActor* CandidateZone = CandidateUnit ? CandidateUnit->GetOwningZone() : nullptr;
	if (!SourceZone || !CandidateZone)
	{
		return false;
	}

	const FGameplayTag SourceTeam = SourceZone->GetTeamTag();
	const FGameplayTag CandidateTeam = CandidateZone->GetTeamTag();
	if (!SourceTeam.IsValid() || !CandidateTeam.IsValid())
	{
		return false;
	}

	const bool bSameTeam = SourceTeam == CandidateTeam;
	return Relationship == EAbilityTargetTeamRelationship::Friendly
		       ? bSameTeam
		       : !bSameTeam;
}

bool FAbilityTargetSelfCriterion::IsTargetValid(
	const FAbilityTargetingContext& Context,
	const FAbilityTargetCandidate& Candidate) const
{
	return IsValid(Context.SourceActor) && Candidate.Actor == Context.SourceActor;
}

bool FAbilityTargetAdjacentUnitCriterion::IsTargetValid(
	const FAbilityTargetingContext& Context,
	const FAbilityTargetCandidate& Candidate) const
{
	const AUnitActor* SourceUnit = Cast<AUnitActor>(Context.SourceActor);
	const AUnitActor* CandidateUnit = Cast<AUnitActor>(Candidate.Actor);
	const AZoneActor* Zone = SourceUnit ? SourceUnit->GetOwningZone() : nullptr;
	if (!Zone || !CandidateUnit || CandidateUnit->GetOwningZone() != Zone)
	{
		return false;
	}

	const TArray<TObjectPtr<AUnitActor>>& Units = Zone->GetUnits();
	const int32 SourceIndex = Units.IndexOfByKey(SourceUnit);
	const int32 CandidateIndex = Units.IndexOfByKey(CandidateUnit);
	if (SourceIndex == INDEX_NONE || CandidateIndex == INDEX_NONE)
	{
		return false;
	}

	switch (Direction)
	{
	case EAbilityTargetAdjacentDirection::Left:
		return CandidateIndex == SourceIndex - 1;

	case EAbilityTargetAdjacentDirection::Right:
		return CandidateIndex == SourceIndex + 1;

	case EAbilityTargetAdjacentDirection::Both:
		return FMath::Abs(CandidateIndex - SourceIndex) == 1;

	default:
		return false;
	}
}

void FAbilityTargetRandomSelectionStrategy::OrderCandidates(
	const FAbilityTargetingContext&,
	TArray<FAbilityTargetCandidate>& Candidates) const
{
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}
}

float FAbilityTargetWeakestSelectionStrategy::ScoreTarget(
	const FAbilityTargetingContext&,
	const FAbilityTargetCandidate& Candidate) const
{
	const AUnitActor* Unit = Cast<AUnitActor>(Candidate.Actor);
	const UBattleAbilitySystem* AbilitySystem = Unit ? Unit->GetUnitAbilitySystem() : nullptr;
	if (!AbilitySystem)
	{
		return TNumericLimits<float>::Lowest();
	}

	return -AbilitySystem->GetNumericAttribute(UDefensiveAttributeSet::GetHealthAttribute());
}
