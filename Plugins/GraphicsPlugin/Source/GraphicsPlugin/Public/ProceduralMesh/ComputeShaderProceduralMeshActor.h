#pragma once

#include "GameFramework/Actor.h"
#include "ComputeShaderProceduralMeshActor.generated.h"

class UComputeShaderProceduralMeshComponent;
class FCSProceduralMeshSceneProxy;

UCLASS(Abstract)
class GRAPHICSPLUGIN_API AComputeShaderProceduralMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AComputeShaderProceduralMeshActor();

protected:
	virtual void BeginPlay() override;

	// since we are manipulating the SceneProxy ourselves, this callback is fired when SceneProxy is ready.
	virtual void OnCreatedSceneProxyRendererResources(FCSProceduralMeshSceneProxy* SceneProxy) {}

#if WITH_EDITOR
private:
	void OnCreatedSceneProxy(FCSProceduralMeshSceneProxy* SceneProxy);
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ProceduralMesh")
	UComputeShaderProceduralMeshComponent* ProceduralMeshComponent = nullptr;
};