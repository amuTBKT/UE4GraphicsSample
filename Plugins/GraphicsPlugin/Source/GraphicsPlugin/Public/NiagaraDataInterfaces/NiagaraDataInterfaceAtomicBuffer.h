// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterfaceRW.h"
#include "NiagaraCommon.h"

#include "NiagaraDataInterfaceAtomicBuffer.generated.h"

class FNiagaraSystemInstance;

UCLASS(EditInlineNew, Category = "Buffer", meta = (DisplayName = "Atomic Buffer"))
class GRAPHICSPLUGIN_API UNiagaraDataInterfaceAtomicBuffer : public UNiagaraDataInterfaceRWBase
{
	GENERATED_UCLASS_BODY()

public:

	DECLARE_NIAGARA_DI_PARAMETER();

	virtual void PostInitProperties() override
	{
		Super::PostInitProperties();

		//Can we register data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
		if (HasAnyFlags(RF_ClassDefaultObject))
		{
			ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
			FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
		}
	}

	//~ UNiagaraDataInterface interface
	// VM functionality
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc) override;

	virtual bool Equals(const UNiagaraDataInterface* Other) const override;

	// GPU sim functionality
#if WITH_EDITORONLY_DATA
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
	virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
#endif

	virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override { return false; }
	virtual int32 PerInstanceDataSize() const override;
	virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual bool HasPostSimulateTick() const override { return true; }
	virtual bool HasPreSimulateTick() const override { return true; }
	//~ UNiagaraDataInterface interface END

protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
	
public:
	UPROPERTY(EditAnywhere)
	int32 NumElements = 0;

	UPROPERTY(EditAnywhere)
	int32 ResetValue = 0;

	UPROPERTY(EditAnywhere)
	bool ClearEveryFrame = true;

public:
	// variable names
	static const FString NumElements_VarName;
	static const FString ResetValue_VarName;
	static const FString AtomicBuffer_VarName;

	// function names
	static const FName SetNumElements_FuncName;
	static const FName SetResetValue_FuncName;
	static const FName SetClearEveryFrame_FuncName;

	static const FName InterlockedAdd_FuncName;
	static const FName InterlockedMin_FuncName;
	static const FName InterlockedMax_FuncName;
	static const FName InterlockedOr_FuncName;
	static const FName InterlockedAnd_FuncName;
	static const FName InterlockedXor_FuncName;
	static const FName InterlockedCompareStore_FuncName;
	static const FName InterlockedCompareExchange_FuncName;
	static const FName InterlockedExchange_FuncName;
	static const FName GetValue_FuncName;
	static const FName SetValue_FuncName;
};


