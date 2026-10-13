#include "AbilitySystem/Targeting/AbilityTargetingSubsystem.h"

#include "AbilitySystem/Targeting/AbilityTargetingDefinition.h"
#include "Engine/World.h"

void UAbilityTargetingSubsystem::Deinitialize()
{
	TargetableActors.Reset();
	Super::Deinitialize();
}

void UAbilityTargetingSubsystem::RegisterTargetableActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		TargetableActors.Add(Actor);
	}
}

void UAbilityTargetingSubsystem::UnregisterTargetableActor(const AActor* Actor)
{
	if (Actor)
	{
		TargetableActors.Remove(Actor);
	}
}

bool UAbilityTargetingSubsystem::ValidateTargetData(
	const FAbilityTargetingContext& Context,
	const UAbilityTargetingDefinition& Definition,
	const FGameplayAbilityTargetDataHandle& TargetData,
	EAbilityTargetSelectionFailure& OutFailure) const
{
	OutFailure = EAbilityTargetSelectionFailure::None;
	EAbilityTargetSelectionFailure DefinitionFailure;
	if (!Definition.IsDefinitionValid(false, DefinitionFailure))
	{
		OutFailure = DefinitionFailure;
		return false;
	}

	TArray<AActor*> Actors;
	GetUniqueActors(TargetData, Actors);
	if (Actors.IsEmpty())
	{
		OutFailure = EAbilityTargetSelectionFailure::MissingProvidedTargets;
		return false;
	}

	if (Actors.Num() < Definition.MinimumTargets)
	{
		OutFailure = EAbilityTargetSelectionFailure::NotEnoughTargets;
		return false;
	}

	if (Actors.Num() > Definition.MaximumTargets)
	{
		OutFailure = EAbilityTargetSelectionFailure::InvalidProvidedTargets;
		return false;
	}

	for (AActor* Actor : Actors)
	{
		if (!IsCandidateValid(Context, Definition, Actor))
		{
			OutFailure = EAbilityTargetSelectionFailure::InvalidProvidedTargets;
			return false;
		}
	}

	return true;
}

void UAbilityTargetingSubsystem::FindValidCandidates(
	const FAbilityTargetingContext& Context,
	const UAbilityTargetingDefinition& Definition,
	TArray<FAbilityTargetCandidate>& OutCandidates) const
{
	OutCandidates.Reset();
	for (const TWeakObjectPtr<AActor>& TargetableActor : TargetableActors)
	{
		AActor* Actor = TargetableActor.Get();
		if (IsCandidateValid(Context, Definition, Actor))
		{
			FAbilityTargetCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
			Candidate.Actor = Actor;
		}
	}

	OutCandidates.Sort([](const FAbilityTargetCandidate& Left, const FAbilityTargetCandidate& Right)
	{
		return GetPathNameSafe(Left.Actor) < GetPathNameSafe(Right.Actor);
	});
}

FAbilityTargetSelectionResult UAbilityTargetingSubsystem::SelectAutomatically(
	const FAbilityTargetingContext& Context,
	const UAbilityTargetingDefinition& Definition) const
{
	FAbilityTargetSelectionResult Result;
	EAbilityTargetSelectionFailure DefinitionFailure;
	if (!Definition.IsDefinitionValid(true, DefinitionFailure))
	{
		Result.Failure = DefinitionFailure;
		return Result;
	}

	TArray<FAbilityTargetCandidate> Candidates;
	FindValidCandidates(Context, Definition, Candidates);
	if (Candidates.Num() < Definition.MinimumTargets)
	{
		Result.Failure = Candidates.IsEmpty()
			                 ? EAbilityTargetSelectionFailure::NoValidTargets
			                 : EAbilityTargetSelectionFailure::NotEnoughTargets;
		return Result;
	}

	const FAbilityTargetSelectionStrategy& Strategy = Definition.AutomaticStrategy.Get();
	Strategy.OrderCandidates(Context, Candidates);

	FGameplayAbilityTargetData_ActorArray* ActorData = new FGameplayAbilityTargetData_ActorArray();
	const int32 TargetCount = FMath::Min(Definition.MaximumTargets, Candidates.Num());
	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		ActorData->TargetActorArray.Add(Candidates[Index].Actor);
	}

	Result.TargetData.Add(ActorData);
	return Result;
}

FGameplayAbilityTargetDataHandle UAbilityTargetingSubsystem::MakeTargetData(
	const FGameplayAbilityTargetDataHandle& ExistingTargetData,
	const TArray<TObjectPtr<AActor>>& Actors) const
{
	FGameplayAbilityTargetDataHandle Result = ExistingTargetData;
	TArray<AActor*> UniqueActors;
	GetUniqueActors(Result, UniqueActors);

	FGameplayAbilityTargetData_ActorArray* ActorData = nullptr;
	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor) || UniqueActors.Contains(Actor))
		{
			continue;
		}

		if (!ActorData)
		{
			ActorData = new FGameplayAbilityTargetData_ActorArray();
		}

		ActorData->TargetActorArray.Add(Actor);
		UniqueActors.Add(Actor);
	}

	if (ActorData)
	{
		Result.Add(ActorData);
	}

	return Result;
}

bool UAbilityTargetingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

bool UAbilityTargetingSubsystem::IsCandidateValid(
	const FAbilityTargetingContext& Context,
	const UAbilityTargetingDefinition& Definition,
	AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FAbilityTargetCandidate Candidate;
	Candidate.Actor = Actor;
	for (const TInstancedStruct<FAbilityTargetCriterion>& Criterion : Definition.Criteria)
	{
		if (!Criterion.IsValid()
			|| Criterion.GetScriptStruct() == FAbilityTargetCriterion::StaticStruct()
			|| !Criterion.Get().IsTargetValid(Context, Candidate))
		{
			return false;
		}
	}

	return true;
}

void UAbilityTargetingSubsystem::GetUniqueActors(
	const FGameplayAbilityTargetDataHandle& TargetData,
	TArray<AActor*>& OutActors)
{
	OutActors.Reset();
	for (int32 DataIndex = 0; DataIndex < TargetData.Num(); ++DataIndex)
	{
		const FGameplayAbilityTargetData* Data = TargetData.Get(DataIndex);
		if (!Data)
		{
			continue;
		}

		for (const TWeakObjectPtr<AActor>& Actor : Data->GetActors())
		{
			if (Actor.IsValid())
			{
				OutActors.AddUnique(Actor.Get());
			}
		}
	}
}
