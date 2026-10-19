#include "AbilitySystem/Events/GlobalGameplayEventSubsystem.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "Subsystems/SubsystemCollection.h"

void UAutoBattleGlobalGameplayEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UAutoBattleAbilitySequenceSubsystem>();
	SequenceSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UAutoBattleAbilitySequenceSubsystem>() : nullptr;
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.AddDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.AddDynamic(this, &ThisClass::HandleSequenceFinished);
	}

	Subscriptions.Reset();
	PendingActivations.Reset();
	NextEventValue = 1;
	NextSubscriptionValue = 1;
}

void UAutoBattleGlobalGameplayEventSubsystem::Deinitialize()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);
	}

	SequenceSubsystem.Reset();
	Subscriptions.Reset();
	PendingActivations.Reset();
	AbilityActivationRequested.Clear();
	OnGlobalEventBroadcast.Clear();
	NextEventValue = 1;
	NextSubscriptionValue = 1;

	Super::Deinitialize();
}

FGlobalGameplayAbilitySubscriptionId UAutoBattleGlobalGameplayEventSubsystem::RegisterAbilitySubscription(
	UBattleAbilitySystem* AbilitySystem,
	const FGameplayAbilitySpecHandle AbilityHandle,
	const TSubclassOf<UBattleGameplayAbility> AbilityClass)
{
	if (!IsValid(AbilitySystem) || !AbilityHandle.IsValid() || !AbilityClass)
	{
		return FGlobalGameplayAbilitySubscriptionId();
	}

	const FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromHandle(AbilityHandle);
	const UBattleGameplayAbility* AbilityDefaultObject = AbilityClass.GetDefaultObject();
	if (!AbilitySpec || !AbilitySpec->Ability || !AbilitySpec->Ability->IsA(AbilityClass)
		|| !AbilityDefaultObject || AbilityDefaultObject->GetGlobalEventTriggers().IsEmpty())
	{
		return FGlobalGameplayAbilitySubscriptionId();
	}

	for (const TPair<FGlobalGameplayAbilitySubscriptionId, FRegisteredGlobalGameplayAbilitySubscription>& Pair
	     : Subscriptions)
	{
		if (Pair.Value.AbilitySystem == AbilitySystem && Pair.Value.AbilityHandle == AbilityHandle)
		{
			return Pair.Key;
		}
	}

	const FGlobalGameplayAbilitySubscriptionId SubscriptionId = AllocateSubscriptionId();
	if (!SubscriptionId.IsValid())
	{
		return FGlobalGameplayAbilitySubscriptionId();
	}

	FRegisteredGlobalGameplayAbilitySubscription Subscription;
	Subscription.SubscriptionId = SubscriptionId;
	Subscription.AbilitySystem = AbilitySystem;
	Subscription.AbilityHandle = AbilityHandle;
	Subscription.AbilityClass = AbilityClass;
	Subscriptions.Add(SubscriptionId, MoveTemp(Subscription));
	return SubscriptionId;
}

bool UAutoBattleGlobalGameplayEventSubsystem::UnregisterAbilitySubscription(
	const FGlobalGameplayAbilitySubscriptionId SubscriptionId)
{
	if (!SubscriptionId.IsValid() || Subscriptions.Remove(SubscriptionId) == 0)
	{
		return false;
	}

	InvalidatePendingActivations(SubscriptionId);
	return true;
}

void UAutoBattleGlobalGameplayEventSubsystem::UnregisterAbilitySystem(
	const UBattleAbilitySystem* AbilitySystem)
{
	if (!AbilitySystem)
	{
		return;
	}

	TArray<FGlobalGameplayAbilitySubscriptionId> SubscriptionIds;
	for (const TPair<FGlobalGameplayAbilitySubscriptionId, FRegisteredGlobalGameplayAbilitySubscription>& Pair
	     : Subscriptions)
	{
		if (Pair.Value.AbilitySystem == AbilitySystem)
		{
			SubscriptionIds.Add(Pair.Key);
		}
	}

	for (const FGlobalGameplayAbilitySubscriptionId SubscriptionId : SubscriptionIds)
	{
		UnregisterAbilitySubscription(SubscriptionId);
	}
}

