#pragma once

#include "RHI.h"

struct FRWVertexBuffer : public FVertexBuffer
{
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FRWVertexBuffer()
		: NumBytes(0)
	{}

	FRWVertexBuffer(FRWVertexBuffer&& Other)
		: UAV(MoveTemp(Other.UAV))
		, SRV(MoveTemp(Other.SRV))
		, NumBytes(Other.NumBytes)
	{
		VertexBufferRHI = MoveTemp(Other.VertexBufferRHI);
		Other.NumBytes = 0;
	}

	FRWVertexBuffer(const FRWVertexBuffer& Other)
		: UAV(Other.UAV)
		, SRV(Other.SRV)
		, NumBytes(Other.NumBytes)
	{
		VertexBufferRHI = Other.VertexBufferRHI;
	}

	FRWVertexBuffer& operator=(FRWVertexBuffer&& Other)
	{
		VertexBufferRHI = MoveTemp(Other.VertexBufferRHI);
		UAV = MoveTemp(Other.UAV);
		SRV = MoveTemp(Other.SRV);
		NumBytes = Other.NumBytes;
		Other.NumBytes = 0;

		return *this;
	}

	FRWVertexBuffer& operator=(const FRWVertexBuffer& Other)
	{
		VertexBufferRHI = Other.VertexBufferRHI;
		UAV = Other.UAV;
		SRV = Other.SRV;
		NumBytes = Other.NumBytes;

		return *this;
	}

	~FRWVertexBuffer()
	{
		Release();
	}

	// @param AdditionalUsage passed down to RHICreateVertexBuffer(), get combined with "BUF_UnorderedAccess | BUF_ShaderResource" e.g. BUF_Static
	void Initialize(uint32 BytesPerElement, uint32 NumElements, EPixelFormat Format, ERHIAccess InResourceState, uint32 AdditionalUsage = 0, const TCHAR* InDebugName = NULL, FResourceArrayInterface *InResourceArray = nullptr)
	{
		check(GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5
			|| IsVulkanPlatform(GMaxRHIShaderPlatform)
			|| IsMetalPlatform(GMaxRHIShaderPlatform)
			|| (GMaxRHIFeatureLevel == ERHIFeatureLevel::ES3_1 && GSupportsResourceView)
		);

		InitResource();

		// Provide a debug name if using Fast VRAM so the allocators diagnostics will work
		ensure(!((AdditionalUsage & BUF_FastVRAM) && !InDebugName));
		NumBytes = BytesPerElement * NumElements;
		FRHIResourceCreateInfo CreateInfo;
		CreateInfo.ResourceArray = InResourceArray;
		CreateInfo.DebugName = InDebugName;
		VertexBufferRHI = RHICreateVertexBuffer(NumBytes, BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage, InResourceState, CreateInfo);
		UAV = RHICreateUnorderedAccessView(VertexBufferRHI, Format);
		SRV = RHICreateShaderResourceView(VertexBufferRHI, BytesPerElement, Format);
	}

	void Initialize(uint32 BytesPerElement, uint32 NumElements, EPixelFormat Format, uint32 AdditionalUsage = 0, const TCHAR* InDebugName = NULL, FResourceArrayInterface* InResourceArray = nullptr)
	{
		Initialize(BytesPerElement, NumElements, Format, ERHIAccess::UAVCompute, AdditionalUsage, InDebugName, InResourceArray);
	}

	void AcquireTransientResource()
	{
		RHIAcquireTransientResource(VertexBufferRHI);
	}
	void DiscardTransientResource()
	{
		RHIDiscardTransientResource(VertexBufferRHI);
	}

	void Release()
	{
		int32 BufferRefCount = VertexBufferRHI ? VertexBufferRHI->GetRefCount() : -1;

		if (BufferRefCount == 1)
		{
			DiscardTransientResource();
		}

		NumBytes = 0;
		ReleaseRHI();
		UAV.SafeRelease();
		SRV.SafeRelease();

		ReleaseResource();
	}
};

