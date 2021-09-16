#pragma once

#include "GameFramework/Actor.h"
#include "ClearTextureActor.generated.h"

class UTexture2D;
class UTextureRenderTarget2D;

UCLASS()
class GRAPHICSEXAMPLES_API AClearTextureActor : public AActor
{
	GENERATED_BODY()

public:
	AClearTextureActor();

	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texture", meta=(EditCondition="bUseTexture"))
	UTexture2D* SourceTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta=(EditCondition="!bUseAnimation && !bUseTexture"))
	FLinearColor ClearColor = FLinearColor::Red;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Texture")
	UTextureRenderTarget2D* ResultTexture = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texture", meta = (ClampMin = "16"))
	int32 TextureWidth = 256;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texture", meta=(ClampMin="16"))
	int32 TextureHeight = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta=(EditCondition="!bUseTexture"))
	bool bUseAnimation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture", meta=(EditCondition="!bUseAnimation"))
	bool bUseTexture = false;

private:
	FUnorderedAccessViewRHIRef ResultTextureUAV = nullptr;
	float AnimationTime = 0.f;
};