#pragma once

#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "ShaderPermutation.h"
#include "ShaderParameterStruct.h"

class GRAPHICSPLUGIN_API FResetIndirectArgsCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FResetIndirectArgsCS)

	SHADER_USE_PARAMETER_STRUCT(FResetIndirectArgsCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, IndirectDrawArgsBuffer)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

public:
	static constexpr int32 ThreadGroupSizeX = 1;
	static constexpr int32 ThreadGroupSizeY = 1;
	static constexpr int32 ThreadGroupSizeZ = 1;
};

class GRAPHICSPLUGIN_API FGenerateTrianglesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGenerateTrianglesCS)

	SHADER_USE_PARAMETER_STRUCT(FGenerateTrianglesCS, FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<float>,  PositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, TangentBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint32>, IndirectDrawArgsBuffer)
		SHADER_PARAMETER_SRV(Texture3D<uint8>, NumVertsPerVoxel)
		SHADER_PARAMETER_SRV(Texture2D<uint8>, NumVerticesLookupTable)
		SHADER_PARAMETER_SRV(Texture2D<uint8>, TriangleVerticesLookupTable)
		SHADER_PARAMETER(FIntVector, VoxelCount)
		SHADER_PARAMETER(float, VoxelSize)
		SHADER_PARAMETER(float, IsoValue)
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(uint32, MaxVertices)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

public:
	static constexpr int32 ThreadGroupSizeX = 4;
	static constexpr int32 ThreadGroupSizeY = 4;
	static constexpr int32 ThreadGroupSizeZ = 4;
};