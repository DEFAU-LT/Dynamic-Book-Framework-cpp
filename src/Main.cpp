#include "BookMenuWatcher.h"
#include "Utility.h"
#include "PapyrusFuncs.h"
#include "SetBookTextHook.h"
#include "FileWatcher.h"
#include "SessionDataManager.h"
#include "API.h"
#include "Version.h" 



// --- Global Variables ---
namespace {
    std::wstring g_iniPath = L"Data\\SKSE\\Plugins\\DynamicBookFramework.ini";
}

void ApiMessageHandler(SKSE::MessagingInterface::Message* a_msg) {
    if (!a_msg) {
        return;
    }

    switch (a_msg->type) {
        case DynamicBookFramework_API::kAppendEntry:
            {
                logger::info("Received AppendEntry API call from plugin: {}", a_msg->sender);
                
                // Validate message data before using it
                if (a_msg->data && a_msg->dataLen == sizeof(DynamicBookFramework_API::AppendEntryMessage)) {
                    auto* messageData = static_cast<DynamicBookFramework_API::AppendEntryMessage*>(a_msg->data);
                    
                    if (messageData->bookTitleKey && messageData->textToAppend) {
                        // Call the core framework function with the received data
                        DynamicBookFramework::SessionDataManager::GetSingleton()->AppendEntry(
                            messageData->bookTitleKey,
                            messageData->textToAppend
                        );
                    } else {
                        logger::error("AppendEntry API message received with null data fields.");
                    }
                } else {
                    logger::error("AppendEntry API message received with invalid data or data length.");
                }
            }
            break;
        default:
            logger::warn("Received unknown API message of type: {}", a_msg->type);
            break;
    }
}

// --- SKSE Message Handler ---
void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
    switch (a_msg->type) {
        case SKSE::MessagingInterface::kPostLoad:
            {
                logger::info("Received kPostLoad message. Registering API listener...");
                auto* messaging = SKSE::GetMessagingInterface();
                if (messaging && messaging->RegisterListener(NULL, ApiMessageHandler)) {
                        logger::info("Successfully registered API listener for messages targeting '{}' from DynamicJournal.", Version::PROJECT);
                } else {
                        logger::warn("Could not register API listener!");
                }
            }
            break;
        
        case SKSE::MessagingInterface::kDataLoaded:
            //logger::info("Received kDataLoaded message.");
            {
                DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad("MainMenu");
                
                // Register Papyrus functions using the new callback
                if (auto* papyrus = SKSE::GetPapyrusInterface()) {
                    papyrus->Register(PapyrusFuncs::SKSE_RegisterPapyrusFuncs_Callback); // Use the new namespaced function
                } else {
                    logger::critical("Could not get Papyrus interface! Native functions not registered.");
                }

                // Register Menu Event Listener
                if (auto* ui = RE::UI::GetSingleton()) {
                    ui->AddEventSink(DynamicBookFramework::BookMenuWatcher::GetSingleton());
                    logger::info("Registered BookMenuWatcher event sink.");
                } else {
                    logger::critical("Could not get UI interface! Menu watcher not registered.");
                }

                // Load dynamic book mappings from INI
                LoadBookMappings(g_iniPath);

                DynamicBookFramework::FileWatcher::Start();

                if (SetBookTextHook::Install()) {
                    //logger::info("SetBookText hook successfully installed directly.");
                } else {
                    logger::error("Failed to install SetBookText hook!");
                }
            }
            break;

        case SKSE::MessagingInterface::kNewGame:
            {
                logger::info("Received kNewGame message. Initializing for new journal.");
                std::string saveName = static_cast<const char*>(a_msg->data);
                DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad(saveName); // No message data for new game
            }
            break;

        case SKSE::MessagingInterface::kPreLoadGame:
            {
                logger::info("Received kPostLoadGame message. Initializing journal for loaded save.");
                //(a_msg->data && reinterpret_cast<uintptr_t>(a_msg->data) > 0x10000);
                //std::string saveName = static_cast<const char*>(a_msg->data);
                std::string saveName = static_cast<const char*>(a_msg->data);
                DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad(saveName);
            }
            break;
        
        case SKSE::MessagingInterface::kSaveGame:
            {
                logger::info("Received kSaveGame event. Committing pending journal entries.");
                std::string saveName = static_cast<const char*>(a_msg->data);
                // Tell SessionDataManager what the current save is, just in case it changed.
                //DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad(saveName);
                // Now commit the buffered data.
                DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameSave(saveName);
            }
            break;
        case DynamicBookFramework_API::kAppendEntry:
            {
                logger::info("Received AppendEntry API call from plugin: {}", a_msg->sender);
                if (a_msg->data && a_msg->dataLen == sizeof(DynamicBookFramework_API::AppendEntryMessage)) {
                    auto* messageData = static_cast<DynamicBookFramework_API::AppendEntryMessage*>(a_msg->data);
                    if (messageData->bookTitleKey && messageData->textToAppend) {
                        DynamicBookFramework::SessionDataManager::GetSingleton()->AppendEntry(
                            messageData->bookTitleKey,
                            messageData->textToAppend
                        );
                    } else {
                        logger::error("AppendEntry API message received with null data fields.");
                    }
                } else {
                    logger::error("AppendEntry API message received with invalid data or data length.");
                }
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
