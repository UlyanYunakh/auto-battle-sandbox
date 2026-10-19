#include "AbilitySystem/Animation/ApplyAnimationProvider.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

FApplyAnimationMockProviderDefinition::FApplyAnimationMockProviderDefinition()
{
	ProviderClass = UApplyAnimationMockProvider::StaticClass();
}

FApplyAnimationNiagaraProviderDefinition::FApplyAnimationNiagaraProviderDefinition()
{
	ProviderClass = UApplyAnimationNiagaraProvider::StaticClass();
}

UApplyAnimationProvider* UApplyAnimationProvider::CreateProvider(
	const FInstancedStruct& ProviderDefinition,
	UObject* Outer)
{
	if (!Outer || !IsProviderDefinitionValid(ProviderDefinition))
	{
		return nullptr;
	}

	const FApplyAnimationProviderDefinition& Definition =
		ProviderDefinition.Get<FApplyAnimationProviderDefinition>();
	UApplyAnimationProvider* Provider = NewObject<UApplyAnimationProvider>(Outer, Definition.ProviderClass);
	if (!Provider || !Provider->InitializeFromProviderDefinition(ProviderDefinition))
	{
		return nullptr;
	}
	return Provider;
}

bool UApplyAnimationProvider::IsProviderDefinitionValid(const FInstancedStruct& ProviderDefinition)
{
	const FApplyAnimationProviderDefinition* Definition =
		ProviderDefinition.GetPtr<FApplyAnimationProviderDefinition>();
	if (!Definition || !Definition->ProviderClass
		|| Definition->ProviderClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	const UApplyAnimationProvider* ProviderDefaultObject =
		Definition->ProviderClass->GetDefaultObject<UApplyAnimationProvider>();
	return ProviderDefaultObject
		&& ProviderDefaultObject->GetProviderDefinitionStruct() == ProviderDefinition.GetScriptStruct();
}

void UApplyAnimationProvider::Cancel()
{
	bCancelled = true;
}

bool UApplyAnimationProvider::BeginPlayback()
{
	if (bStarted || bFinished || bCancelled)
	{
		return false;
	}

	bStarted = true;
	return true;
}

void UApplyAnimationProvider::NotifyImpact()
{
	if (!IsPlaybackActive() || bImpactNotified)
	{
		return;
	}

	bImpactNotified = true;
	OnImpact.Broadcast();
}

void UApplyAnimationProvider::NotifyFinished()
{
	if (!IsPlaybackActive())
	{
		return;
	}

	bFinished = true;
	OnFinished.Broadcast();
}

void UApplyAnimationProvider::NotifyFailed(const EApplyAnimationProviderFailure Failure)
{
	if (!IsPlaybackActive())
	{
		return;
	}

	bFinished = true;
	OnFailed.Broadcast(Failure);
}

UScriptStruct* UApplyAnimationMockProvider::GetProviderDefinitionStruct() const
{
	return FApplyAnimationMockProviderDefinition::StaticStruct();
}

bool UApplyAnimationMockProvider::InitializeFromProviderDefinition(
	const FInstancedStruct& ProviderDefinition)
{
	const FApplyAnimationMockProviderDefinition* Definition =
		ProviderDefinition.GetPtr<FApplyAnimationMockProviderDefinition>();
	if (!Definition || Definition->ApplyDelaySeconds < 0.0f)
	{
		return false;
	}

	ApplyDelaySeconds = Definition->ApplyDelaySeconds;
	return true;
}

bool UApplyAnimationMockProvider::Start(const FApplyAnimationContext& Context)
{
	if (!BeginPlayback() || ApplyDelaySeconds < 0.0f)
	{
		return false;
	}

	if (ApplyDelaySeconds <= 0.0f)
	{
		HandleDelayElapsed();
		return true;
	}

	UWorld* World = Context.Ability ? Context.Ability->GetWorld() : nullptr;
	if (!World)
	{
		NotifyFailed(EApplyAnimationProviderFailure::InvalidContext);
		return false;
	}

	ActiveWorld = World;
	World->GetTimerManager().SetTimer(
		ApplyTimerHandle,
		this,
		&ThisClass::HandleDelayElapsed,
		ApplyDelaySeconds,
		false);
	return true;
}

void UApplyAnimationMockProvider::Cancel()
{
	Super::Cancel();
	if (UWorld* World = ActiveWorld.Get())
	{
		World->GetTimerManager().ClearTimer(ApplyTimerHandle);
	}
	ActiveWorld.Reset();
}

void UApplyAnimationMockProvider::HandleDelayElapsed()
{
	ActiveWorld.Reset();
	NotifyImpact();
	NotifyFinished();
}

UScriptStruct* UApplyAnimationNiagaraProvider::GetProviderDefinitionStruct() const
{
	return FApplyAnimationNiagaraProviderDefinition::StaticStruct();
}

bool UApplyAnimationNiagaraProvider::InitializeFromProviderDefinition(
	const FInstancedStruct& ProviderDefinition)
{
	const FApplyAnimationNiagaraProviderDefinition* Definition =
		ProviderDefinition.GetPtr<FApplyAnimationNiagaraProviderDefinition>();
	if (!Definition || !Definition->System || Definition->ImpactDelaySeconds < 0.0f)
	{
		return false;
	}

	Configuration = *Definition;
	return true;
}

bool UApplyAnimationNiagaraProvider::Start(const FApplyAnimationContext& Context)
{
	if (!BeginPlayback())
	{
		return false;
	}

	UWorld* World = Context.Ability ? Context.Ability->GetWorld() : nullptr;
	if (!World || !Configuration.System)
	{
		NotifyFailed(World
			             ? EApplyAnimationProviderFailure::InvalidConfiguration
			             : EApplyAnimationProviderFailure::InvalidContext);
		return false;
	}

	ActiveContext = Context;
	ActiveWorld = World;
	GatherTargetLocations(Context.TargetData);

	FVector SpawnWorldLocation;
	if (!ResolveWorldLocation(Configuration.SpawnLocation, SpawnWorldLocation))
	{
		NotifyFailed(EApplyAnimationProviderFailure::InvalidContext);
		return false;
	}

	NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Context.Ability,
		Configuration.System,
		SpawnWorldLocation,
		Configuration.SpawnRotation,
		Configuration.SpawnScale,
		false,
		false,
		ENCPoolMethod::None,
		false);
	if (!NiagaraComponent)
	{
		NotifyFailed(EApplyAnimationProviderFailure::PlaybackFailed);
		return false;
	}

	NiagaraComponent->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::HandleSystemFinished);
	for (const FApplyAnimationNiagaraLocationBinding& Binding : Configuration.LocationBindings)
	{
		if (!ApplyLocationBinding(*NiagaraComponent, Binding))
		{
			ReleaseNiagaraComponent();
			NotifyFailed(EApplyAnimationProviderFailure::InvalidConfiguration);
			return false;
		}
	}

	if (Configuration.ImpactMode == EApplyAnimationNiagaraImpactMode::ExportCallback)
	{
		if (Configuration.ImpactCallbackParameter.IsNone())
		{
			ReleaseNiagaraComponent();
			NotifyFailed(EApplyAnimationProviderFailure::InvalidConfiguration);
			return false;
		}
		NiagaraComponent->SetVariableObject(Configuration.ImpactCallbackParameter, this);
	}

	NiagaraComponent->Activate(true);
	if (Configuration.ImpactMode == EApplyAnimationNiagaraImpactMode::Delay)
	{
		if (Configuration.ImpactDelaySeconds <= 0.0f)
		{
			NotifyImpact();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				ImpactTimerHandle,
				this,
				&ThisClass::HandleImpactDelayElapsed,
				Configuration.ImpactDelaySeconds,
				false);
		}
	}
	return true;
}

