#include "AbilitySystem/Tasks/AbilityTask_ApplyGameplayEffectWithAnimation.h"

#include "AbilitySystem/Abilities/BattleGameplayAbility.h"
#include "AbilitySystem/Animation/ApplyAnimationProvider.h"
#include "GameplayEffect.h"

UAbilityTask_ApplyGameplayEffectWithAnimation*
UAbilityTask_ApplyGameplayEffectWithAnimation::ApplyGameplayEffectWithAnimation(
	UBattleGameplayAbility* OwningAbility,
	FGameplayAbilityTargetDataHandle TargetData,
	const TSubclassOf<UGameplayEffect> GameplayEffectClass,
	FInstancedStruct AnimationProvider,
	const float GameplayEffectLevel,
	const int32 Stacks)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_ApplyGameplayEffectWithAnimation* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectWithAnimation>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectClass = GameplayEffectClass;
	Task->AnimationProviderDefinition = MoveTemp(AnimationProvider);
	Task->GameplayEffectLevel = GameplayEffectLevel;
	Task->Stacks = Stacks;
	return Task;
}

UAbilityTask_ApplyGameplayEffectWithAnimation*
UAbilityTask_ApplyGameplayEffectWithAnimation::ApplyGameplayEffectSpecWithAnimation(
	UBattleGameplayAbility* OwningAbility,
	FGameplayAbilityTargetDataHandle TargetData,
	FGameplayEffectSpecHandle GameplayEffectSpec,
	FInstancedStruct AnimationProvider)
{
	if (!OwningAbility)
	{
		return nullptr;
	}

	UAbilityTask_ApplyGameplayEffectWithAnimation* Task =
		NewAbilityTask<UAbilityTask_ApplyGameplayEffectWithAnimation>(OwningAbility);
	Task->InitializeModularTask(*OwningAbility, OwningAbility->GetAbilityContext());
	Task->TargetData = MoveTemp(TargetData);
	Task->GameplayEffectSpec = MoveTemp(GameplayEffectSpec);
	Task->AnimationProviderDefinition = MoveTemp(AnimationProvider);
	Task->bUseGameplayEffectSpec = true;
	return Task;
}

UScriptStruct* UAbilityTask_ApplyGameplayEffectWithAnimation::GetTaskParametersStruct() const
{
	return FApplyGameplayEffectWithAnimationTaskParameters::StaticStruct();
}

bool UAbilityTask_ApplyGameplayEffectWithAnimation::InitializeFromTaskParameters(
	const FInstancedStruct& Parameters)
{
	const FApplyGameplayEffectWithAnimationTaskParameters* TaskParameters =
		Parameters.GetPtr<FApplyGameplayEffectWithAnimationTaskParameters>();
	if (!TaskParameters)
	{
		return false;
	}

	TargetData = CachedAbilityContext.TargetData;
	GameplayEffectClass = TaskParameters->GameplayEffectClass;
	GameplayEffectSpec = FGameplayEffectSpecHandle();
	AnimationProviderDefinition = TaskParameters->AnimationProvider;
	GameplayEffectLevel = TaskParameters->GameplayEffectLevel;
	Stacks = TaskParameters->Stacks;
	bUseGameplayEffectSpec = false;
	PopulateTargetDataFromActivationInput();
	return GameplayEffectClass != nullptr
		&& UApplyAnimationProvider::IsProviderDefinitionValid(AnimationProviderDefinition)
		&& GameplayEffectLevel >= 0.0f && Stacks > 0;
}

