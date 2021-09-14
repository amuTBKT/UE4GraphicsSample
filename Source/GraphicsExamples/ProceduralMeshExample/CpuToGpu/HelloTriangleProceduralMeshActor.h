#pragma once

#include "ProceduralMesh/ComputeShaderProceduralMeshActor.h"
#include "HelloTriangleProceduralMeshActor.generated.h"

UCLASS()
class GRAPHICSEXAMPLES_API AHelloTriangleProceduralMeshActor : public AComputeShaderProceduralMeshActor
{
	GENERATED_BODY()

public:
	AHelloTriangleProceduralMeshActor();

private:
	virtual void OnCreatedSceneProxyRendererResources(FCSProceduralMeshSceneProxy* SceneProxy) override;

public:
	UPROPERTY(EditAnywhere, Category="ProceduralMesh")
	float TriangleSize = 128;
};