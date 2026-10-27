#pragma once

#include "CoreMinimal.h"
#include "AdvancedMVVM/Viewmodels/BaseViewModel.h"
#include "Combat/AutoBattleTypes.h"
#include "AutoBattleViewModel.generated.h"

class AUnitActor;
class AZoneActor;
class UAutoBattleSubsystem;

UCLASS(BlueprintType, Blueprintable)
class AUTOBATTLESANDBOX_API UAutoBattleViewModel : public UBaseViewModel
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Auto Battle")
	void StartBattle();

	UFUNCTION(BlueprintCallable, Category = "Auto Battle")
	void CancelBattle();

	UFUNCTION(BlueprintCallable, Category = "Auto Battle")
	void ClearBattlefield();

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Auto Battle")
	bool bIsBattleRunning = false;

protected:
	virtual void ViewModelInitialize(AActor* InActor) override;
	virtual void ViewModelClear() override;

private:
	void SetBattleSubsystem(UAutoBattleSubsystem* InBattleSubsystem);
	void SetIsBattleRunning(bool bInIsBattleRunning);
	void RefreshBattleState();

	UFUNCTION()
	void HandleBattleStarted(AZoneActor* TeamA, AZoneActor* TeamB);

	UFUNCTION()
	void HandleRoundStarted(int32 RoundNumber);

	UFUNCTION()
	void HandleTurnStarted(AUnitActor* Attacker, AUnitActor* Target);

	UFUNCTION()
	void HandleTurnFinished(AUnitActor* Attacker, AUnitActor* Target, bool bAbilityActivated);

	UFUNCTION()
	void HandleBattleFinished(EAutoBattleResult Result, AZoneActor* WinningTeam, int32 CompletedRounds);

private:
	UPROPERTY(Transient)
	TObjectPtr<UAutoBattleSubsystem> BattleSubsystem = nullptr;
};
