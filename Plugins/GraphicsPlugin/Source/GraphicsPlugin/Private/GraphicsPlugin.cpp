#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FGraphicsPlugin : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FGraphicsPlugin, GraphicsPlugin)

/**
* Shaders needs to be loaded before engine is initialized, this Plugin makes sures we add our shaders into ShaderMap
*/
void FGraphicsPlugin::StartupModule()
{
	const FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("GraphicsPlugin"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/GraphicsPlugin"), PluginShaderDir);
}


void FGraphicsPlugin::ShutdownModule()
{
}