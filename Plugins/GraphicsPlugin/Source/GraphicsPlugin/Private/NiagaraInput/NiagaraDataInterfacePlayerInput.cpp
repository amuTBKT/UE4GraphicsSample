#include "NiagaraInput/NiagaraDataInterfacePlayerInput.h"

#include "NiagaraShader.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstanceBatcher.h"

#include "KeyState.h"
#include "GameFramework/PlayerInput.h"

#define LOCTEXT_NAMESPACE "UNiagaraDataInterfacePlayerInput"

const FName UNiagaraDataInterfacePlayerInput::InputKeyNameSpecifier_VarName(TEXT("KeyName"));

// function names
const FName UNiagaraDataInterfacePlayerInput::GetInputAxisValue_FuncName(TEXT("GetInputAxisValue"));
const FName UNiagaraDataInterfacePlayerInput::IsInputKeyDown_FuncName(TEXT("IsInputKeyDown"));
const FName UNiagaraDataInterfacePlayerInput::WasInputKeyJustPressed_FuncName(TEXT("WasInputKeyJustPressed"));
const FName UNiagaraDataInterfacePlayerInput::WasInputKeyJustReleased_FuncName(TEXT("WasInputKeyJustReleased"));

//------------------------------------------------------------------------------------------------------------

struct FPerInstanceData_GT
{
    TWeakObjectPtr<APlayerController> PlayerController = nullptr;
    int32 PlayerControllerIndex = -1;
};

int32 UNiagaraDataInterfacePlayerInput::PerInstanceDataSize() const
{
    return sizeof(FPerInstanceData_GT);
}

//------------------------------------------------------------------------------------------------------------

UNiagaraDataInterfacePlayerInput::UNiagaraDataInterfacePlayerInput(FObjectInitializer const& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Proxy.Reset(nullptr);
}

void UNiagaraDataInterfacePlayerInput::PostInitProperties()
{
    Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
    }
}

bool UNiagaraDataInterfacePlayerInput::CopyToInternal(UNiagaraDataInterface* Destination) const
{
    if (!Super::CopyToInternal(Destination))
    {
        return false;
    }

    UNiagaraDataInterfacePlayerInput* DstDataInterface = CastChecked<UNiagaraDataInterfacePlayerInput>(Destination);
    DstDataInterface->PlayerControllerIndex = PlayerControllerIndex;

    return true;
}

bool UNiagaraDataInterfacePlayerInput::Equals(const UNiagaraDataInterface* Other) const
{
    if (!Super::Equals(Other))
    {
        return false;
    }

    const UNiagaraDataInterfacePlayerInput* OtherDataInterface = CastChecked<const UNiagaraDataInterfacePlayerInput>(Other);

    return OtherDataInterface->PlayerControllerIndex == PlayerControllerIndex;
}

ETickingGroup UNiagaraDataInterfacePlayerInput::CalculateTickGroup(const void* PerInstanceData) const
{
    return ETickingGroup::TG_PostUpdateWork;
}

bool UNiagaraDataInterfacePlayerInput::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
    FPerInstanceData_GT* InstanceData = new (PerInstanceData) FPerInstanceData_GT();
    SystemInstancesToProxyData_GT.Emplace(SystemInstance->GetId(), InstanceData);

    InstanceData->PlayerController.Reset();
    InstanceData->PlayerControllerIndex = -1;
    
    return true;
}

void UNiagaraDataInterfacePlayerInput::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
    SystemInstancesToProxyData_GT.Remove(SystemInstance->GetId());

    FPerInstanceData_GT* InstanceData = static_cast<FPerInstanceData_GT*>(PerInstanceData);

    InstanceData->~FPerInstanceData_GT();
}


bool UNiagaraDataInterfacePlayerInput::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
    FPerInstanceData_GT* InstanceData = static_cast<FPerInstanceData_GT*>(PerInstanceData);

    if ((!InstanceData->PlayerController.IsValid()) || (PlayerControllerIndex != InstanceData->PlayerControllerIndex))
    {
        UWorld* World = SystemInstance->GetWorld();
        if (World && PlayerControllerIndex < World->GetNumPlayerControllers())
        {
            int32 i = 0;
            for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
            {
                APlayerController* PlayerController = Iterator->Get();
                if (i == PlayerControllerIndex && PlayerController)
                {
                    InstanceData->PlayerController = PlayerController;

                    return false;
                }
                i++;
            }
        }
    }

    return false;
}

