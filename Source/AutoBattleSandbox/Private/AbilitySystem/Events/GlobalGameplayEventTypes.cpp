#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"

#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"

bool FGlobalGameplayEventParticipantTargetResolver::Resolve(
	const FGlobalGameplayEventFilterContext& Context,
	FAbilityTargetSelectionInput& OutTargetingInput) const
{
	OutTargetingInput = FAbilityTargetSelectionInput();
	if (!Context.EventData)
	{
		return false;
	}

	AActor* ParticipantActor = Participant == EGlobalGameplayEventParticipant::Instigator
		                           ? const_cast<AActor*>(Context.EventData->Instigator.Get())
		                           : const_cast<AActor*>(Context.EventData->Target.Get());
	if (!IsValid(ParticipantActor))
	{
		return false;
	}

	OutTargetingInput.ProvidedActors.Add(ParticipantActor);
	return true;
}

bool FGlobalGameplayEventTeamFilter::Matches(const FGlobalGameplayEventFilterContext& Context) const
{
	if (!Context.ListeningActor || !Context.EventData)
	{
		return false;
	}

	const AActor* ParticipantActor = Participant == EGlobalGameplayEventParticipant::Instigator
		                                 ? Context.EventData->Instigator.Get()
		                                 : Context.EventData->Target.Get();
	const AUnitActor* ListeningUnit = Cast<AUnitActor>(Context.ListeningActor);
	const AUnitActor* ParticipantUnit = Cast<AUnitActor>(ParticipantActor);
	const AZoneActor* ListeningZone = ListeningUnit ? ListeningUnit->GetOwningZone() : nullptr;
	const AZoneActor* ParticipantZone = ParticipantUnit ? ParticipantUnit->GetOwningZone() : nullptr;
	if (!ListeningZone || !ParticipantZone)
	{
		return false;
	}

	const FGameplayTag ListeningTeam = ListeningZone->GetTeamTag();
	const FGameplayTag ParticipantTeam = ParticipantZone->GetTeamTag();
	if (!ListeningTeam.IsValid() || !ParticipantTeam.IsValid())
	{
		return false;
	}

	const bool bIsFriendly = ListeningTeam == ParticipantTeam;
	return Relationship == EGlobalGameplayEventTeamRelationship::Friendly
		       ? bIsFriendly
		       : !bIsFriendly;
}

bool FGlobalGameplayEventSelfFilter::Matches(const FGlobalGameplayEventFilterContext& Context) const
{
	if (!IsValid(Context.ListeningActor) || !Context.EventData)
	{
		return false;
	}

	const AActor* ParticipantActor = Participant == EGlobalGameplayEventParticipant::Instigator
		                                 ? Context.EventData->Instigator.Get()
		                                 : Context.EventData->Target.Get();
	return IsValid(ParticipantActor) && ParticipantActor == Context.ListeningActor;
}

bool FGlobalGameplayAbilityTrigger::IsValid() const
{
	return EventTag.IsValid() && AreFiltersValid() && IsTargetResolverValid();
}

bool FGlobalGameplayAbilityTrigger::AreFiltersValid() const
{
	for (const TInstancedStruct<FGlobalGameplayEventFilter>& Filter : Filters)
	{
		if (!Filter.IsValid() || Filter.GetScriptStruct() == FGlobalGameplayEventFilter::StaticStruct())
		{
			return false;
		}
	}

	return true;
}

bool FGlobalGameplayAbilityTrigger::IsTargetResolverValid() const
{
	return !TargetResolver.IsValid()
		|| TargetResolver.GetScriptStruct() != FGlobalGameplayEventTargetResolver::StaticStruct();
}

bool FGlobalGameplayAbilityTrigger::ResolveTargetingInput(
	const FGlobalGameplayEventFilterContext& Context,
	FAbilityTargetSelectionInput& OutTargetingInput) const
{
	if (TargetResolver.IsValid())
	{
		return TargetResolver.Get().Resolve(Context, OutTargetingInput);
	}

	const FGlobalGameplayEventParticipantTargetResolver DefaultResolver;
	return DefaultResolver.Resolve(Context, OutTargetingInput);
}

bool FGlobalGameplayAbilityTrigger::MatchesEvent(const FGameplayTag BroadcastEventTag) const
{
	return MatchesEvent(BroadcastEventTag, FGlobalGameplayEventFilterContext());
}

bool FGlobalGameplayAbilityTrigger::MatchesEvent(
	const FGameplayTag BroadcastEventTag,
	const FGlobalGameplayEventFilterContext& FilterContext) const
{
	if (!IsValid() || !BroadcastEventTag.IsValid())
	{
		return false;
	}

	bool bMatchesTag;
	switch (MatchPolicy)
	{
	case EGlobalGameplayEventMatchPolicy::Exact:
		bMatchesTag = BroadcastEventTag.MatchesTagExact(EventTag);
		break;

	case EGlobalGameplayEventMatchPolicy::IncludeChildTags:
		bMatchesTag = BroadcastEventTag.MatchesTag(EventTag);
		break;

	default:
		return false;
	}

	if (!bMatchesTag)
	{
		return false;
	}

	for (const TInstancedStruct<FGlobalGameplayEventFilter>& Filter : Filters)
	{
		if (!Filter.Get().Matches(FilterContext))
		{
			return false;
		}
	}

	return true;
}
