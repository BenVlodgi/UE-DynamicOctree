// Copyright 2024 Dream Seed LLC.

#pragma once

#include "Modules/ModuleManager.h"

class FDynamicOctreeRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
