#include "ViewModels/AutoBattleViewModel.h"

#include "Actors/UnitActor.h"
#include "Actors/ZoneActor.h"
#include "Combat/AutoBattleSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutoBattleViewModel, Log, All);

void UAutoBattleViewModel::StartBattle()
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle start requested."));

	if (!BattleSubsystem)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("Cannot start battle: the auto battle subsystem is not available. "
			       "Make sure the view model was initialized with an actor in a supported game world."));
		RefreshBattleState();
		return;
	}

	UWorld* const BattleWorld = BattleSubsystem->GetWorld();
	if (!BattleWorld)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("Cannot start battle: the auto battle subsystem has no world."));
		RefreshBattleState();
		return;
	}

	TArray<AZoneActor*> TeamZones;
	for (TActorIterator<AZoneActor> ZoneIterator(BattleWorld); ZoneIterator; ++ZoneIterator)
	{
		TeamZones.Add(*ZoneIterator);
	}

	TeamZones.Sort([](const AZoneActor& Left, const AZoneActor& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});

	if (TeamZones.Num() != 2)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("Cannot start battle: expected exactly 2 zone actors in world '%s', but found %d."),
		       *GetNameSafe(BattleWorld),
		       TeamZones.Num());
		RefreshBattleState();
		return;
	}

	AZoneActor* TeamA = TeamZones[0];
	AZoneActor* TeamB = TeamZones[1];
	UE_LOG(LogAutoBattleViewModel, Warning,
	       TEXT("Starting battle between '%s' (%d units, living units: %s) and '%s' (%d units, living units: %s)."),
	       *TeamA->GetPathName(),
	       TeamA->GetUnitCount(),
	       TeamA->HasLivingUnits() ? TEXT("yes") : TEXT("no"),
	       *TeamB->GetPathName(),
	       TeamB->GetUnitCount(),
	       TeamB->HasLivingUnits() ? TEXT("yes") : TEXT("no"));

	if (const bool bBattleStarted = BattleSubsystem->StartBattle(TeamA, TeamB, nullptr); !bBattleStarted)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("The auto battle subsystem rejected the start request. "
			       "Battle already running: %s; Team A units: %d; Team A has living units: %s; "
			       "Team B units: %d; Team B has living units: %s. "
			       "If these values are valid, verify that the ability sequence subsystem is available."),
		       BattleSubsystem->IsBattleRunning() ? TEXT("yes") : TEXT("no"),
		       TeamA->GetUnitCount(),
		       TeamA->HasLivingUnits() ? TEXT("yes") : TEXT("no"),
		       TeamB->GetUnitCount(),
		       TeamB->HasLivingUnits() ? TEXT("yes") : TEXT("no"));
	}
	else
	{
		UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle start request was accepted."));
	}

	RefreshBattleState();
}

void UAutoBattleViewModel::CancelBattle()
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle cancellation requested."));

	if (BattleSubsystem)
	{
		BattleSubsystem->CancelBattle();
	}

	RefreshBattleState();
}

void UAutoBattleViewModel::ClearBattlefield()
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battlefield clear requested."));

	if (!BattleSubsystem)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("Cannot clear battlefield: the auto battle subsystem is not available."));
		RefreshBattleState();
		return;
	}

	BattleSubsystem->CancelBattle();
	UWorld* const BattleWorld = BattleSubsystem->GetWorld();
	if (!BattleWorld)
	{
		UE_LOG(LogAutoBattleViewModel, Error,
		       TEXT("Cannot clear battlefield: the auto battle subsystem has no world."));
		RefreshBattleState();
		return;
	}

	TArray<AUnitActor*> UnitsToDestroy;
	for (TActorIterator<AUnitActor> UnitIterator(BattleWorld); UnitIterator; ++UnitIterator)
	{
		UnitsToDestroy.Add(*UnitIterator);
	}

	for (TActorIterator<AZoneActor> ZoneIterator(BattleWorld); ZoneIterator; ++ZoneIterator)
	{
		AZoneActor* Zone = *ZoneIterator;
		Zone->SetBattleLocked(false);
		Zone->ClearPreviewUnit();

		const TArray<TObjectPtr<AUnitActor>> ZoneUnits = Zone->GetUnits();
		for (AUnitActor* Unit : ZoneUnits)
		{
			Zone->RemoveUnit(Unit);
		}
	}

	int32 DestroyedUnitCount = 0;
	for (AUnitActor* Unit : UnitsToDestroy)
	{
		if (IsValid(Unit) && Unit->Destroy())
		{
			++DestroyedUnitCount;
		}
	}

	UE_LOG(LogAutoBattleViewModel, Warning,
	       TEXT("Battlefield cleared. Destroyed %d unit actors."),
	       DestroyedUnitCount);
	RefreshBattleState();
}

