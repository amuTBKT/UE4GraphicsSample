// Copyright Epic Games, Inc. All Rights Reserved.
#include "NiagaraDataInterfaces/NiagaraDataInterfaceAtomicBuffer.h"
#include "NiagaraShader.h"
#include "NiagaraRenderer.h"
#include "ShaderParameterUtils.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstanceBatcher.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceAtomicBuffer"

PRAGMA_DISABLE_OPTIMIZATION

const FString UNiagaraDataInterfaceAtomicBuffer::NumElements_VarName("NumElements_");
const FString UNiagaraDataInterfaceAtomicBuffer::ResetValue_VarName("ResetValue_");
const FString UNiagaraDataInterfaceAtomicBuffer::AtomicBuffer_VarName("AtomicBuffer_");

const FName UNiagaraDataInterfaceAtomicBuffer::SetNumElements_FuncName("SetNumElements");
const FName UNiagaraDataInterfaceAtomicBuffer::SetResetValue_FuncName("SetResetValue");
const FName UNiagaraDataInterfaceAtomicBuffer::SetClearEveryFrame_FuncName("SetClearEveryFrame");

const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedAdd_FuncName("InterlockedAdd");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedMin_FuncName("InterlockedMin");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedMax_FuncName("InterlockedMax");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedOr_FuncName("InterlockedOr");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedAnd_FuncName("InterlockedAnd");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedXor_FuncName("InterlockedXor");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedCompareStore_FuncName("InterlockedCompareStore");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedCompareExchange_FuncName("InterlockedCompareExchange");
const FName UNiagaraDataInterfaceAtomicBuffer::InterlockedExchange_FuncName("InterlockedExchange");

const FName UNiagaraDataInterfaceAtomicBuffer::GetValue_FuncName("GetValue");
const FName UNiagaraDataInterfaceAtomicBuffer::SetValue_FuncName("SetValue");

class FAtomicBufferInstanceData_GT
{
public:
	int32 NumElements = 0;
	int32 ResetValue = 0;
	bool ClearEveryFrame = true;
	bool NeedsRealloc = false;
};

class FAtomicBufferInstanceData_GTtoRT
{
public:
	int32 ResetValue = 0;
	bool ClearEveryFrame = true;
};

class FAtomicBufferInstanceData_RT
{
public:
	~FAtomicBufferInstanceData_RT()
	{
		AtomicBuffer.Release();
	}

	void ResizeBuffers()
	{
		AtomicBuffer.Initialize(sizeof(int32), NumElements, EPixelFormat::PF_R32_SINT, BUF_Static, TEXT("AtomicBufferDI"));
	}

public:
	FRWBuffer AtomicBuffer;
	int32 NumElements = 0;
	int32 ResetValue = 0;
	bool ClearEveryFrame = true;
};

struct FNiagaraDataInterfaceProxyAtomicBuffer : public FNiagaraDataInterfaceProxyRW
{
	virtual void PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override
	{
		if (const FAtomicBufferInstanceData_RT* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID))
		{
			if (Context.IsOutputStage && ProxyData->ClearEveryFrame)
			{
				SCOPED_DRAW_EVENT(RHICmdList, NiagaraDIAtomicBuffer_Clear);
				
				FRHITransitionInfo TransitionInfos[] =
				{
					FRHITransitionInfo(ProxyData->AtomicBuffer.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute),
				};
				RHICmdList.Transition(MakeArrayView(TransitionInfos, UE_ARRAY_COUNT(TransitionInfos)));

				RHICmdList.ClearUAVUint(ProxyData->AtomicBuffer.UAV, FUintVector4(ProxyData->ResetValue, ProxyData->ResetValue, ProxyData->ResetValue, ProxyData->ResetValue));
			}
		}
	}

	virtual FIntVector GetElementCount(FNiagaraSystemInstanceID SystemInstanceID) const override
	{
		if (const FAtomicBufferInstanceData_RT* TargetData = SystemInstancesToProxyData.Find(SystemInstanceID))
		{
			return FIntVector(TargetData->NumElements, 1, 1);
		}
		return FIntVector::ZeroValue;
	}

	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstanceID) override
	{
		if (FAtomicBufferInstanceData_RT* InstanceData_RT = SystemInstancesToProxyData.Find(SystemInstanceID))
		{
			FAtomicBufferInstanceData_GTtoRT* DataPassedToRT = (FAtomicBufferInstanceData_GTtoRT*)PerInstanceData;
			
			InstanceData_RT->ResetValue = DataPassedToRT->ResetValue;
			InstanceData_RT->ClearEveryFrame = DataPassedToRT->ClearEveryFrame;
			
			DataPassedToRT->~FAtomicBufferInstanceData_GTtoRT();
		}
	}
	
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return sizeof(FAtomicBufferInstanceData_GTtoRT); }

	/* List of proxy data for each system instances*/
	// #todo(dmp): this should all be refactored to avoid duplicate code
	TMap<FNiagaraSystemInstanceID, FAtomicBufferInstanceData_RT> SystemInstancesToProxyData;
};

