#include "BlueprintLibraries/AbilityTargetingBlueprintLibrary.h"

FAbilityTargetSelectionInput UAbilityTargetingBlueprintLibrary::MakeNormalTargetInput()
{
	return FAbilityTargetSelectionInput();
}

FAbilityTargetSelectionInput UAbilityTargetingBlueprintLibrary::MakeRequiredTargetInputFromActor(AActor* Target)
{
	return MakeTargetInputFromActors({Target}, EAbilityTargetInputPolicy::RequireProvidedTargets);
}

FAbilityTargetSelectionInput UAbilityTargetingBlueprintLibrary::MakePreferredTargetInputFromActor(AActor* Target)
{
	return MakeTargetInputFromActors({Target}, EAbilityTargetInputPolicy::PreferProvidedTargets);
}

FAbilityTargetSelectionInput UAbilityTargetingBlueprintLibrary::MakeTargetInputFromActors(
	const TArray<AActor*>& Targets,
	const EAbilityTargetInputPolicy Policy)
{
	FAbilityTargetSelectionInput Input;
	Input.Policy = Policy;
	for (AActor* Target : Targets)
	{
		if (IsValid(Target))
		{
			Input.ProvidedActors.Add(Target);
		}
	}
	return Input;
}

FAbilityTargetSelectionInput UAbilityTargetingBlueprintLibrary::MakeTargetInputFromEventData(
	const FGameplayEventData& EventData,
	const EAbilityTargetInputPolicy Policy)
{
	return MakeTargetInputFromActors(
		{const_cast<AActor*>(EventData.Target.Get())},
		Policy);
}

TArray<AActor*> UAbilityTargetingBlueprintLibrary::GetActorsFromTargetData(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	TArray<AActor*> Actors;
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
				Actors.AddUnique(Actor.Get());
			}
		}
	}
	return Actors;
}
