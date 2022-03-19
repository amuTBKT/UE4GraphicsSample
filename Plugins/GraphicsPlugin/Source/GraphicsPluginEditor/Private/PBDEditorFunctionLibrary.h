#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBDEditorFunctionLibrary.generated.h"

UCLASS()
class GRAPHICSPLUGINEDITOR_API UPBDEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 *	Generate position and constraint textures used with PBD sim
	 *	@param	ParticlesFilePath	Absolute disk path for .poly file.
	 *	@param	PackagePath			Output directory to save the generated textures.
	 *	@param	BaseFileName		Texture will be saved using this as Prefix. {BaseFileName}_****.uasset.
	 *	@param	DistanceThreshold	Distance threshold to consider particles neighbors.
	 */
	UFUNCTION(BlueprintCallable, Category = "PBD")
	static bool GeneratePBDShapeMatchingTextures(
		const FString& ParticlesFilePath,
		const FString& PackagePath,
		const FString& BaseFileName,
		float DistanceThreshold);
};