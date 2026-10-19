#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "NiagaraDataInterfaceExport.h"
#include "StructUtils/InstancedStruct.h"
#include "ApplyAnimationProvider.generated.h"

class AActor;
class UGameplayAbility;
class UNiagaraComponent;
class UNiagaraSystem;
class UWorld;

UENUM(BlueprintType)
enum class EApplyAnimationProviderFailure : uint8
{
	InvalidContext,
	InvalidConfiguration,
	PlaybackFailed
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationContext
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Apply Animation")
	TObjectPtr<UGameplayAbility> Ability = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Apply Animation")
	TObjectPtr<AActor> Avatar = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Apply Animation")
	TObjectPtr<AActor> PrimaryTarget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Apply Animation")
	FGameplayAbilityTargetDataHandle TargetData;
};

DECLARE_MULTICAST_DELEGATE(FApplyAnimationImpactSignature);
DECLARE_MULTICAST_DELEGATE(FApplyAnimationFinishedSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FApplyAnimationFailedSignature, EApplyAnimationProviderFailure);

/** Per-activation presentation driver that tells an effect task when impact and completion occur. */
UCLASS(Abstract)
class AUTOBATTLESANDBOX_API UApplyAnimationProvider : public UObject
{
	GENERATED_BODY()

public:
	static UApplyAnimationProvider* CreateProvider(
		const FInstancedStruct& ProviderDefinition,
		UObject* Outer);
	static bool IsProviderDefinitionValid(const FInstancedStruct& ProviderDefinition);

	virtual UScriptStruct* GetProviderDefinitionStruct() const PURE_VIRTUAL(
		UApplyAnimationProvider::GetProviderDefinitionStruct, return nullptr;);

	virtual bool InitializeFromProviderDefinition(const FInstancedStruct& ProviderDefinition) PURE_VIRTUAL(
		UApplyAnimationProvider::InitializeFromProviderDefinition, return false;);

