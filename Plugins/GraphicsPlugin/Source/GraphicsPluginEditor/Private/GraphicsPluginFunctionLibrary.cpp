#include "GraphicsPluginFunctionLibrary.h"
#include "HAL/PlatformApplicationMisc.h"

bool UGraphicsPluginEditorFunctionLibrary::CalculateSplineParameters(
	UStaticMesh* StaticMesh,
	ESplineMeshAxis::Type ForwardAxis,
	FVector& OutSplineMeshDir,
	FVector& OutSplineMeshX,
	FVector& OutSplineMeshY,
	float& OutSplineMeshScaleZ,
	float& OutSplineMeshMinZ)
{
	if (StaticMesh)
	{
		// direction vectors
		FVector DirMask(0, 0, 0);
		DirMask = FVector::ZeroVector;
		DirMask[ForwardAxis] = 1;
		OutSplineMeshDir = DirMask;
		DirMask = FVector::ZeroVector;
		DirMask[(ForwardAxis + 1) % 3] = 1;
		OutSplineMeshX = DirMask;
		DirMask = FVector::ZeroVector;
		DirMask[(ForwardAxis + 2) % 3] = 1;
		OutSplineMeshY = DirMask;

		FBoxSphereBounds StaticMeshBounds = StaticMesh->GetBounds();
		OutSplineMeshScaleZ = 0.5f / USplineMeshComponent::GetAxisValue(StaticMeshBounds.BoxExtent, ForwardAxis);
		OutSplineMeshMinZ = USplineMeshComponent::GetAxisValue(StaticMeshBounds.Origin, ForwardAxis) * OutSplineMeshScaleZ - 0.5f;

		return true;
	}

	return false;
}

void UGraphicsPluginEditorFunctionLibrary::CopyTextToClipboard(const FString& Text)
{
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}