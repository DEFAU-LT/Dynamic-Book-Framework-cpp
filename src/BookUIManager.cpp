//BookUIManager.cpp
#include "BookUIManager.h"
#include "BookMenuWatcher.h"
#include "Utility.h"            
#include "SetBookTextHook.h"  
#include "SessionDataManager.h" 
#include "PCH.h" 


namespace DynamicBookFramework {
    namespace BookUIManager {

        bool RefreshCurrentlyOpenBook() {
            auto* ui = RE::UI::GetSingleton();
            if (!ui || !ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
                logger::trace("BookUIManager: BookMenu not open or UI singleton not found. No refresh performed.");
                return false; // Not an error, just nothing to do
            }

            RE::GPtr<RE::IMenu> menuInstance = ui->GetMenu(RE::BookMenu::MENU_NAME);
            if (!menuInstance) {
                logger::warn("BookUIManager: Could not get BookMenu instance handle.");
                return false;
            }

            auto* bookMenu = static_cast<RE::BookMenu*>(menuInstance.get());
            if (!bookMenu) {
                logger::warn("BookUIManager: Could not cast to BookMenu.");
                return false;
            }

<<<<<<< Updated upstream
            // --- FIX: Call GetRuntimeData() and store as a reference. Use '.' for member access. ---
            auto& runtimeData = bookMenu->GetRuntimeData();
=======
            logger::info("BookUIManager: Triggering live refresh for '{}'...", currentBook->GetName());

            // --- THE SAFE LIVE REFRESH LOGIC ---

            // 1. Get the latest raw text from the file.
            auto* watcher = BookMenuWatcher::GetSingleton();
            watcher->ReloadAndCacheBook(currentBook); // Ensure cache is up-to-date
            std::string rawBookText = watcher->GetFullDynamicTextForBook(currentBook);

            //std::string rawBookText = DynamicBookFramework::SessionDataManager::GetSingleton()->GetFullContent(currentBook->GetName());


            // 2. Extract all image paths required by the new text.
            std::vector<std::string> requiredImagePaths = ExtractImagePathsFromText(rawBookText);
>>>>>>> Stashed changes
            
            // Accessing runtimeData.book which is the GFxMovieView GPtr
            if (!runtimeData.book) { 
                logger::warn("BookUIManager: BookMenu's runtimeData.book (uiMovie) is null.");
                return false;
            }

            RE::GFxMovieView* currentMovieView = runtimeData.book.get(); // Get raw pointer from GPtr
            if (!currentMovieView) {
                logger::warn("BookUIManager: runtimeData.book internal pointer is null.");
                return false;
            }

            RE::TESObjectBOOK* currentBook = RE::BookMenu::GetTargetForm(); // Static function from RE::BookMenu
            if (!currentBook) {
                logger::warn("BookUIManager: Could not get target form for the currently open book.");
                return false;
            }

            std::string bookTitle = currentBook->GetFullName() ? currentBook->GetFullName() : "";
            RE::FormID bookFormID = currentBook->GetFormID();
            logger::info("BookUIManager: Attempting to refresh content for book: '{}' (FormID: {:X})", bookTitle, bookFormID);

            // Step 1: Tell BookMenuWatcher to reload this book's content from its .txt file
            bool recached = BookMenuWatcher::GetSingleton()->ReloadAndCacheBook(currentBook); 
            if (!recached) {
                logger::warn("BookUIManager: Failed to reload/re-cache book content for '{}'. Refresh aborted.", bookTitle);
                return false; 
            }

            // Step 2: Get the now fresh HTML from BookMenuWatcher's cache
            auto htmlOpt = BookMenuWatcher::GetSingleton()->GetCachedHtmlForBook(bookFormID); 
            if (!htmlOpt) { 
                logger::error("BookUIManager: Failed to get cached HTML for '{}' after successful reload.", bookTitle);
                return false; 
            }
            const std::string& fullHtmlToShow = *htmlOpt;
            logger::trace("BookUIManager: Retrieved fresh HTML from cache (length: {}).", fullHtmlToShow.length());

            // Step 3: Prepare FxResponseArgs and call the game's SetBookText thunk
            // --- FIX: Accessing isNote through the runtimeData reference ---
            bool isNote = runtimeData.isNote;
            
            RE::FxResponseArgsEx<2> fxArgs;
            fxArgs[0].SetString(fullHtmlToShow.c_str()); 
            fxArgs[1].SetBoolean(isNote);

            if (SetBookTextHook::g_rawOriginalThunkPtr) {
                logger::info("BookUIManager: Re-invoking original SetBookText thunk with new content for '{}'.", bookTitle);
                SetBookTextHook::g_rawOriginalThunkPtr(currentMovieView, "SetBookText", &fxArgs, 0);

                // SKSE::ModCallbackEvent modEvent{ "DBF_onToggleInputMode", "", 0.0f, nullptr };
                // auto* modEventSource = SKSE::GetModCallbackEventSource();
                // if (modEventSource) {
                //     modEventSource->SendEvent(&modEvent);
                //     logger::info("BookUIManager: Sent 'DBF_onToggleInputMode' event to SWF.");
                // } else {
                //     logger::error("BookUIManager: Could not get SKSE ModCallbackEventSource to send update event.");
                // }
                return true;
            } else {
                logger::error("BookUIManager: Original SetBookText thunk pointer (g_rawOriginalThunkPtr) is null. Cannot refresh book content.");
                return false;
            }
        }

    } // namespace BookUIManager
        
}