	virtual bool Start(const FApplyAnimationContext& Context) PURE_VIRTUAL(
		UApplyAnimationProvider::Start, return false;);
	virtual void Cancel();

public:
	FApplyAnimationImpactSignature OnImpact;
	FApplyAnimationFinishedSignature OnFinished;
	FApplyAnimationFailedSignature OnFailed;

protected:
	bool BeginPlayback();
	void NotifyImpact();
	void NotifyFinished();
	void NotifyFailed(EApplyAnimationProviderFailure Failure);
	bool IsPlaybackActive() const { return bStarted && !bFinished && !bCancelled; }

private:
	bool bStarted = false;
	bool bImpactNotified = false;
	bool bFinished = false;
	bool bCancelled = false;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationProviderDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apply Animation")
	TSubclassOf<UApplyAnimationProvider> ProviderClass;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationMockProviderDefinition : public FApplyAnimationProviderDefinition
{
	GENERATED_BODY()

public:
	FApplyAnimationMockProviderDefinition();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation", meta = (ClampMin = "0.0"))
	float ApplyDelaySeconds = 0.0f;
};

UENUM(BlueprintType)
enum class EApplyAnimationLocationSource : uint8
{
	Avatar,
	PrimaryTarget,
	TargetAtIndex,
	AllTargets,
	TargetCentroid,
	Explicit
};

UENUM(BlueprintType)
enum class EApplyAnimationLocationSpace : uint8
{
	World,
	VfxLocal,
	AvatarLocal,
	PrimaryTargetLocal
};

UENUM(BlueprintType)
enum class EApplyAnimationNiagaraImpactMode : uint8
{
	Delay,
	ExportCallback
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationLocationReference
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	EApplyAnimationLocationSource Source = EApplyAnimationLocationSource::PrimaryTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation", meta = (ClampMin = "0"))
	int32 TargetIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FVector ExplicitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FVector Offset = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationNiagaraLocationBinding
{
	GENERATED_BODY()

public:
	/** Niagara user parameter, for example User.TargetPosition or User.TargetPositions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FApplyAnimationLocationReference Location;

	/** World uses Niagara Position; relative spaces use Vector. AllTargets uses the matching array DI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	EApplyAnimationLocationSpace Space = EApplyAnimationLocationSpace::World;
};

USTRUCT(BlueprintType)
struct AUTOBATTLESANDBOX_API FApplyAnimationNiagaraProviderDefinition : public FApplyAnimationProviderDefinition
{
	GENERATED_BODY()

public:
	FApplyAnimationNiagaraProviderDefinition();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	TObjectPtr<UNiagaraSystem> System = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FApplyAnimationLocationReference SpawnLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FRotator SpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FVector SpawnScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	TArray<FApplyAnimationNiagaraLocationBinding> LocationBindings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	EApplyAnimationNiagaraImpactMode ImpactMode = EApplyAnimationNiagaraImpactMode::Delay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation", meta = (ClampMin = "0.0"))
	float ImpactDelaySeconds = 0.0f;

	/** Object user parameter referenced by Niagara's Export Particle Data interface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apply Animation")
	FName ImpactCallbackParameter = TEXT("User.ImpactCallback");
};

/** Test and placeholder provider that emits impact and completion after a fixed delay. */
UCLASS()
class AUTOBATTLESANDBOX_API UApplyAnimationMockProvider : public UApplyAnimationProvider
{
	GENERATED_BODY()

public:
	virtual UScriptStruct* GetProviderDefinitionStruct() const override;
	virtual bool InitializeFromProviderDefinition(const FInstancedStruct& ProviderDefinition) override;
	virtual bool Start(const FApplyAnimationContext& Context) override;
	virtual void Cancel() override;

private:
	void HandleDelayElapsed();

private:
	TWeakObjectPtr<UWorld> ActiveWorld;
	FTimerHandle ApplyTimerHandle;
	float ApplyDelaySeconds = 0.0f;
};

/** Niagara presentation provider with configurable ability-context location bindings. */
UCLASS()
class AUTOBATTLESANDBOX_API UApplyAnimationNiagaraProvider
	: public UApplyAnimationProvider,
	  public INiagaraParticleCallbackHandler
{
	GENERATED_BODY()

public:
	virtual UScriptStruct* GetProviderDefinitionStruct() const override;
	virtual bool InitializeFromProviderDefinition(const FInstancedStruct& ProviderDefinition) override;
	virtual bool Start(const FApplyAnimationContext& Context) override;
	virtual void Cancel() override;
	virtual void ReceiveParticleData_Implementation(
		const TArray<FBasicParticleData>& Data,
		UNiagaraSystem* NiagaraSystem,
		const FVector& SimulationPositionOffset) override;

private:
	void HandleImpactDelayElapsed();
	void GatherTargetLocations(const FGameplayAbilityTargetDataHandle& TargetData);
	bool ResolveWorldLocations(
		const FApplyAnimationLocationReference& Reference,
		TArray<FVector>& OutLocations) const;
	bool ResolveWorldLocation(
		const FApplyAnimationLocationReference& Reference,
		FVector& OutLocation) const;
	bool ResolveReferenceTransform(EApplyAnimationLocationSpace Space, FTransform& OutTransform) const;
	bool ApplyLocationBinding(
		UNiagaraComponent& Component,
		const FApplyAnimationNiagaraLocationBinding& Binding) const;
	void ReleaseNiagaraComponent();

	UFUNCTION()
	void HandleSystemFinished(UNiagaraComponent* FinishedComponent);

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;

	UPROPERTY(Transient)
	FApplyAnimationContext ActiveContext;

	UPROPERTY(Transient)
	FApplyAnimationNiagaraProviderDefinition Configuration;

	TArray<FVector> TargetLocations;
	TWeakObjectPtr<UWorld> ActiveWorld;
	FTimerHandle ImpactTimerHandle;
};