/*--------------------------------------------------------------------------------------------------------------------------*/
struct FNiagaraDataInterfaceParametersCS_AtomicBuffer : public FNiagaraDataInterfaceParametersCS
{
	DECLARE_TYPE_LAYOUT(FNiagaraDataInterfaceParametersCS_AtomicBuffer, NonVirtual);
public:
	void Bind(const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap)
	{
		OutputParam.Bind(ParameterMap, *("RW" + UNiagaraDataInterfaceAtomicBuffer::AtomicBuffer_VarName + ParameterInfo.DataInterfaceHLSLSymbol));
		InputParam.Bind(ParameterMap, *(UNiagaraDataInterfaceAtomicBuffer::AtomicBuffer_VarName + ParameterInfo.DataInterfaceHLSLSymbol));
		NumElementsParam.Bind(ParameterMap, *(UNiagaraDataInterfaceAtomicBuffer::NumElements_VarName + ParameterInfo.DataInterfaceHLSLSymbol));
		ResetValueParam.Bind(ParameterMap, *(UNiagaraDataInterfaceAtomicBuffer::ResetValue_VarName + ParameterInfo.DataInterfaceHLSLSymbol));
	}

	// #todo(dmp): make resource transitions batched
	void Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
	{
		check(IsInRenderingThread());

		FRHIComputeShader* ComputeShaderRHI = Context.Shader.GetComputeShader();
		FNiagaraDataInterfaceProxyAtomicBuffer* Proxy = static_cast<FNiagaraDataInterfaceProxyAtomicBuffer*>(Context.DataInterface);

		FAtomicBufferInstanceData_RT* ProxyData = Proxy->SystemInstancesToProxyData.Find(Context.SystemInstanceID);
		
		if (!(ProxyData && ProxyData->AtomicBuffer.Buffer.IsValid()))
		{			
			SetShaderValue(RHICmdList, ComputeShaderRHI, NumElementsParam , 0);
			SetShaderValue(RHICmdList, ComputeShaderRHI, ResetValueParam, 0);
			if (OutputParam.IsBound())
			{
				SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputParam, Context.Batcher->GetEmptyUAVFromPool(RHICmdList, PF_R32_SINT, ENiagaraEmptyUAVType::Buffer));
			}
			if (InputParam.IsBound())
			{
				SetSRVParameter(RHICmdList, ComputeShaderRHI, InputParam, FNiagaraRenderer::GetDummyIntBuffer());
			}
			return;
		}

		SetShaderValue(RHICmdList, ComputeShaderRHI, NumElementsParam, ProxyData->NumElements);
		SetShaderValue(RHICmdList, ComputeShaderRHI, ResetValueParam, ProxyData->ResetValue);