void UNiagaraDataInterfacePlayerInput::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    Super::GetFunctions(OutFunctions);

    auto GetDefaultFunctionSignature = [this](FName FuncName) -> FNiagaraFunctionSignature
    {
        FNiagaraFunctionSignature Sig;
        Sig.Name = FuncName;
        Sig.bMemberFunction = true;
        Sig.bRequiresContext = false;
        Sig.bSupportsCPU = true;
        Sig.bSupportsGPU = false;
        Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("PlayerInput")));

        Sig.FunctionSpecifiers.Add(UNiagaraDataInterfacePlayerInput::InputKeyNameSpecifier_VarName);

        Sig.bExperimental = true;
        Sig.ExperimentalMessage = NSLOCTEXT("Niagara", "PlayerInputDIFunctionExperimental", "This DataInterface was just a test, please don't use this at work ;)");

        return Sig;
    };

    {
        FNiagaraFunctionSignature Sig = GetDefaultFunctionSignature(UNiagaraDataInterfacePlayerInput::GetInputAxisValue_FuncName);
        Sig.SetDescription(LOCTEXT("GetInputAxisValue", "Returns the analog value for the given key/button."));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Value")));
        OutFunctions.Add(Sig);
    }

    {
        FNiagaraFunctionSignature Sig = GetDefaultFunctionSignature(UNiagaraDataInterfacePlayerInput::IsInputKeyDown_FuncName);
        Sig.SetDescription(LOCTEXT("IsInputKeyDown", "Returns true if the given key/button is pressed on the input of the controller (if present)."));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Value")));        
        OutFunctions.Add(Sig);
    }

    {
        FNiagaraFunctionSignature Sig = GetDefaultFunctionSignature(UNiagaraDataInterfacePlayerInput::WasInputKeyJustPressed_FuncName);
        Sig.SetDescription(LOCTEXT("WasInputKeyJustPressed", "Returns true if the given key/button was up last frame and down this frame."));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Value")));
        OutFunctions.Add(Sig);
    }

    {
        FNiagaraFunctionSignature Sig = GetDefaultFunctionSignature(UNiagaraDataInterfacePlayerInput::WasInputKeyJustReleased_FuncName);
        Sig.SetDescription(LOCTEXT("WasInputKeyJustReleased", "Returns true if the given key/button was down last frame and up this frame."));
        Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Value")));
        OutFunctions.Add(Sig);
    }
}

void UNiagaraDataInterfacePlayerInput::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc)
{
    Super::GetVMExternalFunction(BindingInfo, InstanceData, OutFunc);
    
    if (BindingInfo.Name == UNiagaraDataInterfacePlayerInput::GetInputAxisValue_FuncName)
    {
        const FName InputKeyName = BindingInfo.FindSpecifier(UNiagaraDataInterfacePlayerInput::InputKeyNameSpecifier_VarName)->Value;
        const FKey InputKey = FKey(InputKeyName);
    
        check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 1);
        OutFunc = FVMExternalFunction::CreateLambda([InputKey](FVectorVMContext& Context)
        {
            VectorVM::FUserPtrHandler<FPerInstanceData_GT> InstanceData(Context);
            FNDIOutputParam<float> OutValues(Context);

            // this is same for all the instances/threads
            float Value = 0.f;
            if (APlayerController* PlayerController = InstanceData->PlayerController.Get())
            {
                Value = PlayerController->GetInputAnalogKeyState(InputKey);
            }

            for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
            {
                OutValues.SetAndAdvance(Value);
            }
        });
    }
    else //boolean functions
    {
        using GetKeyStateBoolFunc = bool(APlayerController::*)(FKey) const;
        GetKeyStateBoolFunc Func = nullptr;

        if (BindingInfo.Name == UNiagaraDataInterfacePlayerInput::IsInputKeyDown_FuncName)
        {
            check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 1);
            Func = &APlayerController::IsInputKeyDown;
        }
        else if (BindingInfo.Name == UNiagaraDataInterfacePlayerInput::WasInputKeyJustPressed_FuncName)
        {
            check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 1);
            Func = &APlayerController::WasInputKeyJustPressed;            
        }
        else if (BindingInfo.Name == UNiagaraDataInterfacePlayerInput::WasInputKeyJustReleased_FuncName)
        {
            check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 1);
            Func = &APlayerController::WasInputKeyJustReleased;            
        }

        if (Func)
        {
            const FName InputKeyName = BindingInfo.FindSpecifier(UNiagaraDataInterfacePlayerInput::InputKeyNameSpecifier_VarName)->Value;
            const FKey InputKey = FKey(InputKeyName);

            OutFunc = FVMExternalFunction::CreateLambda([InputKey, Func](FVectorVMContext& Context)
            {
                VectorVM::FUserPtrHandler<FPerInstanceData_GT> InstanceData(Context);
                FNDIOutputParam<bool> OutValues(Context);

                // this is same for all the instances/threads
                bool Value = false;
                if (APlayerController* PlayerController = InstanceData->PlayerController.Get())
                {
                    Value = (PlayerController->*Func)(InputKey);
                }

                for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
                {
                    OutValues.SetAndAdvance(Value);
                }
            });
        }
    }
}

#undef LOCTEXT_NAMESPACE