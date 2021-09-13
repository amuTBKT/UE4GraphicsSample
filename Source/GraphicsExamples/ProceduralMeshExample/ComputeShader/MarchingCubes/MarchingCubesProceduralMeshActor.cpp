#include "MarchingCubesProceduralMeshActor.h"

#include "MarchingCubeTables.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshProxy.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshComponent.h"
#include "ProceduralMesh/MarchingCubeShaders/MarchingCubeShaders.h"

// for using RenderGraph
#include "RenderGraph.h"
#include "RenderGraphUtils.h"

AMarchingCubesProceduralMeshActor::AMarchingCubesProceduralMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMeshComponent = CreateDefaultSubobject<UComputeShaderProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
}

void AMarchingCubesProceduralMeshActor::BeginPlay()
{
	ENQUEUE_RENDER_COMMAND(MarchingCubesProceduralMeshActor_InitializeResources)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			using namespace MarchingCubesResourceProvider;

			FNumVertsResourceArray NumVertsResourceArray = {};
			NumVerticesLookupTable.Initialize(FNumVertsResourceArray::BytesPerElement, FNumVertsResourceArray::NumElements, EPixelFormat::PF_R8_UINT, BUF_Static, TEXT("NumVerticesLookupTable"), &NumVertsResourceArray);

			FTriTableBulkData TriTableBulkData;
			FRHIResourceCreateInfo CreateInfo = FRHIResourceCreateInfo(&TriTableBulkData);
			CreateInfo.DebugName = TEXT("TriangleVerticesLookupTable");
			TriangleVerticesLookupTable.Initialize(FTriTableBulkData::BytesPerElement, FTriTableBulkData::SizeX, FTriTableBulkData::SizeY, EPixelFormat::PF_R8_UINT, TexCreate_ShaderResource, CreateInfo);
		});

	Super::BeginPlay();
}

void AMarchingCubesProceduralMeshActor::BeginDestroy()
{
	ENQUEUE_RENDER_COMMAND(MarchingCubesProceduralMeshActor_DestroyResources)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			NumVerticesLookupTable.Release();
			TriangleVerticesLookupTable.Release();
		});

	Super::BeginDestroy();
}

void AMarchingCubesProceduralMeshActor::ResetIndirectDrawArgs(FRDGBuilder& GraphBuilder, const FRTParams& RTParams, ERHIFeatureLevel::Type FeatureLevel)
{
	ensure(IsInRenderingThread());

	const FIntVector GroupCount(1, 1, 1);

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FResetIndirectArgsCS> ComputeShader(GlobalShaderMap);

	FResetIndirectArgsCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FResetIndirectArgsCS::FParameters>();
	PassParameters->IndirectDrawArgsBuffer = RTParams.SceneProxy->GetIndirectDrawArgsBuffer().UAV;

	ValidateShaderParameters(ComputeShader, *PassParameters);

	// add our pass
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("ResetIndirectDrawArgs"),
		ComputeShader, PassParameters, GroupCount);
}

void AMarchingCubesProceduralMeshActor::GenerateTriangles(FRDGBuilder& GraphBuilder, const FRTParams& RTParams, ERHIFeatureLevel::Type FeatureLevel)
{
	ensure(IsInRenderingThread());

	const FIntVector GroupCount(
		FMath::CeilToInt((float)VoxelCount.X / (float)FGenerateTrianglesCS::ThreadGroupSizeX),
		FMath::CeilToInt((float)VoxelCount.Y / (float)FGenerateTrianglesCS::ThreadGroupSizeY),
		FMath::CeilToInt((float)VoxelCount.Z / (float)FGenerateTrianglesCS::ThreadGroupSizeZ));

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FGenerateTrianglesCS> ComputeShader(GlobalShaderMap);

	FGenerateTrianglesCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FGenerateTrianglesCS::FParameters>();
	PassParameters->PositionBuffer = RTParams.SceneProxy->GetPositionBuffer().UAV;
	PassParameters->TangentBuffer = RTParams.SceneProxy->GetTangentBuffer().UAV;
	PassParameters->IndirectDrawArgsBuffer = RTParams.SceneProxy->GetIndirectDrawArgsBuffer().UAV;
	PassParameters->NumVerticesLookupTable = NumVerticesLookupTable.SRV;
	PassParameters->TriangleVerticesLookupTable = TriangleVerticesLookupTable.SRV;
	PassParameters->VoxelCount = VoxelCount;
	PassParameters->VoxelSize = VoxelSize;
	PassParameters->IsoValue = RTParams.IsoValue;
	PassParameters->Time = RTParams.Time;
	PassParameters->MaxVertices = RTParams.SceneProxy->GetMaxPrimitiveCount() * 3;

	ValidateShaderParameters(ComputeShader, *PassParameters);

	// add our pass
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GenerateTriangles"),
		ComputeShader, PassParameters, GroupCount);
}

void AMarchingCubesProceduralMeshActor::Tick(float DeltaSeconds)
{
	const auto FeatureLevel = GetWorld()->Scene->GetFeatureLevel();

	FCSProceduralMeshSceneProxy* SceneProxy = (FCSProceduralMeshSceneProxy*)ProceduralMeshComponent->SceneProxy;
	
	FRTParams RTParams = {};
	RTParams.SceneProxy = SceneProxy;
	RTParams.IsoValue = IsoValue;
	RTParams.Time = AnimationTime;

	ENQUEUE_RENDER_COMMAND(MarchingCubesProceduralMeshActor_GenerateTriangles)(
		[this, RTParams, FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{		
			if (RTParams.SceneProxy && RTParams.SceneProxy->HasValidBuffers())
			{
				FMemMark Mark(FMemStack::Get());
				FRDGBuilder GraphBuilder(RHICmdList);

				ResetIndirectDrawArgs(GraphBuilder, RTParams, FeatureLevel);

				GenerateTriangles(GraphBuilder, RTParams, FeatureLevel);

				GraphBuilder.Execute();
			}
		});

	AnimationTime += DeltaSeconds;

	Super::Tick(DeltaSeconds);
}