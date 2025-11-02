#pragma once
#include "pch.h"
#include <d3d12.h>
#include <dxgi1_4.h>

typedef void (*ProcessEvent_t)(SDK::UObject*, SDK::UFunction*, void*);

namespace Hooks
{
    void Initialize();
    void Shutdown();

    void hkProcessEvent(SDK::UObject* pThis, SDK::UFunction* Function, void* Parms);
}
