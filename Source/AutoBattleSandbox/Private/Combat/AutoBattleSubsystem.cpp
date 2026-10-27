#include "Combat/AutoBattleSubsystem.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Components/BattleAbilitySystem.h"
#include "AbilitySystem/Events/GlobalGameplayEventSubsystem.h"
#include "AbilitySystem/Sequences/AbilitySequenceSubsystem.h"
#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"
#include "Combat/AutoBattleGameplayTags.h"
#include "Combat/AutoBattleRulesAsset.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UAutoBattleSubsystem::Deinitialize()
{
	FinishBattle(EAutoBattleResult::Cancelled, nullptr, false);
	Super::Deinitialize();
}

bool UAutoBattleSubsystem::StartBattle(
	AZoneActor* TeamA,
	AZoneActor* TeamB,
	UAutoBattleRulesAsset* Rules,
	const int32 RandomSeed)
{
	if (IsBattleRunning() || !IsValid(TeamA) || !IsValid(TeamB) || TeamA == TeamB
		|| TeamA->GetUnitCount() <= 0 || TeamB->GetUnitCount() <= 0)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = World
		                                                                 ? World->GetSubsystem<
			                                                                 UAutoBattleAbilitySequenceSubsystem>()
		                                                                 : nullptr;
	if (!SequenceSubsystemInstance)
	{
		return false;
	}

	TeamAZone = TeamA;
	TeamBZone = TeamB;
	BattleRules = Rules;
	SequenceSubsystem = SequenceSubsystemInstance;
	CurrentRound = 0;
	NextAttackAllowedTime = 0.0;
	bWasWaitingForDeathAnimations = false;
	RandomStream.Initialize(RandomSeed == 0 ? FMath::Rand() : RandomSeed);
	BattleState = EAutoBattleState::Preparing;
	TeamAZone->SetBattleLocked(true);
	TeamBZone->SetBattleLocked(true);
	BindSequenceEvents();

	ResolveDefeatedUnits();
	if (TryFinishForTeamDefeat())
	{
		return false;
	}

	OnBattleStarted.Broadcast(TeamAZone, TeamBZone);
	StartBattleEvent();
	return true;
}

void UAutoBattleSubsystem::CancelBattle()
{
	if (IsBattleRunning())
	{
		FinishBattle(EAutoBattleResult::Cancelled, nullptr, true);
	}
}

bool UAutoBattleSubsystem::IsBattleRunning() const
{
	return BattleState != EAutoBattleState::Inactive
		&& BattleState != EAutoBattleState::Finished
		&& BattleState != EAutoBattleState::Cancelled;
}

bool UAutoBattleSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

void UAutoBattleSubsystem::BeginRound()
{
	if (!IsBattleRunning())
	{
		return;
	}

	++CurrentRound;
	UnitsThatActed.Reset();
	bAnyAttackStartedThisRound = false;
	BuildRoundParticipants();
	BattleState = EAutoBattleState::SelectingAttacker;
	OnRoundStarted.Broadcast(CurrentRound);
	ScheduleAdvance();
}

void UAutoBattleSubsystem::AdvanceBattle()
{
	AdvanceTimerHandle.Invalidate();
	bAdvanceScheduled = false;
	if (!IsBattleRunning() || BattleState == EAutoBattleState::WaitingForSequence)
	{
		return;
	}
	if (HasPendingDeathAnimations())
	{
		bWasWaitingForDeathAnimations = true;
		ScheduleAdvance();
		return;
	}
	if (bWasWaitingForDeathAnimations)
	{
		bWasWaitingForDeathAnimations = false;
		if (const UWorld* World = GetWorld())
		{
			NextAttackAllowedTime = FMath::Max(
				NextAttackAllowedTime,
				World->GetTimeSeconds() + GetMinimumAttackInterval());
		}
	}

	BattleState = EAutoBattleState::ResolvingDefeats;
	ResolveDefeatedUnits();
	if (TryFinishForTeamDefeat())
	{
		return;
	}

	const float RemainingAttackDelay = GetRemainingAttackDelay();
	if (RemainingAttackDelay > KINDA_SMALL_NUMBER)
	{
		ScheduleAdvance(RemainingAttackDelay);
		return;
	}

	BattleState = EAutoBattleState::SelectingAttacker;
	AUnitActor* Attacker = SelectNextAttacker();
	if (!Attacker)
	{
		if (!bAnyAttackStartedThisRound)
		{
			FinishBattle(EAutoBattleResult::Draw, nullptr, true);
			return;
		}

		BeginRound();
		return;
	}

	AUnitActor* Target = SelectTarget(*Attacker);
	UnitsThatActed.Add(TWeakObjectPtr<AUnitActor>(Attacker));
	if (!Target)
	{
		ScheduleAdvance();
		return;
	}

	StartCurrentAttack(*Attacker, *Target);
}

