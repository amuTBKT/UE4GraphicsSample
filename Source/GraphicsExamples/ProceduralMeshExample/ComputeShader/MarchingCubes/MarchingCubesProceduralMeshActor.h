#pragma once

#include "RHI.h"
#include "GameFramework/Actor.h"
#include "MarchingCubesProceduralMeshActor.generated.h"

class UComputeShaderProceduralMeshComponent;
class FCSProceduralMeshSceneProxy;
class FRDGBuilder;

UCLASS()
class GRAPHICSEXAMPLES_API AMarchingCubesProceduralMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AMarchingCubesProceduralMeshActor();

	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

private:
	struct FRTParams
	{
		FCSProceduralMeshSceneProxy* SceneProxy = nullptr;
		float IsoValue = 0.f;
		float Time = 0.f;
	};
	void ResetIndirectDrawArgs(FRDGBuilder& GraphBuilder, const FRTParams& RTParams, ERHIFeatureLevel::Type FeatureLevel);
	void GenerateTriangles(FRDGBuilder& GraphBuilder, const FRTParams& RTParams, ERHIFeatureLevel::Type FeatureLevel);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxels")
	UComputeShaderProceduralMeshComponent* ProceduralMeshComponent = nullptr;
	
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