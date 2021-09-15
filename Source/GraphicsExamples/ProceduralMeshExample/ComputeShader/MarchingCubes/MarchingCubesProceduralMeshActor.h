#pragma once

#include "RHI.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshActor.h"
#include "MarchingCubesProceduralMeshActor.generated.h"

class FRDGBuilder;
class FRHICommandListImmediate;

UCLASS()
class GRAPHICSEXAMPLES_API AMarchingCubesProceduralMeshActor : public AComputeShaderProceduralMeshActor
{
	GENERATED_BODY()

public:
	AMarchingCubesProceduralMeshActor();

	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void OnCreatedSceneProxyRendererResources(FCSProceduralMeshSceneProxy* SceneProxy) override;
	virtual void BeginDestroy() override;
	
private:
	struct FRTParams
	{
		FCSProceduralMeshSceneProxy* SceneProxy = nullptr;
		float IsoValue = 0.f;
		float Time = 0.f;
	};
	void TickSceneProxy_RT(FRHICommandListImmediate& RHICmdList, const FRTParams& RTParams);
	void ResetIndirectDrawArgs(FRDGBuilder& GraphBuilder, const FRTParams& RTParams);
	void GenerateTriangles(FRDGBuilder& GraphBuilder, const FRTParams& RTParams);

public:	
	UPROPERTY(EditAnywhere, Category = "Voxels")
	FIntVector VoxelCount = FIntVector(32, 32, 32);

	UPROPERTY(EditAnywhere, Category = "Voxels")
	float VoxelSize = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadwrite, Category = "Voxels")
	float IsoValue = 0.1;

private:
	// GPU readonly data, these can be global params since they are same for all the actors
	FReadBuffer				NumVerticesLookupTable;
	FTextureReadBuffer2D	TriangleVerticesLookupTable;

	float AnimationTime = 0.f;
};