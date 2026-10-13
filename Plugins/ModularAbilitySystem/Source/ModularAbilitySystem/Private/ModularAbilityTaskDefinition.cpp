#include "ModularAbilityTaskDefinition.h"

#include "ModularAbilityTask.h"

void FModularAbilityTaskDefinition::SynchronizeParameters()
{
	const UModularAbilityTask* TaskDefaultObject = TaskClass
		                                               ? TaskClass->GetDefaultObject<UModularAbilityTask>()
		                                               : nullptr;
	const UScriptStruct* ExpectedParametersStruct = TaskDefaultObject
		                                                ? TaskDefaultObject->GetTaskParametersStruct()
		                                                : nullptr;

	if (!ExpectedParametersStruct)
	{
		Parameters.Reset();
		return;
	}

	if (Parameters.GetScriptStruct() != ExpectedParametersStruct)
	{
		Parameters.InitializeAs(ExpectedParametersStruct);
	}
}

bool FModularAbilityTaskDefinition::IsValid() const
{
	if (!TaskClass || TaskClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}

	const UModularAbilityTask* TaskDefaultObject = TaskClass->GetDefaultObject<UModularAbilityTask>();
	const UScriptStruct* ExpectedParametersStruct = TaskDefaultObject
		                                                ? TaskDefaultObject->GetTaskParametersStruct()
		                                                : nullptr;
	return ExpectedParametersStruct && Parameters.GetScriptStruct() == ExpectedParametersStruct;
}