void UAutoBattleSubsystem::ScheduleAdvance(const float Delay)
{
	if (bAdvanceScheduled || !IsBattleRunning())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishBattle(EAutoBattleResult::Cancelled, nullptr, true);
		return;
	}

	bAdvanceScheduled = true;
	if (Delay > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			AdvanceTimerHandle,
			this,
			&ThisClass::AdvanceBattle,
			Delay,
			false);
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::AdvanceBattle));
}

float UAutoBattleSubsystem::GetMinimumAttackInterval() const
{
	return BattleRules ? FMath::Max(0.0f, BattleRules->MinimumAttackInterval) : 0.5f;
}

float UAutoBattleSubsystem::GetRemainingAttackDelay() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	return static_cast<float>(FMath::Max(0.0, NextAttackAllowedTime - World->GetTimeSeconds()));
}

AUnitActor* UAutoBattleSubsystem::SelectNextAttacker() const
{
	AUnitActor* BestUnit = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	int32 BestBaseOrder = TNumericLimits<int32>::Max();

	for (const TWeakObjectPtr<AUnitActor>& Participant : RoundParticipants)
	{
		AUnitActor* Unit = Participant.Get();
		if (!IsValid(Unit) || UnitsThatActed.Contains(TWeakObjectPtr<AUnitActor>(Unit))
			|| !IsUnitAbleToAttack(*Unit))
		{
			continue;
		}

		const int32 Priority = CalculateAttackPriority(*Unit);
		const int32 BaseOrder = FindBaseOrder(*Unit);
		if (!BestUnit || Priority > BestPriority || (Priority == BestPriority && BaseOrder < BestBaseOrder))
		{
			BestUnit = Unit;
			BestPriority = Priority;
			BestBaseOrder = BaseOrder;
		}
	}

	return BestUnit;
}

AUnitActor* UAutoBattleSubsystem::SelectTarget(AUnitActor& Attacker)
{
	AZoneActor* EnemyZone = Attacker.GetOwningZone() == TeamAZone ? TeamBZone : TeamAZone;
	if (!IsValid(EnemyZone))
	{
		return nullptr;
	}

	FGameplayTagContainer AttackerTags;
	if (const UBattleAbilitySystem* AbilitySystem = Attacker.GetUnitAbilitySystem())
	{
		AbilitySystem->GetOwnedGameplayTags(AttackerTags);
	}

	TArray<AUnitActor*> BestTargets;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	for (AUnitActor* Candidate : EnemyZone->GetUnits())
	{
		if (!IsValid(Candidate) || !IsUnitTargetable(*Candidate))
		{
			continue;
		}

		FGameplayTagContainer TargetTags;
		if (const UBattleAbilitySystem* AbilitySystem = Candidate->GetUnitAbilitySystem())
		{
			AbilitySystem->GetOwnedGameplayTags(TargetTags);
		}

		bool bExcluded = false;
		const int32 Priority = CalculateTargetPriority(AttackerTags, TargetTags, bExcluded);
		if (bExcluded)
		{
			continue;
		}

		if (BestTargets.IsEmpty() || Priority > BestPriority)
		{
			BestTargets.Reset();
			BestTargets.Add(Candidate);
			BestPriority = Priority;
		}
		else if (Priority == BestPriority)
		{
			BestTargets.Add(Candidate);
		}
	}

	return BestTargets.IsEmpty() ? nullptr : BestTargets[RandomStream.RandRange(0, BestTargets.Num() - 1)];
}

