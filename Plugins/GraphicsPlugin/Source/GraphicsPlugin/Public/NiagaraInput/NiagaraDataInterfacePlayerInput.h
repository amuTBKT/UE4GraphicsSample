#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "NiagaraDataInterfacePlayerInput.generated.h"

struct FPerInstanceData_GT;

//------------------------------------------------------------------------------------------------------------

/** Data Interface for rendering InstancedStaticMesh */
UCLASS(EditInlineNew, Category = "PlayerInput", meta = (DisplayName = "PlayerInput"), Blueprintable, BlueprintType)
class UNiagaraDataInterfacePlayerInput : public UNiagaraDataInterface
{
    GENERATED_UCLASS_BODY()

public:
    //UObject Interface
    virtual void PostInitProperties() override;
    //UObject Interface End

    virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return Target == ENiagaraSimTarget::CPUSim; }
    virtual bool Equals(const UNiagaraDataInterface* Other) const override;
    
    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;

    // VM functions
    virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;

    virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override {}
    virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
    virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
    virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
    virtual int32 PerInstanceDataSize() const override;
    virtual bool HasPreSimulateTick() const override { return true; }

    virtual bool HasTickGroupPrereqs() const override { return true; }
    virtual ETickingGroup CalculateTickGroup(const void* PerInstanceData) const override;

protected:
    virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;

protected:
    TMap<FNiagaraSystemInstanceID, FPerInstanceData_GT*> SystemInstancesToProxyData_GT;

    UPROPERTY(EditAnywhere)
    int32 PlayerControllerIndex = 0;

private:
    static const FName InputKeyNameSpecifier_VarName;
    
    // function names
    static const FName GetInputAxisValue_FuncName;
    static const FName IsInputKeyDown_FuncName;
    static const FName WasInputKeyJustPressed_FuncName;
    static const FName WasInputKeyJustReleased_FuncName;
};