		const bool bIsOutputBound = OutputParam.IsBound();
		if (bIsOutputBound)
		{
			FRHIUnorderedAccessView* OutputUAV = ProxyData->AtomicBuffer.UAV;
			RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));			
			SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputParam, OutputUAV);
		}

		if (InputParam.IsBound())
		{
			if (bIsOutputBound)
			{
				UE_LOG(LogTemp, Warning, TEXT("NiagaraDIAtomicBuffer(%s) is bound as both read & write, read will be ignored."), *Context.DataInterface->SourceDIName.ToString());
				SetSRVParameter(RHICmdList, ComputeShaderRHI, InputParam, FNiagaraRenderer::GetDummyIntBuffer());
			}
			else
			{
				FRHIUnorderedAccessView* OutputUAV = ProxyData->AtomicBuffer.UAV;
				RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute));
				SetSRVParameter(RHICmdList, ComputeShaderRHI, InputParam, ProxyData->AtomicBuffer.SRV);
			}
		}
	}

	void Unset(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const 
	{
		FRHIComputeShader* ComputeShaderRHI = Context.Shader.GetComputeShader();
		
		if (OutputParam.IsBound())
		{
			SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputParam, FUnorderedAccessViewRHIRef());
		}
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, OutputParam);
	LAYOUT_FIELD(FShaderResourceParameter, InputParam);
	LAYOUT_FIELD(FShaderParameter, NumElementsParam);
	LAYOUT_FIELD(FShaderParameter, ResetValueParam);
};

IMPLEMENT_TYPE_LAYOUT(FNiagaraDataInterfaceParametersCS_AtomicBuffer);

IMPLEMENT_NIAGARA_DI_PARAMETER(UNiagaraDataInterfaceAtomicBuffer, FNiagaraDataInterfaceParametersCS_AtomicBuffer);

UNiagaraDataInterfaceAtomicBuffer::UNiagaraDataInterfaceAtomicBuffer(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	Proxy.Reset(new FNiagaraDataInterfaceProxyAtomicBuffer());
}

int32 UNiagaraDataInterfaceAtomicBuffer::PerInstanceDataSize() const
{
	return sizeof(FAtomicBufferInstanceData_GT);
}

void UNiagaraDataInterfaceAtomicBuffer::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	Super::GetFunctions(OutFunctions);

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = SetNumElements_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("NumElements")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Success")));

		Sig.ModuleUsageBitmask = ENiagaraScriptUsageMask::Emitter | ENiagaraScriptUsageMask::System;
		Sig.bExperimental = true;
		Sig.bMemberFunction = true;
		Sig.bRequiresExecPin = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = SetResetValue_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("ResetValue")));

		Sig.ModuleUsageBitmask = ENiagaraScriptUsageMask::Emitter | ENiagaraScriptUsageMask::System;
		Sig.bExperimental = true;
		Sig.bMemberFunction = true;
		Sig.bRequiresExecPin = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		OutFunctions.Add(Sig);
	}

	{
		auto GetFunctionSignature = [this](FName FuncName) -> FNiagaraFunctionSignature
		{
			FNiagaraFunctionSignature Sig = {};
			Sig.Name = FuncName;
			Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
			Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Enabled")));
			Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
			Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));
			Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("OriginalValue")));

			Sig.bExperimental = true;
			Sig.bWriteFunction = true;
			Sig.bMemberFunction = true;
			Sig.bRequiresContext = false;
			Sig.bSupportsCPU = false;
			Sig.bSupportsGPU = true;
			
			return Sig;
		};

		OutFunctions.Add(GetFunctionSignature(InterlockedAdd_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedMin_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedMax_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedOr_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedAnd_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedXor_FuncName));
		OutFunctions.Add(GetFunctionSignature(InterlockedExchange_FuncName));
	}

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = InterlockedCompareStore_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Enabled")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("CompareValue")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("OriginalValue")));

		Sig.bExperimental = true;
		Sig.bWriteFunction = true;
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = false;
		Sig.bSupportsGPU = true;

		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = InterlockedCompareExchange_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Enabled")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("CompareValue")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("OriginalValue")));

		Sig.bExperimental = true;
		Sig.bWriteFunction = true;
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = false;
		Sig.bSupportsGPU = true;

		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = GetValue_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));

		Sig.bExperimental = true;
		Sig.bWriteFunction = false;
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = false;
		Sig.bSupportsGPU = true;

		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = {};
		Sig.Name = SetValue_FuncName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("AtomicBuffer")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Enabled")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));

		Sig.bExperimental = true;
		Sig.bWriteFunction = true;
		Sig.bMemberFunction = true;
		Sig.bRequiresExecPin = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = false;
		Sig.bSupportsGPU = true;

		OutFunctions.Add(Sig);
	}
}