void UApplyAnimationNiagaraProvider::Cancel()
{
	Super::Cancel();
	if (UWorld* World = ActiveWorld.Get())
	{
		World->GetTimerManager().ClearTimer(ImpactTimerHandle);
	}
	ReleaseNiagaraComponent();
	ActiveContext = FApplyAnimationContext();
	TargetLocations.Reset();
	ActiveWorld.Reset();
}

void UApplyAnimationNiagaraProvider::ReceiveParticleData_Implementation(
	const TArray<FBasicParticleData>& Data,
	UNiagaraSystem*,
	const FVector&)
{
	if (Configuration.ImpactMode == EApplyAnimationNiagaraImpactMode::ExportCallback && !Data.IsEmpty())
	{
		NotifyImpact();
	}
}

void UApplyAnimationNiagaraProvider::HandleImpactDelayElapsed()
{
	NotifyImpact();
}

void UApplyAnimationNiagaraProvider::GatherTargetLocations(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	TargetLocations.Reset();
	for (int32 DataIndex = 0; DataIndex < TargetData.Num(); ++DataIndex)
	{
		const FGameplayAbilityTargetData* Data = TargetData.Get(DataIndex);
		if (!Data)
		{
			continue;
		}

		bool bAddedActor = false;
		for (const TWeakObjectPtr<AActor>& TargetActor : Data->GetActors())
		{
			if (TargetActor.IsValid())
			{
				TargetLocations.Add(TargetActor->GetActorLocation());
				bAddedActor = true;
			}
		}

		if (!bAddedActor && Data->HasHitResult())
		{
			if (const FHitResult* HitResult = Data->GetHitResult())
			{
				TargetLocations.Add(HitResult->Location);
			}
		}
		else if (!bAddedActor)
		{
			TargetLocations.Add(Data->GetEndPoint());
		}
	}
}

bool UApplyAnimationNiagaraProvider::ResolveWorldLocations(
	const FApplyAnimationLocationReference& Reference,
	TArray<FVector>& OutLocations) const
{
	OutLocations.Reset();
	if (Reference.Source == EApplyAnimationLocationSource::AllTargets)
	{
		OutLocations = TargetLocations;
		for (FVector& Location : OutLocations)
		{
			Location += Reference.Offset;
		}
		return !OutLocations.IsEmpty();
	}

	FVector Location;
	if (!ResolveWorldLocation(Reference, Location))
	{
		return false;
	}
	OutLocations.Add(Location);
	return true;
}

