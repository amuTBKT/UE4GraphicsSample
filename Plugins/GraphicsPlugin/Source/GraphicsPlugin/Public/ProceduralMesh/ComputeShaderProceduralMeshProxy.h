#pragma once

#include "RHI.h"
#include "PrimitiveSceneProxy.h"
#include "LocalVertexFactory.h"
#include "ComputeFriendlyBuffers.h"

class UComputeShaderProceduralMeshComponent;

class GRAPHICSPLUGIN_API FCSProceduralMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FCSProceduralMeshSceneProxy(const UComputeShaderProceduralMeshComponent* InComponent);
	~FCSProceduralMeshSceneProxy();

	// Begin FPrimitiveSceneProxy interface
	virtual void CreateRenderThreadResources() override;
	virtual void DestroyRenderThreadResources() override;
	// RenderThread calls this to fetch data we need to render for the proxy
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	// End FPrimitiveSceneProxy interface

	bool HasValidBuffers() const { return PositionBuffer.IsInitialized(); } //one initalized all initialized :)

	// accessor for GPU buffers	
	FRWVertexBuffer& GetPositionBuffer()			{ return PositionBuffer; }
	FRWVertexBuffer& GetTangentBuffer()				{ return TangentBuffer; }
	FRWVertexBuffer& GetTexCoordBuffer()			{ return TexCoordBuffer; }
	FRWVertexBuffer& GetColorBuffer()				{ return ColorBuffer; }
	FRWVertexBuffer& GetIndirectDrawArgsBuffer()	{ return IndirectDrawArgs; }

	uint32 GetMaxPrimitiveCount() const				{ return MaxPrimitiveCount; }

#if WITH_EDITOR
	DECLARE_DELEGATE_OneParam(FOnCreatedRendererResources, FCSProceduralMeshSceneProxy*);
	FOnCreatedRendererResources OnCreatedRendererResources;
#endif

	// {BOILER_PLATE BEGIN}
#pragma region Hidden
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() 		const override { return !MaterialRelevance.bDisableDepthTest; }
	virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }
	uint32 GetAllocatedSize() 			const { return FPrimitiveSceneProxy::GetAllocatedSize(); }
#pragma endregion Hidden
	// {BOILER_PLATE END}

private:
	// copy of Material used by owning component
	const UMaterialInterface* RenderMaterial = nullptr;
	
	// this is boiler plate stuff, check documentation for this. I didn't look too much at this (boring :p)
	FMaterialRelevance MaterialRelevance = {};
	
	// this is vanilla UE4 vertex factory, we can create our own but it'll be too much work :|
	// VertexFactory tells shader pipeline how to fetch vertex attributes
	// Nanite uses texture (visibility buffers) to fetch attributes (no InputAssembler involved), so possibilities are endless
	FLocalVertexFactory VertexFactory;

	// resources used by ComputeShader
	// Note: we can use RWBuffer, RWByteAddressBuffer, RWStructuredBuffer with CS too, but LocalVertexFactory used by Unreal needs VertexBuffer
	FRWVertexBuffer PositionBuffer;
	FRWVertexBuffer TangentBuffer;
	FRWVertexBuffer TexCoordBuffer;
	FRWVertexBuffer ColorBuffer;
	FRWVertexBuffer IndirectDrawArgs;

	// We allocate for worst case :>|
	int32 MaxPrimitiveCount = 0;

	static constexpr uint32 PositionStride	= sizeof(FVector);				// Position
	static constexpr uint32 TangentStride	= sizeof(FPackedNormal) * 2;	// Tangent and Normals are interleaved and packed as uints
	static constexpr uint32 TexCoordStride	= sizeof(FVector2D);			// NOTE: using full precision UVs for simplicity
	static constexpr uint32 ColorStride		= sizeof(FColor);				// Color is stored as uint
};