void UNiagaraDataInterfaceAtomicBuffer::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc)
{
	Super::GetVMExternalFunction(BindingInfo, InstanceData, OutFunc);

	if (BindingInfo.Name == UNiagaraDataInterfaceAtomicBuffer::SetNumElements_FuncName)
	{
		check(BindingInfo.GetNumInputs() == 2 && BindingInfo.GetNumOutputs() == 1);
		OutFunc = FVMExternalFunction::CreateLambda([](FVectorVMContext& Context)
		{
			VectorVM::FUserPtrHandler<FAtomicBufferInstanceData_GT> InstanceData(Context);
			VectorVM::FExternalFuncInputHandler<int32> InElementsCount(Context);
			VectorVM::FExternalFuncRegisterHandler<FNiagaraBool> OutSuccess(Context);

			for (int32 InstanceIdx = 0; InstanceIdx < Context.GetNumInstances(); ++InstanceIdx)
			{
				const int NewElementsCount = InElementsCount.GetAndAdvance();
				const bool bSuccess = (InstanceData.Get() != nullptr && Context.GetNumInstances() == 1 && NewElementsCount > 0);
				*OutSuccess.GetDestAndAdvance() = bSuccess;
				if (bSuccess)
				{
					int32 OldElementsCount = InstanceData->NumElements;
					InstanceData->NumElements = NewElementsCount;					
					InstanceData->NeedsRealloc = OldElementsCount != InstanceData->NumElements;
				}
			}
		});
	}
	else if (BindingInfo.Name == UNiagaraDataInterfaceAtomicBuffer::SetResetValue_FuncName)
	{
		check(BindingInfo.GetNumInputs() == 2 && BindingInfo.GetNumOutputs() == 0);
		OutFunc = FVMExternalFunction::CreateLambda([](FVectorVMContext& Context)
		{
			VectorVM::FUserPtrHandler<FAtomicBufferInstanceData_GT> InstanceData(Context);
			VectorVM::FExternalFuncInputHandler<int32> InResetValue(Context);
			
			for (int32 InstanceIdx = 0; InstanceIdx < Context.GetNumInstances(); ++InstanceIdx)
			{
				InstanceData->ResetValue = InResetValue.GetAndAdvance();
			}
		});
	}
}

bool UNiagaraDataInterfaceAtomicBuffer::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	const UNiagaraDataInterfaceAtomicBuffer* OtherTyped = CastChecked<const UNiagaraDataInterfaceAtomicBuffer>(Other);

	return OtherTyped->ResetValue == ResetValue && OtherTyped->ClearEveryFrame == ClearEveryFrame;
}

#if WITH_EDITORONLY_DATA
void UNiagaraDataInterfaceAtomicBuffer::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	Super::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);

	static const TCHAR* FormatDeclarations = TEXT(R"(
		RWBuffer<int> RW{BufferName};
		Buffer<int> {BufferName};
		int {NumElementsName};
		int {ResetValueName};
	)");
	TMap<FString, FStringFormatArg> ArgsDeclarations =
	{
		{ TEXT("BufferName"),		AtomicBuffer_VarName + ParamInfo.DataInterfaceHLSLSymbol },
		{ TEXT("NumElementsName"),	NumElements_VarName + ParamInfo.DataInterfaceHLSLSymbol },
		{ TEXT("ResetValueName"),	ResetValue_VarName + ParamInfo.DataInterfaceHLSLSymbol },
	};
	OutHLSL += FString::Format(FormatDeclarations, ArgsDeclarations);
}

