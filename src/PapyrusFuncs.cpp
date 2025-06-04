// PapyrusFuncs.cpp
#include "PapyrusFuncs.h"
#include "Utility.h"      // For logger alias (if SetupLog is there) and string utils if needed for other Papyrus funcs
#include "PCH.h"


namespace {
    std::wstring g_iniPath = L"Data\\SKSE\\Plugins\\DynamicBookFramework.ini";
}

namespace { // Anonymous namespace for file-local native function implementations

    // Papyrus_AppendToFile now interprets its first argument as a book title key
    bool Papyrus_AppendToFile(RE::StaticFunctionTag*, RE::BSFixedString bookTitleKeyBS, RE::BSFixedString textToAppend) {
        if (!bookTitleKeyBS.c_str() || bookTitleKeyBS.empty()) {
            logger::warn("AppendToFile called with an empty bookTitleKey.");
            return false;
        }
        if (!textToAppend.c_str()) { // textToAppend can be empty, but not null
             logger::warn("AppendToFile called with null textToAppend.");
            return false;
        }

        std::string bookTitleKey_utf8(bookTitleKeyBS.c_str());
        std::wstring bookTitleKey_wstr = string_to_wstring(bookTitleKey_utf8); // Use your utility

        //logger::info("AppendToFile called for book key: '{}'", bookTitleKey_utf8.c_str());

        // 1. Resolve the book title key to a full file path using your INI mapping
        std::optional<std::wstring> resolvedPathOpt = GetDynamicBookPathByTitle(bookTitleKey_wstr);

        if (!resolvedPathOpt) {
            logger::error("Failed to resolve book key '{}' to a file path. Check your INI mappings.", bookTitleKey_utf8.c_str());
            return false;
        }

        std::string actualFilePath_utf8 = wstring_to_utf8(*resolvedPathOpt);
        logger::info("Resolved path for key '{}' is '{}'", bookTitleKey_utf8.c_str(), actualFilePath_utf8.c_str());
        
        // 2. The rest of the file appending logic, using actualFilePath_utf8

        // Check if directory exists, create if not (C++17 filesystem)
        try {
            std::filesystem::path p = actualFilePath_utf8; // Use the resolved path
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
        } catch (const std::exception& e) {
            logger::error("Filesystem error checking/creating directory for '{}': {}", actualFilePath_utf8.c_str(), e.what());
            // Continue anyway, maybe path is okay or will fail at open()
        }

        std::ofstream outfile;
        outfile.open(actualFilePath_utf8.c_str(), std::ios_base::app); // Use the resolved path
        if (!outfile.is_open()) {
            logger::error("Failed to open file for appending: {}", actualFilePath_utf8.c_str());
            return false;
        }
        
        outfile.seekp(0, std::ios::end);
        long long fileSize = outfile.tellp();
        if (fileSize > 0 && !std::string(textToAppend.c_str()).empty()) { // Only add newline if file not empty AND new text not empty
             outfile << "\n"; 
        }
        outfile << textToAppend.c_str();
        outfile.close();

        if (!outfile.good()) { // Check for errors after closing
            logger::error("Error occurred while writing/closing file: {}", actualFilePath_utf8.c_str());
            return false;
        }
        
        logger::info("Appended text successfully to '{}'.", actualFilePath_utf8.c_str());
        return true;
    }

    void Papyrus_ReloadDynamicBookINI(RE::StaticFunctionTag* /*base*/) {
        logger::info("Papyrus_ReloadDynamicBookINI called from Papyrus. Reloading INI mappings...");
        LoadBookMappings(g_iniPath); // Call your utility function
        RE::DebugNotification("Dynamic book INI reloaded."); 
    }

} // end anonymous namespace

namespace PapyrusFuncs {

    bool SKSE_RegisterPapyrusFuncs_Callback(RE::BSScript::IVirtualMachine* a_vm) {
        if (!a_vm) {
            logger::error("Papyrus VM is null, cannot register native functions from PapyrusFuncs.");
            return false;
        }

        // Make sure "DBF_ScriptUtil" is the name of your Papyrus script that declares these functions as native.
        a_vm->RegisterFunction("AppendToFile", "DBF_ScriptUtil", Papyrus_AppendToFile);
        //logger::info("Registered Papyrus native function 'AppendToFile' (key-based) for DBF_ScriptUtil.");
        
        // Register other functions here if you add more
        // a_vm->RegisterFunction("MyOtherFunction", "MyPluginScript", MyOtherPapyrusNativeFunction);
        return true;
    }

} // end namespace PapyrusFuncs