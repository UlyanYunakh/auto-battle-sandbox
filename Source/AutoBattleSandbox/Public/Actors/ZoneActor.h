// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneActor.generated.h"

class UBattleAbilitySystem;
class UBoxComponent;
class AZoneActor;
class AUnitActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZoneUnitChangedSignature, AZoneActor*, ZoneActor, AUnitActor*, UnitActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FZonePreviewChangedSignature, AZoneActor*, ZoneActor, AUnitActor*,
                                               PreviewActor, int32, SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZoneLayoutChangedSignature, AZoneActor*, ZoneActor);

UCLASS()
class AUTOBATTLESANDBOX_API AZoneActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AZoneActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Zone")
	bool AddUnit(AUnitActor* UnitActor);

	UFUNCTION(BlueprintCallable, Category = "Zone")
	bool RemoveUnit(AUnitActor* UnitActor);

	UFUNCTION(BlueprintPure, Category = "Zone")
	bool ContainsUnit(const AUnitActor* UnitActor) const;

	UFUNCTION(BlueprintCallable, Category = "Zone|Preview")
	bool SetPreviewUnit(AUnitActor* UnitActor, int32 SlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "Zone|Preview")
	void ClearPreviewUnit();

	UFUNCTION(BlueprintCallable, Category = "Zone|Preview")
	bool CommitPreviewUnit();

	UFUNCTION(BlueprintPure, Category = "Zone")
	int32 GetUnitCount() const;

	UFUNCTION(BlueprintPure, Category = "Zone")
	bool HasFreeSlot() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	void RefreshUnitLayout(bool bAnimate);
	FTransform GetSlotTransform(int32 SlotIndex, int32 TotalSlots) const;
	void MoveActorToTransform(AActor* Actor, const FTransform& TargetTransform, bool bAnimate);
	void UpdateLayoutAnimations(float DeltaTime);
	int32 FindAnimationIndexForActor(const AActor* Actor) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BoxComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleAbilitySystem> BattleAbilityComponent = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Layout")
	int32 MaxUnitCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Layout")
	float SpaceBetweenUnits = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Layout")
	FVector LocalLayoutOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Layout")
	bool bCenterUnits = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Animation")
	bool bAnimateLayoutChanges = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Animation", meta = (ClampMin = "0.0"))
	float LayoutAnimationDuration = 0.25f;

public:
	UPROPERTY(BlueprintAssignable, Category = "Zone|Events")
	FZoneUnitChangedSignature OnUnitAdded;

	UPROPERTY(BlueprintAssignable, Category = "Zone|Events")
	FZoneUnitChangedSignature OnUnitRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Zone|Events")
	FZonePreviewChangedSignature OnPreviewUnitChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zone|Events")
	FZoneLayoutChangedSignature OnLayoutChanged;

private:
	UPROPERTY(BlueprintReadOnly, Category = "Zone", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AUnitActor>> UnitActors;

	UPROPERTY(BlueprintReadOnly, Category = "Zone|Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AUnitActor> PreviewActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Zone|Preview", meta = (AllowPrivateAccess = "true"))
	int32 PreviewSlotIndex = INDEX_NONE;

private:
	struct FLayoutAnimation
	{
		TWeakObjectPtr<AActor> Actor;
		FTransform SourceTransform = FTransform::Identity;
		FTransform TargetTransform = FTransform::Identity;
		float ElapsedTime = 0.0f;
	};

	TArray<FLayoutAnimation> ActiveLayoutAnimations;
};
