#include "HelloTriangleProceduralMeshActor.h"

#include "ProceduralMesh/ComputeShaderProceduralMeshComponent.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshProxy.h"

AHelloTriangleProceduralMeshActor::AHelloTriangleProceduralMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMeshComponent = CreateDefaultSubobject<UComputeShaderProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
}

void AHelloTriangleProceduralMeshActor::BeginPlay()
{
	// this shows how we can pass data from CPU to GPU
	// NOTE:	This is expensive and should really be done on initialization
	ENQUEUE_RENDER_COMMAND(HelloTriangleProceduralMeshActor_CreateTriangle)(
		[this](FRHICommandListImmediate& RHICmdList)
	{
		FCSProceduralMeshSceneProxy* SceneProxy = (FCSProceduralMeshSceneProxy*)ProceduralMeshComponent->SceneProxy;
		if (SceneProxy && SceneProxy->HasValidBuffers())
		{
			if (FVector* VertPositions = (FVector*)RHILockVertexBuffer(SceneProxy->GetPositionBuffer().VertexBufferRHI, 0, sizeof(FVector) * 3, RLM_WriteOnly))
			{
				VertPositions[0] = FVector(-TriangleSize, -TriangleSize, 0);
				VertPositions[1] = FVector(0, TriangleSize, 0);
				VertPositions[2] = FVector(TriangleSize, -TriangleSize, 0);
				RHIUnlockVertexBuffer(SceneProxy->GetPositionBuffer().VertexBufferRHI);
			}

			if (FPackedNormal* VertTangents = (FPackedNormal*)RHILockVertexBuffer(SceneProxy->GetTangentBuffer().VertexBufferRHI, 0, sizeof(FPackedNormal) * 6, RLM_WriteOnly))
			{
				VertTangents[0] = FPackedNormal(FVector(1, 0, 0));
				VertTangents[1] = FPackedNormal(FVector(0, 0, 1));

				VertTangents[2] = FPackedNormal(FVector(1, 0, 0));
				VertTangents[3] = FPackedNormal(FVector(0, 0, 1));

				VertTangents[4] = FPackedNormal(FVector(1, 0, 0));
				VertTangents[5] = FPackedNormal(FVector(0, 0, 1));

				RHIUnlockVertexBuffer(SceneProxy->GetTangentBuffer().VertexBufferRHI);
			}

			if (FVector2D* VertTexCoords = (FVector2D*)RHILockVertexBuffer(SceneProxy->GetTexCoordBuffer().VertexBufferRHI, 0, sizeof(FVector2D) * 3, RLM_WriteOnly))
			{
				VertTexCoords[0] = FVector2D(0.f, 0.f);
				VertTexCoords[1] = FVector2D(0.5f, 1.f);
				VertTexCoords[2] = FVector2D(1.f, 0.f);;

				RHIUnlockVertexBuffer(SceneProxy->GetTexCoordBuffer().VertexBufferRHI);
			}

			if (FColor* VertColors = (FColor*)RHILockVertexBuffer(SceneProxy->GetColorBuffer().VertexBufferRHI, 0, sizeof(FColor) * 3, RLM_WriteOnly))
			{
				VertColors[0] = FColor::Red;
				VertColors[1] = FColor::Green;
				VertColors[2] = FColor::Blue;

				RHIUnlockVertexBuffer(SceneProxy->GetColorBuffer().VertexBufferRHI);
			}

			if (FRHIDrawIndirectParameters* ArgParams = (FRHIDrawIndirectParameters*)RHILockVertexBuffer(SceneProxy->GetIndirectDrawArgsBuffer().VertexBufferRHI, 0, sizeof(FRHIDrawIndirectParameters), RLM_WriteOnly))
			{
				ArgParams->VertexCountPerInstance = 3;
				ArgParams->InstanceCount = 1;
				ArgParams->StartVertexLocation = 0;
				ArgParams->StartInstanceLocation = 0;

				RHIUnlockVertexBuffer(SceneProxy->GetIndirectDrawArgsBuffer().VertexBufferRHI);
			}
		}
	});

	Super::BeginPlay();
}

void AHelloTriangleProceduralMeshActor::BeginDestroy()
{
	Super::BeginDestroy();
}

void AHelloTriangleProceduralMeshActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}