bool UNiagaraDataInterfaceAtomicBuffer::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
	const bool ParentRet = Super::GetFunctionHLSL(ParamInfo, FunctionInfo, FunctionInstanceIndex, OutHLSL);
	if (ParentRet)
	{
		return true;
	}

	TMap<FString, FStringFormatArg> ArgsBounds =
	{
		{ TEXT("FunctionName"),		FunctionInfo.InstanceName },
		{ TEXT("HLSLFunctionName"),	FunctionInfo.DefinitionName.ToString()},
		{ TEXT("BufferName"),		AtomicBuffer_VarName + ParamInfo.DataInterfaceHLSLSymbol },
		{ TEXT("NumElementsName"),	NumElements_VarName + ParamInfo.DataInterfaceHLSLSymbol },
		{ TEXT("ResetValueName"),	ResetValue_VarName + ParamInfo.DataInterfaceHLSLSymbol },
	};

	if (FunctionInfo.DefinitionName == InterlockedCompareExchange_FuncName)
	{
		static const TCHAR* FormatBounds = TEXT(R"(
			void {FunctionName}(in bool In_bEnabled, int In_Index, int In_CompareValue, int In_Value, out int Out_OriginalValue)
			{
				Out_OriginalValue = {ResetValueName};
				
				if (In_bEnabled && (In_Index < {NumElementsName}))
				{
					{HLSLFunctionName}(RW{BufferName}[In_Index], In_CompareValue, In_Value, Out_OriginalValue);
				}
			}
		)");
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);

		return true;
	}
	else if (FunctionInfo.DefinitionName == GetValue_FuncName)
	{
		static const TCHAR* FormatBounds = TEXT(R"(
			void {FunctionName}(int In_Index, out int Out_Value)
			{
				Out_Value = {ResetValueName};
				
				if ((In_Index < {NumElementsName}))
				{
					Out_Value = {BufferName}[In_Index];
				}
			}
		)");
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);

		return true;
	}
	else if (FunctionInfo.DefinitionName == SetValue_FuncName)
	{
		static const TCHAR* FormatBounds = TEXT(R"(
			void {FunctionName}(in bool In_bEnabled, int In_Index, in int In_Value)
			{
				if (In_bEnabled && (In_Index < {NumElementsName}))
				{
					RW{BufferName}[In_Index] = In_Value;
				}
			}
		)");
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);

		return true;
	}
	else
	{
		static const TCHAR* FormatBounds = TEXT(R"(
			void {FunctionName}(in bool In_bEnabled, int In_Index, int In_Value, out int Out_OriginalValue)
			{
				Out_OriginalValue = {ResetValueName};
				
				if (In_bEnabled && (In_Index < {NumElementsName}))
				{
					{HLSLFunctionName}(RW{BufferName}[In_Index], In_Value, Out_OriginalValue);
				}
			}
		)");
		OutHLSL += FString::Format(FormatBounds, ArgsBounds);

		return true;
	}

	return false;
}
#endif

void UNiagaraDataInterfaceAtomicBuffer::ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance)
{
	check(Proxy);

	FAtomicBufferInstanceData_GT* InstanceData = static_cast<FAtomicBufferInstanceData_GT*>(PerInstanceData);
	FAtomicBufferInstanceData_GTtoRT* DataToPass = new (DataForRenderThread) FAtomicBufferInstanceData_GTtoRT;
	
	DataToPass->ResetValue = InstanceData->ResetValue;
	DataToPass->ClearEveryFrame = InstanceData->ClearEveryFrame;
}

