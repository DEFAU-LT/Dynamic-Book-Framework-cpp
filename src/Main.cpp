#include "BookMenuWatcher.h"
#include "Utility.h"
#include "PapyrusFuncs.h"
#include "SetBookTextHook.h"
#include "FileWatcher.h"
#include "SessionDataManager.h"
#include "API.h"
#include "ImGuiMenu.h"
#include "InputListener.h"
#include "Settings.h"
<<<<<<< Updated upstream
=======
#include "ModEventHandler.h"
>>>>>>> Stashed changes
#include "Version.h" 

// --- API Message Handler ---
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
                
                // Register the API listener
                if (messaging && messaging->RegisterListener(NULL, ApiMessageHandler)) {
                        logger::info("Successfully registered API listener for messages targeting '{}'.", Version::PROJECT);
                } else {
                        logger::warn("Could not register API listener!");
                }
            }
            break;
        
        case SKSE::MessagingInterface::kDataLoaded:
            {
                logger::info("kDataLoaded: Initializing framework components...");
                DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad("MainMenu");
                
                InputListener::Install();
                ImGuiRender::InitializeSKSEMenuFramework();
                ImGuiRender::Register();
                
                if (auto* papyrus = SKSE::GetPapyrusInterface()) {
                    papyrus->Register(PapyrusFuncs::SKSE_RegisterPapyrusFuncs_Callback);
                }
                if (auto* ui = RE::UI::GetSingleton()) {
                    ui->AddEventSink(DynamicBookFramework::BookMenuWatcher::GetSingleton());
                }

                LoadBookMappings();
                DynamicBookFramework::FileWatcher::Start();
<<<<<<< Updated upstream
                SetBookTextHook::Install();
=======
                BookHooks::Install();
>>>>>>> Stashed changes
                Settings::LoadSettings();

                ModEventHandler::Register();

                
            }
            break;

        // Handle NewGame, PreLoadGame, and SaveGame events...            
        case SKSE::MessagingInterface::kNewGame:
            DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad(static_cast<const char*>(a_msg->data));
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameLoad(static_cast<const char*>(a_msg->data));
            break;
        case SKSE::MessagingInterface::kSaveGame:
            DynamicBookFramework::SessionDataManager::GetSingleton()->OnGameSave(static_cast<const char*>(a_msg->data));
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
