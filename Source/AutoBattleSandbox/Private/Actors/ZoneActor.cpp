// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ZoneActor.h"

#include "AbilitySystem/BattleAbilitySystem.h"
#include "Actors/UnitActor.h"
#include "Components/BoxComponent.h"

// Sets default values
AZoneActor::AZoneActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComponent);

	BattleAbilityComponent = CreateDefaultSubobject<UBattleAbilitySystem>(TEXT("BattleAbilityComponent"));
}

// Called when the game starts or when spawned
void AZoneActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AZoneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLayoutAnimations(DeltaTime);
}

bool AZoneActor::AddUnit(AUnitActor* UnitActor)
{
	if (!IsValid(UnitActor))
	{
		return false;
	}

	if (ContainsUnit(UnitActor))
	{
		return false;
	}

	if (!HasFreeSlot())
	{
		return false;
	}

	if (UnitActor == PreviewActor)
	{
		PreviewActor = nullptr;
		PreviewSlotIndex = INDEX_NONE;
		OnPreviewUnitChanged.Broadcast(this, nullptr, INDEX_NONE);
	}

	UnitActors.Add(UnitActor);
	OnUnitAdded.Broadcast(this, UnitActor);
	RefreshUnitLayout(bAnimateLayoutChanges);
	return true;
}

bool AZoneActor::RemoveUnit(AUnitActor* UnitActor)
{
	if (!IsValid(UnitActor))
	{
		return false;
	}

	const int32 RemovedCount = UnitActors.Remove(UnitActor);
	if (RemovedCount <= 0)
	{
		return false;
	}

	const int32 AnimationIndex = FindAnimationIndexForActor(UnitActor);
	if (AnimationIndex != INDEX_NONE)
	{
		ActiveLayoutAnimations.RemoveAtSwap(AnimationIndex);
	}

	OnUnitRemoved.Broadcast(this, UnitActor);
	RefreshUnitLayout(bAnimateLayoutChanges);
	return true;
}

bool AZoneActor::ContainsUnit(const AUnitActor* UnitActor) const
{
	return IsValid(UnitActor) && UnitActors.Contains(UnitActor);
}

bool AZoneActor::SetPreviewUnit(AUnitActor* UnitActor, int32 SlotIndex)
{
	if (!IsValid(UnitActor))
	{
		return false;
	}

	if (ContainsUnit(UnitActor))
	{
		return false;
	}

	if (MaxUnitCount > 0 && UnitActors.Num() >= MaxUnitCount)
	{
		return false;
	}

	const int32 LastPreviewSlotIndex = UnitActors.Num();
	PreviewActor = UnitActor;
	PreviewSlotIndex = SlotIndex == INDEX_NONE
		                   ? LastPreviewSlotIndex
		                   : FMath::Clamp(SlotIndex, 0, LastPreviewSlotIndex);

	OnPreviewUnitChanged.Broadcast(this, PreviewActor, PreviewSlotIndex);
	RefreshUnitLayout(bAnimateLayoutChanges);
	return true;
}

void AZoneActor::ClearPreviewUnit()
{
	if (!PreviewActor && PreviewSlotIndex == INDEX_NONE)
	{
		return;
	}

	if (IsValid(PreviewActor))
	{
		if (const int32 AnimationIndex = FindAnimationIndexForActor(PreviewActor); AnimationIndex != INDEX_NONE)
		{
			ActiveLayoutAnimations.RemoveAtSwap(AnimationIndex);
		}
	}

	PreviewActor = nullptr;
	PreviewSlotIndex = INDEX_NONE;

	OnPreviewUnitChanged.Broadcast(this, nullptr, INDEX_NONE);
	RefreshUnitLayout(bAnimateLayoutChanges);
}

bool AZoneActor::CommitPreviewUnit()
{
	AUnitActor* UnitActor = PreviewActor;
	if (!IsValid(UnitActor))
	{
		return false;
	}

	PreviewActor = nullptr;
	PreviewSlotIndex = INDEX_NONE;
	OnPreviewUnitChanged.Broadcast(this, nullptr, INDEX_NONE);

	return AddUnit(UnitActor);
}

int32 AZoneActor::GetUnitCount() const
{
	return UnitActors.Num();
}

bool AZoneActor::HasFreeSlot() const
{
	return MaxUnitCount <= 0 || UnitActors.Num() < MaxUnitCount;
}