int32 UAutoBattleSubsystem::CalculateAttackPriority(const AUnitActor& Unit) const
{
	if (!BattleRules)
	{
		return 0;
	}

	FGameplayTagContainer UnitTags;
	if (const UBattleAbilitySystem* AbilitySystem = Unit.GetUnitAbilitySystem())
	{
		AbilitySystem->GetOwnedGameplayTags(UnitTags);
	}

	int32 Priority = 0;
	for (const FAutoBattleAttackOrderRule& Rule : BattleRules->AttackOrderRules)
	{
		if (QueryMatches(Rule.UnitQuery, UnitTags))
		{
			Priority += Rule.PriorityModifier;
		}
	}
	return Priority;
}

int32 UAutoBattleSubsystem::CalculateTargetPriority(
	const FGameplayTagContainer& AttackerTags,
	const FGameplayTagContainer& TargetTags,
	bool& bOutExcluded) const
{
	bOutExcluded = false;
	if (!BattleRules)
	{
		return 0;
	}

	int32 Priority = 0;
	for (const FAutoBattleTargetRule& Rule : BattleRules->TargetRules)
	{
		if (!QueryMatches(Rule.AttackerQuery, AttackerTags) || !QueryMatches(Rule.TargetQuery, TargetTags))
		{
			continue;
		}

		if (Rule.Effect == EAutoBattleTargetRuleEffect::Exclude)
		{
			bOutExcluded = true;
			return Priority;
		}

		Priority += Rule.Priority;
	}
	return Priority;
}

bool UAutoBattleSubsystem::IsUnitAbleToAttack(const AUnitActor& Unit) const
{
	if (!Unit.IsAlive() || !Unit.GetBasicAttackAbility())
	{
		return false;
	}

	const UBattleAbilitySystem* AbilitySystem = Unit.GetUnitAbilitySystem();
	return AbilitySystem && (!BattleRules || !AbilitySystem->
		HasAnyMatchingGameplayTags(BattleRules->PreventAttackTags));
}

bool UAutoBattleSubsystem::IsUnitTargetable(const AUnitActor& Unit) const
{
	if (!Unit.IsAlive())
	{
		return false;
	}

	const UBattleAbilitySystem* AbilitySystem = Unit.GetUnitAbilitySystem();
	return AbilitySystem && (!BattleRules || !AbilitySystem->HasAnyMatchingGameplayTags(BattleRules->UntargetableTags));
}

void UAutoBattleSubsystem::StartCurrentAttack(AUnitActor& Attacker, AUnitActor& Target)
{
	CurrentAttacker = &Attacker;
	CurrentTarget = &Target;
	ActiveSequenceId = FAbilitySequenceId();
	BattleState = EAutoBattleState::WaitingForSequence;
	OnTurnStarted.Broadcast(CurrentAttacker, CurrentTarget);

	UBattleAbilitySystem* AbilitySystem = Attacker.GetUnitAbilitySystem();
	const bool bActivated = AbilitySystem && AbilitySystem->TryActivateAbilityWithTargetAndSequence(
		Attacker.GetBasicAttackAbility(),
		&Target,
		ActiveSequenceId);
	if (!bActivated)
	{
		CompleteCurrentTurn(false);
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		NextAttackAllowedTime = World->GetTimeSeconds() + GetMinimumAttackInterval();
	}
	bAnyAttackStartedThisRound = true;
	const UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	if (!ActiveSequenceId.IsValid() || !SequenceSubsystemInstance
		|| !SequenceSubsystemInstance->IsSequenceRunning(ActiveSequenceId))
	{
		CompleteCurrentTurn(true);
	}
}

void UAutoBattleSubsystem::CompleteCurrentTurn(const bool bAbilityActivated)
{
	if (BattleState != EAutoBattleState::WaitingForSequence)
	{
		return;
	}

	AUnitActor* FinishedAttacker = CurrentAttacker;
	AUnitActor* FinishedTarget = CurrentTarget;
	CurrentAttacker = nullptr;
	CurrentTarget = nullptr;
	ActiveSequenceId = FAbilitySequenceId();
	BattleState = EAutoBattleState::ResolvingDefeats;
	OnTurnFinished.Broadcast(FinishedAttacker, FinishedTarget, bAbilityActivated);
	ScheduleAdvance();
}

