#include "ProceduralMesh/ComputeShaderProceduralMeshComponent.h"
#include "Engine/CollisionProfile.h"

UComputeShaderProceduralMeshComponent::UComputeShaderProceduralMeshComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

#if WITH_EDITOR
void UComputeShaderProceduralMeshComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	OnCreatedSceneProxy.ExecuteIfBound((FCSProceduralMeshSceneProxy*)SceneProxy);
}
#endif

FBoxSphereBounds UComputeShaderProceduralMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVector ScaledBounds = FixedLocalBounds * LocalToWorld.GetScale3D() * 0.5f;
	return FBoxSphereBounds(LocalToWorld.GetLocation(), ScaledBounds, ScaledBounds.Size());
}

UMaterialInterface* UComputeShaderProceduralMeshComponent::GetMaterial(int32 Index) const
{
	return Material;
}

void UComputeShaderProceduralMeshComponent::SetMaterial(int32 ElementIndex, class UMaterialInterface* InMaterial)
{
	Material = InMaterial;

	MarkRenderStateDirty();
}

void UComputeShaderProceduralMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (Material)
	{
		OutMaterials.AddUnique(Material);
	}
}

void UComputeShaderProceduralMeshComponent::SetMaxPrimitiveCount(int32 PrimitiveCount)
{
	MaxPrimitives = FMath::Max(0, PrimitiveCount);

	MarkRenderStateDirty();
}

#if WITH_EDITOR
void UComputeShaderProceduralMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	bool bRecreateRenderState = false;

	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UComputeShaderProceduralMeshComponent, Material))
		{
			bRecreateRenderState = true;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UComputeShaderProceduralMeshComponent, MaxPrimitives))
		{
			bRecreateRenderState = true;
		}
	}

	if (bRecreateRenderState)
	{
		MarkRenderStateDirty();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif