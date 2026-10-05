// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedMVVM/Definitions/ViewModelDefinitions.h"
#include "AdvancedMVVM/Viewmodels/BaseViewModel.h"
#include "UnitViewModel.generated.h"

struct FOnAttributeChangeData;
class UTexture2D;
class UUnitAsset;
class AUnitActor;

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FUnitViewModelProperties : public FDefaultViewModelProperty
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	TObjectPtr<UUnitAsset> UnitAsset = nullptr;
};

UCLASS(BlueprintType, Blueprintable)
class AUTOBATTLESANDBOX_API UUnitViewModel : public UBaseViewModel
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetUnitActor(AUnitActor* InUnitActor);

	UFUNCTION(BlueprintPure, Category = "Unit")
	AUnitActor* GetUnitActor() const { return UnitActor; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void InitializeFromAsset(const UUnitAsset* InUnitAsset);

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void RefreshFromUnitActor();

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Unit")
	FText UnitName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Unit")
	TSoftObjectPtr<UTexture2D> UnitArt;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	FText HealthText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	float Attack = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	FText AttackText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	float Armor = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Attributes")
	FText ArmorText;

protected:
	virtual void ViewModelInitialize(AActor* InActor) override;
	virtual void ViewModelClear() override;
	virtual void SetupProperties(const FInstancedStruct& InData) override;
	virtual bool IsSupportedStruct(const UScriptStruct* InData) const override;

private:
	void SetUnitName(const FText& InUnitName);
	void SetUnitArt(const TSoftObjectPtr<UTexture2D>& InUnitArt);
	void SetHealth(float InHealth);
	void SetAttack(float InAttack);
	void SetArmor(float InArmor);
	void BindAttributeChangeDelegates();
	void UnbindAttributeChangeDelegates();
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleAttackChanged(const FOnAttributeChangeData& Data);
	void HandleArmorChanged(const FOnAttributeChangeData& Data);
	void ClearUnit();

private:
	UPROPERTY()
	TObjectPtr<AUnitActor> UnitActor = nullptr;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle AttackChangedHandle;
	FDelegateHandle ArmorChangedHandle;
};
