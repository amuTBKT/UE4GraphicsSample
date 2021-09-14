#include "ProceduralMesh/ComputeShaderProceduralMeshActor.h"

#include "ProceduralMesh/ComputeShaderProceduralMeshComponent.h"
#include "ProceduralMesh/ComputeShaderProceduralMeshProxy.h"

AComputeShaderProceduralMeshActor::AComputeShaderProceduralMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMeshComponent = CreateDefaultSubobject<UComputeShaderProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	
#if WITH_EDITOR
	ProceduralMeshComponent->OnCreatedSceneProxy.BindUObject(this, &AComputeShaderProceduralMeshActor::OnCreatedSceneProxy);
#endif
}

#if WITH_EDITOR
void AComputeShaderProceduralMeshActor::OnCreatedSceneProxy(FCSProceduralMeshSceneProxy* SceneProxy)
{
	if (SceneProxy)
	{
		SceneProxy->OnCreatedRendererResources.BindUObject(this, &AComputeShaderProceduralMeshActor::OnCreatedSceneProxyRendererResources);
	}
}
#endif

void AComputeShaderProceduralMeshActor::BeginPlay()
{
	// When playing with NonEditor config SceneProxy is not created in a conventional way so the OnCreatedSceneProxy callback is never called
	// for such cases we need to manually call this in BeginPlay
	FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	if (WorldContext && WorldContext->WorldType != EWorldType::PIE)
	{
		ENQUEUE_RENDER_COMMAND(MarchingCubesProceduralMeshActor_DestroyResources)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				OnCreatedSceneProxyRendererResources((FCSProceduralMeshSceneProxy*)ProceduralMeshComponent->SceneProxy);
			});
	}

	Super::BeginPlay();
}