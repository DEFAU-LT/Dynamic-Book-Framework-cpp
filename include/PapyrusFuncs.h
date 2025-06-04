// PapyrusFuncs.h
#pragma once
#include "PCH.h"


namespace PapyrusFuncs {
    // Callback function to register all Papyrus native functions.
    bool SKSE_RegisterPapyrusFuncs_Callback(RE::BSScript::IVirtualMachine* a_vm);

    void ReloadDynamicBookINI(RE::StaticFunctionTag* base); 
    
}