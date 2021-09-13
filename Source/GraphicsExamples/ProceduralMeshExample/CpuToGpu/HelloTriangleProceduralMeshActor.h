#pragma once

#include "GameFramework/Actor.h"
#include "HelloTriangleProceduralMeshActor.generated.h"

class UComputeShaderProceduralMeshComponent;

UCLASS()
class GRAPHICSEXAMPLES_API AHelloTriangleProceduralMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AHelloTriangleProceduralMeshActor();

	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
	UComputeShaderProceduralMeshComponent* ProceduralMeshComponent = nullptr;

public:
	UPROPERTY(EditAnywhere, Category="ProceduralMesh")
	float TriangleSize = 128;
};