#include "ProceduralMesh/ComputeShaderProceduralMeshProxy.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshComponent.h"

FCSProceduralMeshSceneProxy::FCSProceduralMeshSceneProxy(const UComputeShaderProceduralMeshComponent* InComponent)
: FPrimitiveSceneProxy(InComponent)
, RenderMaterial(InComponent->GetMaterial(0))
, VertexFactory(GetScene().GetFeatureLevel(), "FCSProceduralMeshSceneProxy")
, MaxPrimitiveCount(InComponent->GetMaxPrimitiveCount())
{
	checkf(RenderMaterial, TEXT("Material can not be null"));
	MaterialRelevance = RenderMaterial->GetRelevance_Concurrent(GetScene().GetFeatureLevel());
}

FCSProceduralMeshSceneProxy::~FCSProceduralMeshSceneProxy()
{		
}

void FCSProceduralMeshSceneProxy::CreateRenderThreadResources()
{
	// initialize buffers used by Graphics and Compute shaders
	const uint32 VertexCount = MaxPrimitiveCount * 3;
	PositionBuffer.Initialize(sizeof(float), 3 * VertexCount, PF_R32_FLOAT, BUF_Static, TEXT("PositionBuffer"));
	TangentBuffer.Initialize(sizeof(FPackedNormal), 2 * VertexCount, PF_R8G8B8A8_SNORM, BUF_Static, TEXT("TangentBuffer"));
	TexCoordBuffer.Initialize(sizeof(FVector2D), VertexCount, PF_G32R32F, BUF_Static, TEXT("TexCoordBuffer"));
	ColorBuffer.Initialize(sizeof(FColor), VertexCount, PF_R8G8B8A8, BUF_Static, TEXT("ColorBuffer"));
	IndirectDrawArgs.Initialize(sizeof(uint32), sizeof(FRHIDrawIndirectParameters) / sizeof(uint32), PF_R32_UINT, BUF_DrawIndirect | BUF_Static, TEXT("IndirectDrawArgs"));

	// make sure to reset this so that we don't use garbage data
	if (FRHIDrawIndirectParameters* ArgParams = (FRHIDrawIndirectParameters*)RHILockVertexBuffer(IndirectDrawArgs.VertexBufferRHI, 0, sizeof(FRHIDrawIndirectParameters), RLM_WriteOnly))
	{
		ArgParams->VertexCountPerInstance = 0;
		ArgParams->InstanceCount = 0;
		ArgParams->StartVertexLocation = 0;
		ArgParams->StartInstanceLocation = 0;

		RHIUnlockVertexBuffer(IndirectDrawArgs.VertexBufferRHI);
	}

	// this is analogous to D3D12_INPUT_ELEMENT_DESC (https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc)
	// tells the InputAssembler how to load vertex attributes
	FLocalVertexFactory::FDataType Data;

	// Position stream
	Data.PositionComponent = FVertexStreamComponent(
		&PositionBuffer,
		0,
		PositionStride,
		VET_Float3
	);

	// Tangent stream
	Data.TangentBasisComponents[0] = FVertexStreamComponent(
		&TangentBuffer,
		0,
		TangentStride,
		VET_PackedNormal
	);

	// Normal stream
	Data.TangentBasisComponents[1] = FVertexStreamComponent(
		&TangentBuffer,
		TangentStride / 2,
		TangentStride,
		VET_PackedNormal
	);

	// TexCoord stream, using only one channel
	Data.TextureCoordinates.Add(FVertexStreamComponent(
		&TexCoordBuffer,
		0,
		TexCoordStride,
		VET_Float2
	));

	// Color stream
	Data.ColorComponent = FVertexStreamComponent(
		&ColorBuffer,
		0,
		ColorStride,
		VET_Color
	);

	// set Buffer pointers
	Data.PositionComponentSRV = PositionBuffer.SRV;
	Data.TangentsSRV = TangentBuffer.SRV;
	Data.TextureCoordinatesSRV = TexCoordBuffer.SRV;
	Data.ColorComponentsSRV = ColorBuffer.SRV;

	// not using
	Data.LightMapCoordinateIndex = -1;
	Data.LightMapCoordinateComponent = {};
	Data.LODLightmapDataIndex = 0;

	Data.NumTexCoords = 1;
	Data.ColorIndexMask = ~0u;

	// initialize vertex factory
	VertexFactory.SetData(Data);
	VertexFactory.InitResource();
}

void FCSProceduralMeshSceneProxy::DestroyRenderThreadResources()
{
	FPrimitiveSceneProxy::DestroyRenderThreadResources();
	
	VertexFactory.ReleaseResource();

	PositionBuffer.Release();
	TangentBuffer.Release();
	TexCoordBuffer.Release();
	ColorBuffer.Release();
	IndirectDrawArgs.Release();
}

void FCSProceduralMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_ProceduralMeshSceneProxy_GetDynamicMeshElements );

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			// Set up the FMeshElement.
			FMeshBatch& Mesh = Collector.AllocateMesh();

			Mesh.VertexFactory = &VertexFactory;
			Mesh.MaterialRenderProxy = RenderMaterial->GetRenderProxy();
			Mesh.LCI = NULL;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.CastShadow = true; //TODO
			Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)GetDepthPriorityGroup(View);
			Mesh.Type = PT_TriangleList;
			Mesh.bDisableBackfaceCulling = false;

			// Set up the FMeshBatchElement.
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.FirstIndex = 0;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = (PositionBuffer.NumBytes / sizeof(FVector)) - 1;
			BatchElement.BaseVertexIndex = 0;
			BatchElement.IndexBuffer = nullptr;
			
			// setup required to use IndirectDraw (NumPrimitives=0 + Set IndirectArg parameters)
			BatchElement.NumPrimitives = 0;
			BatchElement.IndirectArgsOffset = 0;
			BatchElement.IndirectArgsBuffer = IndirectDrawArgs.VertexBufferRHI;
			
			// for debug
			Mesh.bCanApplyViewModeOverrides = true;
			Mesh.bUseWireframeSelectionColoring = IsSelected();

			Collector.AddMesh(ViewIndex, Mesh);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;
			RenderBounds(Collector.GetPDI(ViewIndex), EngineShowFlags, GetBounds(), IsSelected());
#endif
		}
	}
}

FPrimitiveSceneProxy* UComputeShaderProceduralMeshComponent::CreateSceneProxy()
{
	checkf(GRHISupportsDrawIndirect, TEXT("GPU doesn't support DrawIndirect :("));

 	if (GRHISupportsDrawIndirect && Material && MaxPrimitives > 0)
	{
		return new FCSProceduralMeshSceneProxy(this);
	}

	return nullptr;
}