void UAutoBattleSubsystem::ResolveDefeatedUnits()
{
	if (TeamAZone)
	{
		TeamAZone->RemoveDefeatedUnits();
	}
	if (TeamBZone)
	{
		TeamBZone->RemoveDefeatedUnits();
	}
}

bool UAutoBattleSubsystem::HasPendingDeathAnimations() const
{
	const AZoneActor* Zones[] = {TeamAZone, TeamBZone};
	for (const AZoneActor* Zone : Zones)
	{
		if (!IsValid(Zone))
		{
			continue;
		}

		for (const AUnitActor* Unit : Zone->GetUnits())
		{
			if (IsValid(Unit) && Unit->IsDeathAnimationPlaying())
			{
				return true;
			}
		}
	}

	return false;
}

bool UAutoBattleSubsystem::TryFinishForTeamDefeat()
{
	const bool bTeamAAlive = TeamAZone && TeamAZone->HasLivingUnits();
	const bool bTeamBAlive = TeamBZone && TeamBZone->HasLivingUnits();
	if (bTeamAAlive && bTeamBAlive)
	{
		return false;
	}

	if (!bTeamAAlive && !bTeamBAlive)
	{
		FinishBattle(EAutoBattleResult::Draw, nullptr, true);
	}
	else if (bTeamAAlive)
	{
		FinishBattle(EAutoBattleResult::TeamA, TeamAZone, true);
	}
	else
	{
		FinishBattle(EAutoBattleResult::TeamB, TeamBZone, true);
	}
	return true;
}

void UAutoBattleSubsystem::FinishBattle(
	const EAutoBattleResult Result,
	AZoneActor* WinningTeam,
	const bool bBroadcast)
{
	const FAbilitySequenceId SequenceToCancel = BattleStartSequenceId;
	BattleStartSequenceId = FAbilitySequenceId();
	BattleStartRootNodeId = FAbilitySequenceNodeId();
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
		SequenceSubsystemInstance && SequenceToCancel.IsValid()
		&& SequenceSubsystemInstance->IsSequenceRunning(SequenceToCancel))
	{
		TArray<FAbilitySequenceNodeId> IgnoredNodesToCancel;
		SequenceSubsystemInstance->CancelSequence(SequenceToCancel, IgnoredNodesToCancel);
	}

	if (TeamAZone)
	{
		TeamAZone->SetBattleLocked(false);
	}
	if (TeamBZone)
	{
		TeamBZone->SetBattleLocked(false);
	}

	UnbindSequenceEvents();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AdvanceTimerHandle);
	}
	AdvanceTimerHandle.Invalidate();
	bAdvanceScheduled = false;
	NextAttackAllowedTime = 0.0;
	bWasWaitingForDeathAnimations = false;
	CurrentAttacker = nullptr;
	CurrentTarget = nullptr;
	ActiveSequenceId = FAbilitySequenceId();
	RoundParticipants.Reset();
	UnitsThatActed.Reset();
	BattleState = Result == EAutoBattleResult::Cancelled ? EAutoBattleState::Cancelled : EAutoBattleState::Finished;
	if (bBroadcast)
	{
		OnBattleFinished.Broadcast(Result, WinningTeam, CurrentRound);
	}

	TeamAZone = nullptr;
	TeamBZone = nullptr;
	BattleRules = nullptr;
	SequenceSubsystem.Reset();
}

void UAutoBattleSubsystem::StartBattleEvent()
{
	UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
	UWorld* World = GetWorld();
	UAutoBattleGlobalGameplayEventSubsystem* GlobalEventSubsystem = World
		                                                                ? World->GetSubsystem<
			                                                                UAutoBattleGlobalGameplayEventSubsystem>()
		                                                                : nullptr;
	if (!SequenceSubsystemInstance || !GlobalEventSubsystem)
	{
		BeginRound();
		return;
	}

	FAbilitySequenceNodeRequest RootRequest;
	RootRequest.DebugName = TEXT("BattleStart");
	if (!SequenceSubsystemInstance->StartSequence(
		RootRequest,
		BattleStartSequenceId,
		BattleStartRootNodeId))
	{
		BeginRound();
		return;
	}

	FGlobalGameplayEventDispatch Dispatch;
	if (!GlobalEventSubsystem->BroadcastGlobalEvent(
			BattleStartSequenceId,
			BattleStartRootNodeId,
			AutoBattleGameplayTags::Battle_Event_Start,
			FGameplayEventData(),
			Dispatch)
		|| Dispatch.SequenceTransition.AddedNodeIds.IsEmpty())
	{
		CompleteBattleStartEvent();
	}
}

