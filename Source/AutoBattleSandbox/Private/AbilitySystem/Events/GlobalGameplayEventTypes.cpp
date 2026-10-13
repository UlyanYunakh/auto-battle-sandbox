#include "AbilitySystem/Events/GlobalGameplayEventTypes.h"

bool FGlobalGameplayAbilityTrigger::MatchesEvent(const FGameplayTag BroadcastEventTag) const
{
	if (!IsValid() || !BroadcastEventTag.IsValid())
	{
		return false;
	}

	switch (MatchPolicy)
	{
	case EGlobalGameplayEventMatchPolicy::Exact:
		return BroadcastEventTag.MatchesTagExact(EventTag);

	case EGlobalGameplayEventMatchPolicy::IncludeChildTags:
		return BroadcastEventTag.MatchesTag(EventTag);

	default:
		return false;
	}
}