void UAutoBattleViewModel::ViewModelInitialize(AActor* InActor)
{
	Super::ViewModelInitialize(InActor);

	UWorld* World = IsValid(InActor) ? InActor->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogAutoBattleViewModel, Warning,
		       TEXT("Auto battle view model initialized without a valid actor world. Actor: '%s'."),
		       *GetNameSafe(InActor));
	}
	SetBattleSubsystem(World ? World->GetSubsystem<UAutoBattleSubsystem>() : nullptr);
}

void UAutoBattleViewModel::ViewModelClear()
{
	SetBattleSubsystem(nullptr);
	Super::ViewModelClear();
}

void UAutoBattleViewModel::SetBattleSubsystem(UAutoBattleSubsystem* InBattleSubsystem)
{
	if (BattleSubsystem == InBattleSubsystem)
	{
		RefreshBattleState();
		return;
	}

	if (BattleSubsystem)
	{
		BattleSubsystem->OnBattleStarted.RemoveDynamic(this, &ThisClass::HandleBattleStarted);
		BattleSubsystem->OnRoundStarted.RemoveDynamic(this, &ThisClass::HandleRoundStarted);
		BattleSubsystem->OnTurnStarted.RemoveDynamic(this, &ThisClass::HandleTurnStarted);
		BattleSubsystem->OnTurnFinished.RemoveDynamic(this, &ThisClass::HandleTurnFinished);
		BattleSubsystem->OnBattleFinished.RemoveDynamic(this, &ThisClass::HandleBattleFinished);
	}

	BattleSubsystem = InBattleSubsystem;
	if (BattleSubsystem)
	{
		UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Bound to auto battle subsystem '%s'."),
		       *GetNameSafe(BattleSubsystem));
		BattleSubsystem->OnBattleStarted.AddUniqueDynamic(this, &ThisClass::HandleBattleStarted);
		BattleSubsystem->OnRoundStarted.AddUniqueDynamic(this, &ThisClass::HandleRoundStarted);
		BattleSubsystem->OnTurnStarted.AddUniqueDynamic(this, &ThisClass::HandleTurnStarted);
		BattleSubsystem->OnTurnFinished.AddUniqueDynamic(this, &ThisClass::HandleTurnFinished);
		BattleSubsystem->OnBattleFinished.AddUniqueDynamic(this, &ThisClass::HandleBattleFinished);
	}
	else
	{
		UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Auto battle view model has no battle subsystem."));
	}

	RefreshBattleState();
}

void UAutoBattleViewModel::SetIsBattleRunning(const bool bInIsBattleRunning)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsBattleRunning, bInIsBattleRunning);
}

void UAutoBattleViewModel::RefreshBattleState()
{
	SetIsBattleRunning(BattleSubsystem && BattleSubsystem->IsBattleRunning());
}

void UAutoBattleViewModel::HandleBattleStarted(AZoneActor* TeamA, AZoneActor* TeamB)
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle started: Team A '%s', Team B '%s'."),
	       *GetNameSafe(TeamA),
	       *GetNameSafe(TeamB));
	SetIsBattleRunning(true);
}

void UAutoBattleViewModel::HandleRoundStarted(const int32 RoundNumber)
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle round %d started."), RoundNumber);
}

void UAutoBattleViewModel::HandleTurnStarted(AUnitActor* Attacker, AUnitActor* Target)
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle turn started: '%s' attacking '%s'."),
	       *GetNameSafe(Attacker),
	       *GetNameSafe(Target));
}

void UAutoBattleViewModel::HandleTurnFinished(
	AUnitActor* Attacker,
	AUnitActor* Target,
	const bool bAbilityActivated)
{
	UE_LOG(LogAutoBattleViewModel, Warning,
	       TEXT("Battle turn finished: attacker '%s', target '%s', ability activated: %s."),
	       *GetNameSafe(Attacker),
	       *GetNameSafe(Target),
	       bAbilityActivated ? TEXT("yes") : TEXT("no"));
}

void UAutoBattleViewModel::HandleBattleFinished(
	const EAutoBattleResult Result,
	AZoneActor* WinningTeam,
	const int32 CompletedRounds)
{
	UE_LOG(LogAutoBattleViewModel, Warning, TEXT("Battle finished: result '%s', winner '%s', completed rounds: %d."),
	       *UEnum::GetValueAsString(Result),
	       *GetNameSafe(WinningTeam),
	       CompletedRounds);
	SetIsBattleRunning(false);
}
