#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Components/SplineMeshComponent.h"

#include "GraphicsPluginFunctionLibrary.generated.h"

class UStaticMesh;

UCLASS()
class GRAPHICSPLUGINEDITOR_API UGraphicsPluginEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SplineMesh")
	static bool CalculateSplineParameters(
		UStaticMesh* StaticMesh,
		ESplineMeshAxis::Type ForwardAxis,
		UPARAM(ref) FVector& OutSplineMeshDir,
		UPARAM(ref) FVector& OutSplineMeshX,
		UPARAM(ref) FVector& OutSplineMeshY,
		UPARAM(ref) float& OutSplineMeshScaleZ,
		UPARAM(ref) float& OutSplineMeshMinZ);

	UFUNCTION(BlueprintCallable, Category = "EditorUtils")
	static void CopyTextToClipboard(const FString& Text);
};