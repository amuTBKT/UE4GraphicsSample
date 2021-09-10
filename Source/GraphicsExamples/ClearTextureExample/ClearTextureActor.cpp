#include "ClearTextureActor.h"
#include "Engine/TextureRenderTarget2D.h"

// shader from Graphics plugin. [TODO] How to make it nested into GraphicsPlugin/
#include "ClearTextureShader.h"

// for using RenderGraph
#include "RenderGraph.h"
#include "RenderGraphUtils.h"

AClearTextureActor::AClearTextureActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AClearTextureActor::BeginPlay()
{
	// we can remove this line if the RenderTarget is coming from ContentBrowser
	ResultTexture = NewObject<UTextureRenderTarget2D>();
	checkf(ResultTexture, TEXT("ResultTexture should not be invalid here!"));

	ResultTexture->bCanCreateUAV = true;		//needed to create UAV
	ResultTexture->bAutoGenerateMips = false;	//need additional work to update mips
	ResultTexture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	ResultTexture->MipsSamplerFilter = TextureFilter::TF_Bilinear;
	ResultTexture->ClearColor = FLinearColor::Black;
	ResultTexture->InitAutoFormat(TextureWidth, TextureHeight);
	ResultTexture->UpdateResourceImmediate(true);

	// queue task on RenderThread to create UAV resource
	ENQUEUE_RENDER_COMMAND(ClearTextureActor_InitializeUAV)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			FTextureRenderTarget2DResource* TextureResource = (FTextureRenderTarget2DResource*)ResultTexture->Resource;
			ResultTextureUAV = RHICreateUnorderedAccessView(TextureResource->TextureRHI, /*MipLevel=*/ 0); //we are rendering to first mip

			// texture to be used by Material (graphics)
			RHICmdList.Transition(FRHITransitionInfo(ResultTextureUAV, ERHIAccess::Unknown, ERHIAccess::SRVGraphics));
		});

	Super::BeginPlay();
}

void AClearTextureActor::BeginDestroy()
{
	// queue task on RenderThread to release UAV resource
	ENQUEUE_RENDER_COMMAND(ClearTextureActor_InitializeUAV)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			ResultTextureUAV.SafeRelease();
		});

	Super::BeginDestroy();
}

void AClearTextureActor::Tick(float DeltaSeconds)
{
	const auto FeatureLevel = GetWorld()->Scene->GetFeatureLevel();
	AnimationTime += DeltaSeconds;

	// we can copy some values that can be modified on GameThread
	struct FParametersRTCopy
	{
		FLinearColor	ClearColor;
		bool			UseAnimation;
		float			AnimationTime;
	};
	FParametersRTCopy RTCopy = {};
	RTCopy.ClearColor = ClearColor;
	RTCopy.UseAnimation = bUseAnimation;
	RTCopy.AnimationTime = AnimationTime;

	// queue work on RenderThread to update texture
	ENQUEUE_RENDER_COMMAND(ClearTextureActor_ClearTexture)(
		[this, FeatureLevel, RTCopy](FRHICommandListImmediate& RHICmdList)
		{
			FMemMark Mark(FMemStack::Get());		//required for tracking
			FRDGBuilder GraphBuilder(RHICmdList);	//https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/Rendering/RenderDependencyGraph/

			const float RTWidth = ResultTexture->GetSurfaceWidth();
			const float RTHeight = ResultTexture->GetSurfaceHeight();

			const FIntVector GroupCount(
				FMath::CeilToInt(RTWidth / (float)FClearTextureCS::ThreadGroupSizeX),
				FMath::CeilToInt(RTHeight / (float)FClearTextureCS::ThreadGroupSizeY),
				1);

			// update Permutation to select proper shader
			FClearTextureCS::FPermutationDomain Permutation;
			Permutation.Set<FClearTextureCS::FUseAnimation>(RTCopy.UseAnimation);

			// get GlobalShaderMap
			FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
			// if the shader is not using Permutation, we can remove the second argument
			TShaderMapRef<FClearTextureCS> ComputeShader(GlobalShaderMap, Permutation);

			// this is the new way of adding parameters, very easy to use :)
			FClearTextureCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FClearTextureCS::FParameters>();
			PassParameters->OutputTexture = ResultTextureUAV;
			PassParameters->ClearColor = FVector4(RTCopy.ClearColor);
			PassParameters->TextureDimensions = FVector4(RTWidth, RTHeight, 1.f / RTWidth, 1.f / RTHeight);
			PassParameters->AnimationTime = RTCopy.AnimationTime;

			// some validation (refer to link above)
			ValidateShaderParameters(ComputeShader, *PassParameters);

			// texture needs to be in writable state from compute shader
			RHICmdList.Transition(FRHITransitionInfo(ResultTextureUAV, ERHIAccess::SRVGraphics, ERHIAccess::UAVCompute));

			// add our pass
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("ClearTexture_Animated"),
				ComputeShader, PassParameters, GroupCount);

			// texture to be used by Material (graphics)
			RHICmdList.Transition(FRHITransitionInfo(ResultTextureUAV, ERHIAccess::UAVCompute, ERHIAccess::SRVGraphics));

			// we can add more work here

			// finally execute
			GraphBuilder.Execute();
		});

	Super::Tick(DeltaSeconds);
}