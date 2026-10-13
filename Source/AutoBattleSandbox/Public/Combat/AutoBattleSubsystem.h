#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Sequences/AbilitySequenceTypes.h"
#include "Combat/AutoBattleTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "AutoBattleSubsystem.generated.h"

class AUnitActor;
class AZoneActor;
class UAutoBattleAbilitySequenceSubsystem;
class UAutoBattleRulesAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAutoBattleStartedSignature,
	AZoneActor*, TeamA,
	AZoneActor*, TeamB);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAutoBattleRoundStartedSignature, int32, RoundNumber);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAutoBattleTurnStartedSignature,
	AUnitActor*, Attacker,
	AUnitActor*, Target);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FAutoBattleTurnFinishedSignature,
	AUnitActor*, Attacker,
	AUnitActor*, Target,
	bool, bAbilityActivated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FAutoBattleFinishedSignature,
	EAutoBattleResult, Result,
	AZoneActor*, WinningTeam,
	int32, CompletedRounds);

UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAutoBattleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Auto Battle")
	bool StartBattle(
		AZoneActor* TeamA,
		AZoneActor* TeamB,
		UAutoBattleRulesAsset* Rules,
		int32 RandomSeed = 0);

	UFUNCTION(BlueprintCallable, Category = "Auto Battle")
	void CancelBattle();

	UFUNCTION(BlueprintPure, Category = "Auto Battle")
	bool IsBattleRunning() const;

	UFUNCTION(BlueprintPure, Category = "Auto Battle")
	EAutoBattleState GetBattleState() const { return BattleState; }

	UFUNCTION(BlueprintPure, Category = "Auto Battle")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "Auto Battle")
	AUnitActor* GetCurrentAttacker() const { return CurrentAttacker; }

	UFUNCTION(BlueprintPure, Category = "Auto Battle")
	AUnitActor* GetCurrentTarget() const { return CurrentTarget; }

public:
	UPROPERTY(BlueprintAssignable, Category = "Auto Battle|Events")
	FAutoBattleStartedSignature OnBattleStarted;

	UPROPERTY(BlueprintAssignable, Category = "Auto Battle|Events")
	FAutoBattleRoundStartedSignature OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "Auto Battle|Events")
	FAutoBattleTurnStartedSignature OnTurnStarted;

	UPROPERTY(BlueprintAssignable, Category = "Auto Battle|Events")
	FAutoBattleTurnFinishedSignature OnTurnFinished;

	UPROPERTY(BlueprintAssignable, Category = "Auto Battle|Events")
	FAutoBattleFinishedSignature OnBattleFinished;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void BeginRound();
	void AdvanceBattle();
	void ScheduleAdvance();
	AUnitActor* SelectNextAttacker() const;
	AUnitActor* SelectTarget(AUnitActor& Attacker);
	int32 CalculateAttackPriority(const AUnitActor& Unit) const;
	int32 CalculateTargetPriority(
		const FGameplayTagContainer& AttackerTags,
		const FGameplayTagContainer& TargetTags,
		bool& bOutExcluded) const;
	bool IsUnitAbleToAttack(const AUnitActor& Unit) const;
	bool IsUnitTargetable(const AUnitActor& Unit) const;
	void StartCurrentAttack(AUnitActor& Attacker, AUnitActor& Target);
	void CompleteCurrentTurn(bool bAbilityActivated);
	void ResolveDefeatedUnits();
	bool TryFinishForTeamDefeat();
	void FinishBattle(EAutoBattleResult Result, AZoneActor* WinningTeam, bool bBroadcast);
	void BindSequenceEvents();
	void UnbindSequenceEvents();
	void BuildRoundParticipants();
	int32 FindBaseOrder(const AUnitActor& Unit) const;
	static bool QueryMatches(const FGameplayTagQuery& Query, const FGameplayTagContainer& Tags);

	UFUNCTION()
	void HandleSequenceFinished(FAbilitySequenceId SequenceId, EAbilitySequenceState FinalState);

private:
	UPROPERTY(Transient)
	TObjectPtr<AZoneActor> TeamAZone = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AZoneActor> TeamBZone = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAutoBattleRulesAsset> BattleRules = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> CurrentAttacker = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AUnitActor> CurrentTarget = nullptr;

	TWeakObjectPtr<UAutoBattleAbilitySequenceSubsystem> SequenceSubsystem;
	TArray<TWeakObjectPtr<AUnitActor>> RoundParticipants;
	TSet<TWeakObjectPtr<AUnitActor>> UnitsThatActed;
	FRandomStream RandomStream;
	FAbilitySequenceId ActiveSequenceId;
	EAutoBattleState BattleState = EAutoBattleState::Inactive;
	int32 CurrentRound = 0;
	bool bAnyAttackStartedThisRound = false;
	bool bAdvanceScheduled = false;
};
