#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "ComputeShaderProceduralMeshComponent.generated.h"

class FPrimitiveSceneProxy;
class UMaterialInterface;

UCLASS(hidecategories=(Collision,Object,Physics,SceneComponent,Activation,"Components|Activation"), ClassGroup=Rendering, meta=(BlueprintSpawnableComponent))
class GRAPHICSPLUGIN_API UComputeShaderProceduralMeshComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:
	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual UMaterialInterface* GetMaterial(int32 Index) const override;
	virtual void SetMaterial(int32 ElementIndex, class UMaterialInterface* InMaterial) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "ProceduralMesh")
	void SetMaxPrimitiveCount(int32 PrimitiveCount);
	
	UFUNCTION(BlueprintCallable, Category = "ProceduralMesh")
	int32 GetMaxPrimitiveCount() const { return MaxPrimitives; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProceduralMesh")
	FVector FixedLocalBounds = FVector(64, 64, 64);

private:
	UPROPERTY(EditAnywhere, Category = "ProceduralMesh")
	UMaterialInterface* Material = nullptr;

	UPROPERTY(EditAnywhere, Category = "ProceduralMesh", meta = (ClampMin = "1"))
	int32 MaxPrimitives = 1;
};