void AZoneActor::RefreshUnitLayout(bool bAnimate)
{
	TArray<AActor*> ActorsToLayout;
	ActorsToLayout.Reserve(UnitActors.Num() + (IsValid(PreviewActor) ? 1 : 0));

	for (AUnitActor* UnitActor : UnitActors)
	{
		if (IsValid(UnitActor))
		{
			ActorsToLayout.Add(UnitActor);
		}
	}

	if (IsValid(PreviewActor))
	{
		const int32 ClampedPreviewSlotIndex = FMath::Clamp(PreviewSlotIndex, 0, ActorsToLayout.Num());
		ActorsToLayout.Insert(PreviewActor, ClampedPreviewSlotIndex);
	}

	for (int32 Index = 0; Index < ActorsToLayout.Num(); ++Index)
	{
		MoveActorToTransform(ActorsToLayout[Index], GetSlotTransform(Index, ActorsToLayout.Num()), bAnimate);
	}

	OnLayoutChanged.Broadcast(this);
}

FTransform AZoneActor::GetSlotTransform(int32 SlotIndex, int32 TotalSlots) const
{
	const int32 ClampedTotalSlots = FMath::Max(TotalSlots, 0);
	const float StartOffset = bCenterUnits
		                          ? -static_cast<float>(ClampedTotalSlots - 1) * SpaceBetweenUnits * 0.5f
		                          : 0.0f;
	const FVector LocalLocation = LocalLayoutOffset + FVector(
		0.0f, StartOffset + static_cast<float>(SlotIndex) * SpaceBetweenUnits, 0.0f);

	return FTransform(GetActorRotation(), GetActorTransform().TransformPosition(LocalLocation), FVector::OneVector);
}

void AZoneActor::MoveActorToTransform(AActor* Actor, const FTransform& TargetTransform, bool bAnimate)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const int32 AnimationIndex = FindAnimationIndexForActor(Actor);
	if (!bAnimate || LayoutAnimationDuration <= 0.0f)
	{
		if (AnimationIndex != INDEX_NONE)
		{
			ActiveLayoutAnimations.RemoveAtSwap(AnimationIndex);
		}

		Actor->SetActorTransform(TargetTransform);
		return;
	}

	FLayoutAnimation LayoutAnimation;
	LayoutAnimation.Actor = Actor;
	LayoutAnimation.SourceTransform = Actor->GetActorTransform();
	LayoutAnimation.TargetTransform = TargetTransform;
	LayoutAnimation.ElapsedTime = 0.0f;

	if (AnimationIndex != INDEX_NONE)
	{
		ActiveLayoutAnimations[AnimationIndex] = LayoutAnimation;
	}
	else
	{
		ActiveLayoutAnimations.Add(LayoutAnimation);
	}
}

void AZoneActor::UpdateLayoutAnimations(float DeltaTime)
{
	for (int32 Index = ActiveLayoutAnimations.Num() - 1; Index >= 0; --Index)
	{
		FLayoutAnimation& LayoutAnimation = ActiveLayoutAnimations[Index];
		AActor* Actor = LayoutAnimation.Actor.Get();
		if (!IsValid(Actor))
		{
			ActiveLayoutAnimations.RemoveAtSwap(Index);
			continue;
		}

		LayoutAnimation.ElapsedTime += DeltaTime;
		const float Alpha = LayoutAnimationDuration <= 0.0f
			                    ? 1.0f
			                    : FMath::Clamp(LayoutAnimation.ElapsedTime / LayoutAnimationDuration, 0.0f, 1.0f);
		const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

		const FVector Location = FMath::Lerp(LayoutAnimation.SourceTransform.GetLocation(),
		                                     LayoutAnimation.TargetTransform.GetLocation(), SmoothAlpha);
		const FQuat Rotation = FQuat::Slerp(LayoutAnimation.SourceTransform.GetRotation(),
		                                    LayoutAnimation.TargetTransform.GetRotation(), SmoothAlpha);
		const FVector Scale = FMath::Lerp(LayoutAnimation.SourceTransform.GetScale3D(),
		                                  LayoutAnimation.TargetTransform.GetScale3D(), SmoothAlpha);

		Actor->SetActorTransform(FTransform(Rotation, Location, Scale));

		if (Alpha >= 1.0f)
		{
			Actor->SetActorTransform(LayoutAnimation.TargetTransform);
			ActiveLayoutAnimations.RemoveAtSwap(Index);
		}
	}
}

int32 AZoneActor::FindAnimationIndexForActor(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < ActiveLayoutAnimations.Num(); ++Index)
	{
		if (ActiveLayoutAnimations[Index].Actor.Get() == Actor)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}
