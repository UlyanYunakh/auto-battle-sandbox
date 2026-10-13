#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Targeting/AbilityTargetingTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "AbilityTargetingSubsystem.generated.h"

class UAbilityTargetingDefinition;

UCLASS(BlueprintType)
class AUTOBATTLESANDBOX_API UAbilityTargetingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void RegisterTargetableActor(AActor* Actor);
	void UnregisterTargetableActor(const AActor* Actor);

	bool ValidateTargetData(
		const FAbilityTargetingContext& Context,
		const UAbilityTargetingDefinition& Definition,
		const FGameplayAbilityTargetDataHandle& TargetData,
		EAbilityTargetSelectionFailure& OutFailure) const;

	void FindValidCandidates(
		const FAbilityTargetingContext& Context,
		const UAbilityTargetingDefinition& Definition,
		TArray<FAbilityTargetCandidate>& OutCandidates) const;

	FAbilityTargetSelectionResult SelectAutomatically(
		const FAbilityTargetingContext& Context,
		const UAbilityTargetingDefinition& Definition) const;

	FGameplayAbilityTargetDataHandle MakeTargetData(
		const FGameplayAbilityTargetDataHandle& ExistingTargetData,
		const TArray<TObjectPtr<AActor>>& Actors) const;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	bool IsCandidateValid(
		const FAbilityTargetingContext& Context,
		const UAbilityTargetingDefinition& Definition,
		AActor* Actor) const;

	static void GetUniqueActors(
		const FGameplayAbilityTargetDataHandle& TargetData,
		TArray<AActor*>& OutActors);

private:
	TSet<TWeakObjectPtr<AActor>> TargetableActors;
};