struct FRWIndexBuffer : public FIndexBuffer
{
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FRWIndexBuffer()
		: NumBytes(0)
	{}

	FRWIndexBuffer(FRWIndexBuffer&& Other)
		: UAV(MoveTemp(Other.UAV))
		, SRV(MoveTemp(Other.SRV))
		, NumBytes(Other.NumBytes)
	{
		IndexBufferRHI = MoveTemp(Other.IndexBufferRHI);
		Other.NumBytes = 0;
	}

	FRWIndexBuffer(const FRWIndexBuffer& Other)
		: UAV(Other.UAV)
		, SRV(Other.SRV)
		, NumBytes(Other.NumBytes)
	{
		IndexBufferRHI = Other.IndexBufferRHI;
	}

	FRWIndexBuffer& operator=(FRWIndexBuffer&& Other)
	{
		IndexBufferRHI = MoveTemp(Other.IndexBufferRHI);
		UAV = MoveTemp(Other.UAV);
		SRV = MoveTemp(Other.SRV);
		NumBytes = Other.NumBytes;
		Other.NumBytes = 0;

		return *this;
	}

	FRWIndexBuffer& operator=(const FRWIndexBuffer& Other)
	{
		IndexBufferRHI = Other.IndexBufferRHI;
		UAV = Other.UAV;
		SRV = Other.SRV;
		NumBytes = Other.NumBytes;

		return *this;
	}

	~FRWIndexBuffer()
	{
		Release();
	}

	// @param AdditionalUsage passed down to RHICreateVertexBuffer(), get combined with "BUF_UnorderedAccess | BUF_ShaderResource" e.g. BUF_Static
	void Initialize(uint32 Stride, uint32 NumElements, EPixelFormat Format, ERHIAccess InResourceState, uint32 AdditionalUsage = 0, const TCHAR* InDebugName = NULL, FResourceArrayInterface *InResourceArray = nullptr)
	{
		check(GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5
			|| IsVulkanPlatform(GMaxRHIShaderPlatform)
			|| IsMetalPlatform(GMaxRHIShaderPlatform)
			|| (GMaxRHIFeatureLevel == ERHIFeatureLevel::ES3_1 && GSupportsResourceView)
		);

		InitResource();

		// Provide a debug name if using Fast VRAM so the allocators diagnostics will work
		ensure(!((AdditionalUsage & BUF_FastVRAM) && !InDebugName));
		NumBytes = Stride * NumElements;
		FRHIResourceCreateInfo CreateInfo;
		CreateInfo.ResourceArray = InResourceArray;
		CreateInfo.DebugName = InDebugName;
		IndexBufferRHI = RHICreateIndexBuffer(Stride, NumBytes, BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage, CreateInfo);
		UAV = RHICreateUnorderedAccessView(IndexBufferRHI, Format);
		SRV = RHICreateShaderResourceView(IndexBufferRHI);
	}

	void Initialize(uint32 BytesPerElement, uint32 NumElements, EPixelFormat Format, uint32 AdditionalUsage = 0, const TCHAR* InDebugName = NULL, FResourceArrayInterface* InResourceArray = nullptr)
	{
		Initialize(BytesPerElement, NumElements, Format, ERHIAccess::UAVCompute, AdditionalUsage, InDebugName, InResourceArray);
	}

	void AcquireTransientResource()
	{
		//RHIAcquireTransientResource(IndexBufferRHI);
	}
	void DiscardTransientResource()
	{
		//RHIDiscardTransientResource(IndexBufferRHI);
	}

	void Release()
	{
		int32 BufferRefCount = IndexBufferRHI ? IndexBufferRHI->GetRefCount() : -1;

		if (BufferRefCount == 1)
		{
			DiscardTransientResource();
		}

		NumBytes = 0;
		ReleaseRHI();
		UAV.SafeRelease();
		SRV.SafeRelease();

		ReleaseResource();
	}
};