bool UNiagaraDataInterfaceAtomicBuffer::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FAtomicBufferInstanceData_GT* InstanceData_GT = new (PerInstanceData) FAtomicBufferInstanceData_GT();

	FNiagaraDataInterfaceProxyAtomicBuffer* RT_Proxy = GetProxyAs<FNiagaraDataInterfaceProxyAtomicBuffer>();

	InstanceData_GT->NumElements = FMath::Max(1, NumElements);
	InstanceData_GT->ResetValue = ResetValue;
	InstanceData_GT->ClearEveryFrame = ClearEveryFrame;
	InstanceData_GT->NeedsRealloc = false;

	ENQUEUE_RENDER_COMMAND(FUpdateData)(
		[RT_Proxy, GameThreadData=*InstanceData_GT, InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
	{
		check(!RT_Proxy->SystemInstancesToProxyData.Contains(InstanceID));
		FAtomicBufferInstanceData_RT* InstanceData_RT = &RT_Proxy->SystemInstancesToProxyData.Add(InstanceID);
		InstanceData_RT->NumElements = GameThreadData.NumElements;
		InstanceData_RT->ResetValue = GameThreadData.ResetValue;
		InstanceData_RT->ClearEveryFrame = GameThreadData.ClearEveryFrame;

		InstanceData_RT->ResizeBuffers();

		RHICmdList.ClearUAVUint(InstanceData_RT->AtomicBuffer.UAV, FUintVector4(InstanceData_RT->ResetValue, InstanceData_RT->ResetValue, InstanceData_RT->ResetValue, InstanceData_RT->ResetValue));
	});

	return true;
}

bool UNiagaraDataInterfaceAtomicBuffer::PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
	FAtomicBufferInstanceData_GT* InstanceData_GT = static_cast<FAtomicBufferInstanceData_GT*>(PerInstanceData);
	bool bNeedsReset = false;

	if (InstanceData_GT->NeedsRealloc && InstanceData_GT->NumElements > 0)
	{
		InstanceData_GT->NeedsRealloc = false;
		
		FNiagaraDataInterfaceProxyAtomicBuffer* RT_Proxy = GetProxyAs<FNiagaraDataInterfaceProxyAtomicBuffer>();
		ENQUEUE_RENDER_COMMAND(FUpdateData)(
			[RT_Proxy, GameThreadData = *InstanceData_GT, InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
		{
			check(RT_Proxy->SystemInstancesToProxyData.Contains(InstanceID));
			FAtomicBufferInstanceData_RT* InstanceData_RT = &RT_Proxy->SystemInstancesToProxyData.FindOrAdd(InstanceID);
			InstanceData_RT->NumElements = GameThreadData.NumElements;
			InstanceData_RT->ResetValue = GameThreadData.ResetValue;
			InstanceData_RT->ClearEveryFrame = GameThreadData.ClearEveryFrame;

			InstanceData_RT->ResizeBuffers();
		});
	}

	return false;
}


void UNiagaraDataInterfaceAtomicBuffer::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{		
	FAtomicBufferInstanceData_GT* InstanceData = static_cast<FAtomicBufferInstanceData_GT*>(PerInstanceData);
	InstanceData->~FAtomicBufferInstanceData_GT();

	FNiagaraDataInterfaceProxyAtomicBuffer* ThisProxy = GetProxyAs<FNiagaraDataInterfaceProxyAtomicBuffer>();
	if (!ThisProxy)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(FNiagaraDIDestroyInstanceData) (
		[ThisProxy, InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& CmdList)
	{
		//check(ThisProxy->SystemInstancesToProxyData.Contains(InstanceID));
		ThisProxy->SystemInstancesToProxyData.Remove(InstanceID);
	});
}

bool UNiagaraDataInterfaceAtomicBuffer::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceAtomicBuffer* OtherTyped = CastChecked<UNiagaraDataInterfaceAtomicBuffer>(Destination);

	OtherTyped->NumElements = NumElements;
	OtherTyped->ResetValue = ResetValue;
	OtherTyped->ClearEveryFrame = ClearEveryFrame;

	return true;
}

PRAGMA_ENABLE_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