bool UAbilityTask_ApplyGameplayEffectWithAnimation::ActivateModularAbilityTask()
{
	if (bFinished)
	{
		return true;
	}

	UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidAbility);
		return true;
	}

	if (!TargetData.IsValid(0))
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidTargetData);
		return true;
	}

	if (bUseGameplayEffectSpec)
	{
		if (!GameplayEffectSpec.IsValid())
		{
			FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidGameplayEffect);
			return true;
		}
	}
	else if (!GameplayEffectClass || GameplayEffectClass->HasAnyClassFlags(CLASS_Abstract)
		|| GameplayEffectLevel < 0.0f || Stacks <= 0)
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidGameplayEffect);
		return true;
	}

	const FGameplayAbilityActorInfo* ActorInfo = BattleAbility->GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidActorInfo);
		return true;
	}

	if (!UApplyAnimationProvider::IsProviderDefinitionValid(AnimationProviderDefinition))
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidAnimationProvider);
		return true;
	}

	ActiveAnimationProvider = UApplyAnimationProvider::CreateProvider(AnimationProviderDefinition, this);
	if (!ActiveAnimationProvider)
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::InvalidAnimationProvider);
		return true;
	}

	ActiveAnimationProvider->OnImpact.AddUObject(this, &ThisClass::HandleAnimationImpact);
	ActiveAnimationProvider->OnFinished.AddUObject(this, &ThisClass::HandleAnimationFinished);
	ActiveAnimationProvider->OnFailed.AddUObject(this, &ThisClass::HandleAnimationFailed);

	FApplyAnimationContext AnimationContext;
	AnimationContext.Ability = BattleAbility;
	AnimationContext.Avatar = ActorInfo->AvatarActor.Get();
	AnimationContext.PrimaryTarget = FindPrimaryTarget();
	AnimationContext.TargetData = TargetData;
	if (!ActiveAnimationProvider->Start(AnimationContext) && !bFinished)
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::AnimationStartFailed);
	}
	return true;
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::OnDestroy(const bool bAbilityEnding)
{
	ReleaseAnimationProvider();
	Super::OnDestroy(bAbilityEnding);
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::HandleAnimationImpact()
{
	if (bFinished || bEffectTriggered)
	{
		return;
	}

	bEffectTriggered = true;
	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		PendingFailure = EApplyGameplayEffectWithAnimationFailure::InvalidAbility;
		return;
	}

	if (bUseGameplayEffectSpec)
	{
		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectSpecToTargetData(TargetData, GameplayEffectSpec);
	}
	else
	{
		ActiveEffectHandles = BattleAbility->ApplyGameplayEffectClassToTargetData(
			TargetData,
			GameplayEffectClass,
			GameplayEffectLevel,
			Stacks);
	}

	if (ActiveEffectHandles.IsEmpty())
	{
		PendingFailure = EApplyGameplayEffectWithAnimationFailure::ApplicationRejected;
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnEffectApplied.Broadcast(TargetData, ActiveEffectHandles);
	}
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::HandleAnimationFinished()
{
	if (bFinished)
	{
		return;
	}

	if (PendingFailure != EApplyGameplayEffectWithAnimationFailure::None)
	{
		FinishFailed(PendingFailure);
		return;
	}

	if (!bEffectTriggered)
	{
		FinishFailed(EApplyGameplayEffectWithAnimationFailure::EffectNotTriggered);
		return;
	}

	CompleteSuccessfully();
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::HandleAnimationFailed(
	const EApplyAnimationProviderFailure)
{
	FinishFailed(EApplyGameplayEffectWithAnimationFailure::AnimationFailed);
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::FinishFailed(
	const EApplyGameplayEffectWithAnimationFailure Failure)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	ReleaseAnimationProvider();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFailed.Broadcast(Failure);
	}
	CancelModularAbilityTask();
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::CompleteSuccessfully()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	ReleaseAnimationProvider();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast(TargetData, ActiveEffectHandles);
	}

	FModularGameplayAbilityContext UpdatedContext = CachedAbilityContext;
	UpdatedContext.TargetData = TargetData;
	UpdatedContext.Target = FindPrimaryTarget();
	CompleteModularAbilityTask(UpdatedContext);
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::PopulateTargetDataFromActivationInput()
{
	if (TargetData.IsValid(0))
	{
		return;
	}

	const UBattleGameplayAbility* BattleAbility = Cast<UBattleGameplayAbility>(Ability);
	if (!BattleAbility)
	{
		return;
	}

	const FAbilityTargetSelectionInput& TargetingInput = BattleAbility->GetActivationTargetingInput();
	TargetData = TargetingInput.ProvidedTargetData;
	if (TargetData.IsValid(0))
	{
		return;
	}

	FGameplayAbilityTargetData_ActorArray* ActorData = nullptr;
	for (AActor* TargetActor : TargetingInput.ProvidedActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		if (!ActorData)
		{
			ActorData = new FGameplayAbilityTargetData_ActorArray();
		}
		ActorData->TargetActorArray.AddUnique(TargetActor);
	}

	if (ActorData)
	{
		TargetData.Add(ActorData);
	}
}

AActor* UAbilityTask_ApplyGameplayEffectWithAnimation::FindPrimaryTarget() const
{
	for (int32 DataIndex = 0; DataIndex < TargetData.Num(); ++DataIndex)
	{
		const FGameplayAbilityTargetData* Data = TargetData.Get(DataIndex);
		if (!Data)
		{
			continue;
		}

		for (const TWeakObjectPtr<AActor>& TargetActor : Data->GetActors())
		{
			if (TargetActor.IsValid())
			{
				return TargetActor.Get();
			}
		}
	}
	return const_cast<AActor*>(CachedAbilityContext.Target.Get());
}

void UAbilityTask_ApplyGameplayEffectWithAnimation::ReleaseAnimationProvider()
{
	if (!ActiveAnimationProvider)
	{
		return;
	}

	ActiveAnimationProvider->OnImpact.RemoveAll(this);
	ActiveAnimationProvider->OnFinished.RemoveAll(this);
	ActiveAnimationProvider->OnFailed.RemoveAll(this);
	ActiveAnimationProvider->Cancel();
	ActiveAnimationProvider = nullptr;
}