bool UApplyAnimationNiagaraProvider::ResolveWorldLocation(
	const FApplyAnimationLocationReference& Reference,
	FVector& OutLocation) const
{
	switch (Reference.Source)
	{
	case EApplyAnimationLocationSource::Avatar:
		if (!ActiveContext.Avatar)
		{
			return false;
		}
		OutLocation = ActiveContext.Avatar->GetActorLocation();
		break;

	case EApplyAnimationLocationSource::PrimaryTarget:
		if (ActiveContext.PrimaryTarget)
		{
			OutLocation = ActiveContext.PrimaryTarget->GetActorLocation();
		}
		else if (!TargetLocations.IsEmpty())
		{
			OutLocation = TargetLocations[0];
		}
		else
		{
			return false;
		}
		break;

	case EApplyAnimationLocationSource::TargetAtIndex:
		if (!TargetLocations.IsValidIndex(Reference.TargetIndex))
		{
			return false;
		}
		OutLocation = TargetLocations[Reference.TargetIndex];
		break;

	case EApplyAnimationLocationSource::TargetCentroid:
		if (TargetLocations.IsEmpty())
		{
			return false;
		}
		OutLocation = FVector::ZeroVector;
		for (const FVector& TargetLocation : TargetLocations)
		{
			OutLocation += TargetLocation;
		}
		OutLocation /= TargetLocations.Num();
		break;

	case EApplyAnimationLocationSource::Explicit:
		OutLocation = Reference.ExplicitLocation;
		break;

	case EApplyAnimationLocationSource::AllTargets:
	default:
		return false;
	}

	OutLocation += Reference.Offset;
	return true;
}

bool UApplyAnimationNiagaraProvider::ResolveReferenceTransform(
	const EApplyAnimationLocationSpace Space,
	FTransform& OutTransform) const
{
	switch (Space)
	{
	case EApplyAnimationLocationSpace::VfxLocal:
		if (!NiagaraComponent)
		{
			return false;
		}
		OutTransform = NiagaraComponent->GetComponentTransform();
		return true;

	case EApplyAnimationLocationSpace::AvatarLocal:
		if (!ActiveContext.Avatar)
		{
			return false;
		}
		OutTransform = ActiveContext.Avatar->GetActorTransform();
		return true;

	case EApplyAnimationLocationSpace::PrimaryTargetLocal:
		if (!ActiveContext.PrimaryTarget)
		{
			return false;
		}
		OutTransform = ActiveContext.PrimaryTarget->GetActorTransform();
		return true;

	case EApplyAnimationLocationSpace::World:
	default:
		return false;
	}
}

bool UApplyAnimationNiagaraProvider::ApplyLocationBinding(
	UNiagaraComponent& Component,
	const FApplyAnimationNiagaraLocationBinding& Binding) const
{
	if (Binding.ParameterName.IsNone())
	{
		return false;
	}

	TArray<FVector> Locations;
	if (!ResolveWorldLocations(Binding.Location, Locations))
	{
		return false;
	}

	if (Binding.Space != EApplyAnimationLocationSpace::World)
	{
		FTransform ReferenceTransform;
		if (!ResolveReferenceTransform(Binding.Space, ReferenceTransform))
		{
			return false;
		}
		for (FVector& Location : Locations)
		{
			Location = ReferenceTransform.InverseTransformPosition(Location);
		}
	}

	if (Binding.Location.Source == EApplyAnimationLocationSource::AllTargets)
	{
		if (Binding.Space == EApplyAnimationLocationSpace::World)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
				&Component,
				Binding.ParameterName,
				Locations);
		}
		else
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
				&Component,
				Binding.ParameterName,
				Locations);
		}
		return true;
	}

	if (Binding.Space == EApplyAnimationLocationSpace::World)
	{
		Component.SetVariablePosition(Binding.ParameterName, Locations[0]);
	}
	else
	{
		Component.SetVariableVec3(Binding.ParameterName, Locations[0]);
	}
	return true;
}

void UApplyAnimationNiagaraProvider::ReleaseNiagaraComponent()
{
	if (!NiagaraComponent)
	{
		return;
	}

	NiagaraComponent->OnSystemFinished.RemoveAll(this);
	NiagaraComponent->DeactivateImmediate();
	NiagaraComponent->DestroyComponent();
	NiagaraComponent = nullptr;
}

void UApplyAnimationNiagaraProvider::HandleSystemFinished(UNiagaraComponent* FinishedComponent)
{
	if (FinishedComponent != NiagaraComponent)
	{
		return;
	}

	if (UWorld* World = ActiveWorld.Get())
	{
		World->GetTimerManager().ClearTimer(ImpactTimerHandle);
	}
	ReleaseNiagaraComponent();
	NotifyFinished();
}