bool UAutoBattleGlobalGameplayEventSubsystem::BroadcastGlobalEvent(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId SourceNodeId,
	const FGameplayTag EventTag,
	const FGameplayEventData& EventData,
	FGlobalGameplayEventDispatch& OutDispatch)
{
	OutDispatch = FGlobalGameplayEventDispatch();

	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (!SequenceSubsystemInstance || !SequenceId.IsValid() || !SourceNodeId.IsValid() || !EventTag.IsValid())
	{
		return false;
	}

	const FGlobalGameplayEventId EventId = AllocateEventId();
	if (!EventId.IsValid())
	{
		return false;
	}

	OutDispatch.EventId = EventId;
	OutDispatch.EventTag = EventTag;
	OutDispatch.SequenceId = SequenceId;
	OutDispatch.SourceNodeId = SourceNodeId;

	FGameplayEventData RoutedEventData = EventData;
	RoutedEventData.EventTag = EventTag;
	const TArray<FResolvedGlobalGameplayAbilitySubscription> ResolvedSubscriptions =
		ResolveSubscriptions(EventTag, RoutedEventData);
	for (const FResolvedGlobalGameplayAbilitySubscription& ResolvedSubscription : ResolvedSubscriptions)
	{
		OutDispatch.MatchingSubscriptions.Add(ResolvedSubscription.SubscriptionId);
	}

	if (ResolvedSubscriptions.IsEmpty())
	{
		OnGlobalEventBroadcast.Broadcast(OutDispatch);
		return true;
	}

	TArray<FAbilitySequenceNodeRequest> ChildRequests;
	ChildRequests.Reserve(ResolvedSubscriptions.Num());
	for (const FResolvedGlobalGameplayAbilitySubscription& ResolvedSubscription : ResolvedSubscriptions)
	{
		const FRegisteredGlobalGameplayAbilitySubscription* Subscription =
			Subscriptions.Find(ResolvedSubscription.SubscriptionId);

		FAbilitySequenceNodeRequest& ChildRequest = ChildRequests.AddDefaulted_GetRef();
		ChildRequest.DebugName = Subscription && Subscription->AbilityClass
			                         ? Subscription->AbilityClass->GetFName()
			                         : NAME_None;
		ChildRequest.TriggerEvent = EventTag;
	}

	if (!SequenceSubsystemInstance->PushChildren(
		SequenceId,
		SourceNodeId,
		ChildRequests,
		OutDispatch.SequenceTransition))
	{
		OutDispatch = FGlobalGameplayEventDispatch();
		return false;
	}

	TMap<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>& SequencePendingActivations =
		PendingActivations.FindOrAdd(SequenceId);

	for (int32 Index = 0; Index < ResolvedSubscriptions.Num(); ++Index)
	{
		const FResolvedGlobalGameplayAbilitySubscription& ResolvedSubscription = ResolvedSubscriptions[Index];
		const FRegisteredGlobalGameplayAbilitySubscription* Subscription =
			Subscriptions.Find(ResolvedSubscription.SubscriptionId);
		if (!Subscription)
		{
			continue;
		}

		FPendingGlobalGameplayAbilityActivation PendingActivation;
		PendingActivation.Request.AbilitySystem = Subscription->AbilitySystem.Get();
		PendingActivation.Request.AbilityClass = Subscription->AbilityClass;
		PendingActivation.Request.AbilityHandle = Subscription->AbilityHandle;
		PendingActivation.Request.SubscriptionId = Subscription->SubscriptionId;
		PendingActivation.Request.EventId = EventId;
		PendingActivation.Request.SequenceId = SequenceId;
		PendingActivation.Request.NodeId = OutDispatch.SequenceTransition.AddedNodeIds[Index];
		PendingActivation.Request.EventData = RoutedEventData;
		PendingActivation.Request.TargetingInput = ResolvedSubscription.TargetingInput;
		SequencePendingActivations.Add(PendingActivation.Request.NodeId, MoveTemp(PendingActivation));
	}

	OnGlobalEventBroadcast.Broadcast(OutDispatch);
	RequestActivationForNode(SequenceId, OutDispatch.SequenceTransition.NodeToActivate);
	return true;
}

bool UAutoBattleGlobalGameplayEventSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

void UAutoBattleGlobalGameplayEventSubsystem::HandleSequenceNodeStateChanged(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNode Node)
{
	if (Node.State == EAbilitySequenceNodeState::Activating)
	{
		RequestActivationForNode(SequenceId, Node.NodeId);
		return;
	}

	if (Node.State == EAbilitySequenceNodeState::Active || Node.State == EAbilitySequenceNodeState::Finished)
	{
		RemovePendingActivation(SequenceId, Node.NodeId);
	}
}

void UAutoBattleGlobalGameplayEventSubsystem::HandleSequenceFinished(
	const FAbilitySequenceId SequenceId,
	const EAbilitySequenceState)
{
	PendingActivations.Remove(SequenceId);
}

FGlobalGameplayEventId UAutoBattleGlobalGameplayEventSubsystem::AllocateEventId()
{
	if (NextEventValue <= 0)
	{
		return FGlobalGameplayEventId();
	}

	const FGlobalGameplayEventId EventId(NextEventValue);
	NextEventValue = NextEventValue == TNumericLimits<int64>::Max() ? 0 : NextEventValue + 1;
	return EventId;
}

FGlobalGameplayAbilitySubscriptionId UAutoBattleGlobalGameplayEventSubsystem::AllocateSubscriptionId()
{
	if (NextSubscriptionValue <= 0)
	{
		return FGlobalGameplayAbilitySubscriptionId();
	}

	const FGlobalGameplayAbilitySubscriptionId SubscriptionId(NextSubscriptionValue);
	NextSubscriptionValue = NextSubscriptionValue == TNumericLimits<int64>::Max()
		                        ? 0
		                        : NextSubscriptionValue + 1;
	return SubscriptionId;
}

TArray<FResolvedGlobalGameplayAbilitySubscription>
UAutoBattleGlobalGameplayEventSubsystem::ResolveSubscriptions(
	const FGameplayTag EventTag,
	const FGameplayEventData& EventData)
{
	TArray<FResolvedGlobalGameplayAbilitySubscription> ResolvedSubscriptions;
	TArray<FGlobalGameplayAbilitySubscriptionId> StaleSubscriptionIds;

	for (const TPair<FGlobalGameplayAbilitySubscriptionId, FRegisteredGlobalGameplayAbilitySubscription>& Pair
	     : Subscriptions)
	{
		const FRegisteredGlobalGameplayAbilitySubscription& Subscription = Pair.Value;
		const UBattleAbilitySystem* AbilitySystem = Subscription.AbilitySystem.Get();
		const UBattleGameplayAbility* AbilityDefaultObject = Subscription.AbilityClass
			                                                     ? Subscription.AbilityClass.GetDefaultObject()
			                                                     : nullptr;

		if (!AbilitySystem || !AbilitySystem->FindAbilitySpecFromHandle(Subscription.AbilityHandle)
			|| !AbilityDefaultObject)
		{
			StaleSubscriptionIds.Add(Pair.Key);
			continue;
		}

		const AActor* ListeningActor = AbilitySystem->GetAvatarActor();
		if (!ListeningActor)
		{
			ListeningActor = AbilitySystem->GetOwner();
		}

		FGlobalGameplayEventFilterContext FilterContext;
		FilterContext.ListeningActor = ListeningActor;
		FilterContext.EventData = &EventData;
		const FGlobalGameplayAbilityTrigger* MatchingTrigger =
			AbilityDefaultObject->FindGlobalEventTrigger(EventTag, FilterContext);
		if (!MatchingTrigger)
		{
			continue;
		}

		FResolvedGlobalGameplayAbilitySubscription& ResolvedSubscription =
			ResolvedSubscriptions.AddDefaulted_GetRef();
		ResolvedSubscription.SubscriptionId = Pair.Key;
		ResolvedSubscription.Priority = MatchingTrigger->Priority;
		MatchingTrigger->ResolveTargetingInput(FilterContext, ResolvedSubscription.TargetingInput);
	}

	for (const FGlobalGameplayAbilitySubscriptionId StaleSubscriptionId : StaleSubscriptionIds)
	{
		UnregisterAbilitySubscription(StaleSubscriptionId);
	}

	ResolvedSubscriptions.Sort([](
		const FResolvedGlobalGameplayAbilitySubscription& Left,
		const FResolvedGlobalGameplayAbilitySubscription& Right)
		{
			if (Left.Priority != Right.Priority)
			{
				return Left.Priority > Right.Priority;
			}

			return Left.SubscriptionId.Value < Right.SubscriptionId.Value;
		});
	return ResolvedSubscriptions;
}

