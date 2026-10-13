#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"

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

void FAbilityTargetRandomSelectionStrategy::OrderCandidates(
	const FAbilityTargetingContext&,
	TArray<FAbilityTargetCandidate>& Candidates) const
{
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}
}
