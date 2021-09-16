#pragma once

#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "ShaderPermutation.h"
#include "ShaderParameterStruct.h"

// GlobalShader has only one instance active in the application, refer Engine/Source/Runtime/RenderCore/Public/GlobalShader.h for more details
class GRAPHICSPLUGIN_API FClearTextureCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FClearTextureCS)

	// used to pass parameters to shader (much better than legacy code, kindof means we need to use RenderGraph)
	SHADER_USE_PARAMETER_STRUCT(FClearTextureCS, FGlobalShader)

	// Permutations are used to enable/disable shader code at compile time
	class FUseAnimation : SHADER_PERMUTATION_BOOL("USE_ANIMATION");
	class FUseTexture : SHADER_PERMUTATION_BOOL("USE_TEXTURE");
	using FPermutationDomain = TShaderPermutationDomain<FUseAnimation,FUseTexture>;
	
	/**
	* Mirror variables defined in shader
	* You can expect this error if the variables don't match b/w code and shader
	*	"Shader FClearTextureCS, permutation 0 has unbound parameters not represented in the parameter struct: {VAR_NAME}"
	*/
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture<float4>, OutputTexture)
		SHADER_PARAMETER_SRV(Texture2D, SourceTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SourceSampler)
		SHADER_PARAMETER(FVector4, ClearColor)
		SHADER_PARAMETER(FVector4, TextureDimensions)
		SHADER_PARAMETER(float, AnimationTime)
	END_SHADER_PARAMETER_STRUCT()

public:
	// we can override this to check if we can compile this shader
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	// this is useful if we want to set additional arguments
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		// for example we can add this flag to enable Wave Ops
		// needs #include "ShaderCompilerCore.h"
		//OutEnvironment.CompilerFlags.Add(CFLAG_WaveOperations);

		// or set ThreadGroupSize instead of hard coding inside shader
		// make sure numthreads in shader are using these instead of hardcoded values
		//OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), 8);
		//OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), 8);
	}

public:
	// alternatively we can set ThreadGroupSize through ModifyCompilationEnvironment
	static constexpr int32 ThreadGroupSizeX = 8;
	static constexpr int32 ThreadGroupSizeY = 8;
};