void UAutoBattleGlobalGameplayEventSubsystem::RequestActivationForNode(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId)
{
	TMap<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>* SequencePendingActivations =
		PendingActivations.Find(SequenceId);
	FPendingGlobalGameplayAbilityActivation* PendingActivation = SequencePendingActivations
		                                                             ? SequencePendingActivations->Find(NodeId)
		                                                             : nullptr;
	if (!PendingActivation || PendingActivation->bRequestSent)
	{
		return;
	}

	if (!IsValid(PendingActivation->Request.AbilitySystem)
		|| !Subscriptions.Contains(PendingActivation->Request.SubscriptionId))
	{
		RemovePendingActivation(SequenceId, NodeId);
		if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
		{
			FAbilitySequenceTransition IgnoredTransition;
			SequenceSubsystemInstance->FinishNode(
				SequenceId,
				NodeId,
				EAbilitySequenceNodeResult::ActivationFailed,
				IgnoredTransition);
		}
		return;
	}

	PendingActivation->bRequestSent = true;
	AbilityActivationRequested.Broadcast(PendingActivation->Request);
}

void UAutoBattleGlobalGameplayEventSubsystem::RemovePendingActivation(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNodeId NodeId)
{
	TMap<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>* SequencePendingActivations =
		PendingActivations.Find(SequenceId);
	if (!SequencePendingActivations)
	{
		return;
	}

	SequencePendingActivations->Remove(NodeId);
	if (SequencePendingActivations->IsEmpty())
	{
		PendingActivations.Remove(SequenceId);
	}
}

void UAutoBattleGlobalGameplayEventSubsystem::InvalidatePendingActivations(
	const FGlobalGameplayAbilitySubscriptionId SubscriptionId)
{
	TArray<TPair<FAbilitySequenceId, FAbilitySequenceNodeId>> NodesToFail;
	for (TPair<FAbilitySequenceId, TMap<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>>&
	     SequencePair : PendingActivations)
	{
		for (TPair<FAbilitySequenceNodeId, FPendingGlobalGameplayAbilityActivation>& NodePair : SequencePair.Value)
		{
			if (NodePair.Value.Request.SubscriptionId == SubscriptionId)
			{
				NodePair.Value.Request.AbilitySystem = nullptr;
				NodePair.Value.bRequestSent = false;

				FAbilitySequenceSnapshot SequenceSnapshot;
				if (const UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
					SequenceSubsystemInstance
					&& SequenceSubsystemInstance->GetSequenceSnapshot(SequencePair.Key, SequenceSnapshot))
				{
					const FAbilitySequenceNode* SequenceNode = SequenceSnapshot.Nodes.FindByPredicate(
						[NodeId = NodePair.Key](const FAbilitySequenceNode& Node)
						{
							return Node.NodeId == NodeId;
						});
					if (SequenceNode && SequenceNode->State == EAbilitySequenceNodeState::Activating)
					{
						NodesToFail.Emplace(SequencePair.Key, NodePair.Key);
					}
				}
			}
		}
	}

	for (const TPair<FAbilitySequenceId, FAbilitySequenceNodeId>& NodeToFail : NodesToFail)
	{
		RequestActivationForNode(NodeToFail.Key, NodeToFail.Value);
	}
}
