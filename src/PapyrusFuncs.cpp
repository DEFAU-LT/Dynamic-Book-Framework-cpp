// PapyrusFuncs.cpp
#include "PapyrusFuncs.h"
#include "Utility.h"
#include "SessionDataManager.h"
#include "PCH.h"


namespace { // Anonymous namespace for file-local native function implementations

    // This is the native implementation of your API function.
    // Papyrus scripts will call "AppendToFile" (or whatever you name it).
    // It now uses the SessionDataManager instead of writing to disk directly.
    void Papyrus_AppendToFile(RE::StaticFunctionTag* /*base*/, RE::BSFixedString bookTitleKeyBS, RE::BSFixedString textToAppend) {
        if (!bookTitleKeyBS.c_str() || bookTitleKeyBS.empty()) {
            logger::warn("Papyrus AppendToFile called with an empty bookTitleKey.");
            return; // Papyrus functions that don't return a value are void
        }
        if (!textToAppend.c_str()) {
             logger::warn("Papyrus AppendToFile called with null textToAppend.");
            return;
        }

        std::string bookFileKey = bookTitleKeyBS.c_str();
        std::string entry = textToAppend.c_str();
        
        logger::info("Papyrus AppendToFile called for file key: '{}'. Adding to session buffer.", bookFileKey);

        // --- THE KEY CHANGE ---
        // Instead of handling file I/O here, we call the framework's core service.
        // This service will handle the buffering, save-game states, and writing to disk on save.
        DynamicBookFramework::SessionDataManager::GetSingleton()->AppendEntry(bookFileKey, entry);
    }

    void Papyrus_ReloadDynamicBookINI(RE::StaticFunctionTag* /*base*/) {
        logger::info("Papyrus_ReloadDynamicBookINI called. Reloading INI mappings...");
        LoadBookMappings();
        // RE::DebugNotification("Dynamic book INI reloaded."); // Optional feedback
    }

} // end anonymous namespace

namespace PapyrusFuncs {

    bool SKSE_RegisterPapyrusFuncs_Callback(RE::BSScript::IVirtualMachine* a_vm) {
        if (!a_vm) {
            logger::error("Papyrus VM is null, cannot register native functions.");
            return false;
        }

        // The script name "DBF_ScriptUtil" must match the .psc file.
        a_vm->RegisterFunction("AppendToFile", "DBF_ScriptUtil", Papyrus_AppendToFile);
        a_vm->RegisterFunction("ReloadDynamicBookINI", "DBF_ScriptUtil", Papyrus_ReloadDynamicBookINI);
        
        logger::info("Registered Papyrus native functions for DBF_ScriptUtil.");
        return true;
    }

} // end namespace PapyrusFuncs