void UAutoBattleSubsystem::CompleteBattleStartEvent()
{
	const FAbilitySequenceId CompletedSequenceId = BattleStartSequenceId;
	const FAbilitySequenceNodeId CompletedRootNodeId = BattleStartRootNodeId;
	BattleStartSequenceId = FAbilitySequenceId();
	BattleStartRootNodeId = FAbilitySequenceNodeId();

	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get();
		SequenceSubsystemInstance && CompletedSequenceId.IsValid() && CompletedRootNodeId.IsValid())
	{
		FAbilitySequenceTransition IgnoredTransition;
		SequenceSubsystemInstance->FinishNode(
			CompletedSequenceId,
			CompletedRootNodeId,
			EAbilitySequenceNodeResult::Succeeded,
			IgnoredTransition);
	}

	if (IsBattleRunning() && BattleState == EAutoBattleState::Preparing)
	{
		BeginRound();
	}
}

void UAutoBattleSubsystem::BindSequenceEvents()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.AddUniqueDynamic(this, &ThisClass::HandleSequenceFinished);
	}
}

void UAutoBattleSubsystem::UnbindSequenceEvents()
{
	if (UAutoBattleAbilitySequenceSubsystem* SequenceSubsystemInstance = SequenceSubsystem.Get())
	{
		SequenceSubsystemInstance->OnSequenceNodeStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSequenceNodeStateChanged);
		SequenceSubsystemInstance->OnSequenceFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);
	}
}

void UAutoBattleSubsystem::BuildRoundParticipants()
{
	RoundParticipants.Reset();
	if (TeamAZone)
	{
		for (AUnitActor* Unit : TeamAZone->GetUnits())
		{
			RoundParticipants.Add(Unit);
		}
	}
	if (TeamBZone)
	{
		for (AUnitActor* Unit : TeamBZone->GetUnits())
		{
			RoundParticipants.Add(Unit);
		}
	}
}

int32 UAutoBattleSubsystem::FindBaseOrder(const AUnitActor& Unit) const
{
	int32 BaseOrder = 0;
	if (TeamAZone)
	{
		for (AUnitActor* TeamUnit : TeamAZone->GetUnits())
		{
			if (TeamUnit == &Unit)
			{
				return BaseOrder;
			}
			++BaseOrder;
		}
	}
	if (TeamBZone)
	{
		for (AUnitActor* TeamUnit : TeamBZone->GetUnits())
		{
			if (TeamUnit == &Unit)
			{
				return BaseOrder;
			}
			++BaseOrder;
		}
	}
	return TNumericLimits<int32>::Max();
}

bool UAutoBattleSubsystem::QueryMatches(
	const FGameplayTagQuery& Query,
	const FGameplayTagContainer& Tags)
{
	return Query.IsEmpty() || Query.Matches(Tags);
}

void UAutoBattleSubsystem::HandleSequenceNodeStateChanged(
	const FAbilitySequenceId SequenceId,
	const FAbilitySequenceNode Node)
{
	if (SequenceId == BattleStartSequenceId
		&& Node.NodeId == BattleStartRootNodeId
		&& Node.State == EAbilitySequenceNodeState::Active)
	{
		CompleteBattleStartEvent();
	}
}

void UAutoBattleSubsystem::HandleSequenceFinished(
	const FAbilitySequenceId SequenceId,
	const EAbilitySequenceState)
{
	if (SequenceId == BattleStartSequenceId)
	{
		BattleStartSequenceId = FAbilitySequenceId();
		BattleStartRootNodeId = FAbilitySequenceNodeId();
		if (IsBattleRunning() && BattleState == EAutoBattleState::Preparing)
		{
			BeginRound();
		}
		return;
	}

	if (BattleState == EAutoBattleState::WaitingForSequence && SequenceId == ActiveSequenceId)
	{
		CompleteCurrentTurn(true);
	}
}
