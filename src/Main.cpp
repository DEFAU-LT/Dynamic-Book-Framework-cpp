#include "Utility.h"
#include "BookMenuWatcher.h"
#include "PapyrusFuncs.h"
#include "SetBookTextHook.h"
#include "Version.h" 



// --- Global Variables ---
namespace {
    std::wstring g_iniPath = L"Data\\SKSE\\Plugins\\DynamicBookFramework.ini";
}


// --- SKSE Message Handler ---
void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
    switch (a_msg->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            //logger::info("Received kDataLoaded message.");

            // Register Papyrus functions using the new callback
            if (auto* papyrus = SKSE::GetPapyrusInterface()) {
                papyrus->Register(PapyrusFuncs::SKSE_RegisterPapyrusFuncs_Callback); // Use the new namespaced function
            } else {
                logger::critical("Could not get Papyrus interface! Native functions not registered.");
            }

            // Register Menu Event Listener
            if (auto* ui = RE::UI::GetSingleton()) {
                ui->AddEventSink(BookMenuWatcher::GetSingleton());
                //logger::info("Registered BookMenuWatcher event sink.");
            } else {
                logger::critical("Could not get UI interface! Menu watcher not registered.");
            }

            // Load dynamic book mappings from INI
            LoadBookMappings(g_iniPath);

            if (SetBookTextHook::Install()) {
                //logger::info("SetBookText hook successfully installed directly.");
            } else {
                logger::error("Failed to install SetBookText hook!");
            }

            break;
    }
}

// --- SKSE Plugin Load Entry Point ---
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    // 1. Initialize Logging FIRST
    SetupLog();
    // Now log with simpler arguments
    //logger::info("{} loading...", Version::PROJECT);

    // 2. Initialize SKSE interfaces
    SKSE::Init(a_skse);

    SKSE::AllocTrampoline(128); // Request 128 bytes, for example. Adjust as needed.
    //logger::info("Requested 128 bytes for SKSE trampoline.");

    // 3. Register SKSE Message Listener (for kDataLoaded)
    auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener("SKSE", MessageHandler)) {
        logger::critical("Could not register SKSE Message Listener!");
        return false; // Critical failure if we can't get messages for initialization
    }
    logger::info("{} loaded successfully.", Version::PROJECT);
    return true;
}