#include "BookMenuWatcher.h"
#include "FileWatcher.h"
#include "SessionDataManager.h"
#include "Utility.h"
#include "BookUIManager.h"
#include "PCH.h"


namespace DynamicBookFramework {

	BookMenuWatcher* BookMenuWatcher::GetSingleton() {
		static BookMenuWatcher singleton;
		return &singleton;
	}

	RE::BSEventNotifyControl BookMenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
		if (!a_event || a_event->menuName != RE::BookMenu::MENU_NAME) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->opening) {
			if (auto* currentBookObject = RE::BookMenu::GetTargetForm()) {
				this->PrepareAndCacheBookContent(currentBookObject);
			} else {
				logger::warn("BookMenuWatcher::ProcessEvent: RE::BookMenu::GetTargetForm() returned null.");
			}
		} else {
			if (auto lastOpenedTitle = GetLastOpenedDynamicBookTitle(); !lastOpenedTitle.empty()) {
				logger::info("BookMenuWatcher: Book menu closing. Stopping file watch for book '{}'.", lastOpenedTitle);
				FileWatcher::StopMonitoringBookFile(lastOpenedTitle);
				this->ClearLastOpenedBook(); 
			}
		}
		
		return RE::BSEventNotifyControl::kContinue;
	}

	void BookMenuWatcher::PrepareAndCacheBookContent(RE::TESObjectBOOK* bookToPrepare) {
		if (!bookToPrepare) {
            return;
        }
		
		const char* bookTitleCStr = bookToPrepare->GetFullName();
		if (!bookTitleCStr || *bookTitleCStr == '\0') {
            return;
        }
		std::string currentTitle(bookTitleCStr);
		RE::FormID currentFormID = bookToPrepare->GetFormID();
		
		std::wstring wCurrentTitle = string_to_wstring(currentTitle);
		auto dynamicBookPathOpt = GetDynamicBookPathByTitle(wCurrentTitle);
        
		if (dynamicBookPathOpt) {
			const std::wstring& dynamicBookPath_w = *dynamicBookPathOpt;
			
			FileWatcher::MonitorBookFile(currentTitle, dynamicBookPath_w);
			this->SetLastOpenedBook(currentFormID, currentTitle); 

			// --- FIX: Use SessionDataManager to get the full, combined content ---
			// The fileKey for the personal journal should be the book's title to match the API call.
            std::string fileKey = currentTitle; 
			std::string fileContent = SessionDataManager::GetSingleton()->GetFullContent(fileKey);
            logger::info("BookMenuWatcher: Loaded combined content for key '{}'. Total length: {}", fileKey, fileContent.length());
            
			// Process the final content from SessionDataManager
			std::string textToStoreForBook;
			if (fileContent.rfind(";;RAW_HTML;;", 0) == 0) {
                logger::info("BookMenuWatcher: Raw HTML marker found. Using content as-is after marker.");
				size_t markerLineEnd = fileContent.find('\n');
                if (markerLineEnd != std::string::npos) {
                    textToStoreForBook = fileContent.substr(markerLineEnd + 1);
                } else { 
                    textToStoreForBook = ""; 
                }
			} else {
                logger::info("BookMenuWatcher: No raw HTML marker. Applying general markup.");
                std::string body = HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(fileContent);
                std::string defaultFontFace = "$HandWrittenFont"; 
                int defaultFontSize = 20;
                textToStoreForBook = "<font face=\"" + defaultFontFace + "\" size=\"" + std::to_string(defaultFontSize) + "\">\n" + body + "</font>";
			}
			
			this->dynamicBookTexts[currentFormID] = textToStoreForBook; // Update the cache
			logger::info("BookMenuWatcher: Prepared and cached content for '{}'.", currentTitle);

		} else {
			this->dynamicBookTexts.erase(currentFormID);
			if (_lastOpenedDynamicBookTitle == currentTitle) {
				ClearLastOpenedBook();
			}
		}
	}	
    
    // This is the implementation for the public method declared in the header.
    // It's just a wrapper around the internal PrepareAndCacheBookContent.
    bool BookMenuWatcher::ReloadAndCacheBook(RE::TESObjectBOOK* bookToReload) {
        // We can add more logic here if needed, but for now, it just calls the main worker function.
        // The return value could be more robust, checking if content was actually cached.
        this->PrepareAndCacheBookContent(bookToReload);
        return this->dynamicBookTexts.contains(bookToReload->GetFormID());
    }

	std::optional<std::string> BookMenuWatcher::GetCachedHtmlForBook(RE::FormID bookFormID) {
		auto it = this->dynamicBookTexts.find(bookFormID);
		if (it != this->dynamicBookTexts.end()) {
			return it->second;
		}
		return std::nullopt;
	}
    
    // --- Implementation of tracker methods ---
    void BookMenuWatcher::SetLastOpenedBook(RE::FormID a_formID, const std::string& a_title) {
        _lastOpenedDynamicBookID = a_formID;
        _lastOpenedDynamicBookTitle = a_title;
    }

    RE::FormID BookMenuWatcher::GetLastOpenedDynamicBook() {
        return _lastOpenedDynamicBookID;
    }

    std::string BookMenuWatcher::GetLastOpenedDynamicBookTitle() {
        return _lastOpenedDynamicBookTitle;
    }

    void BookMenuWatcher::ClearLastOpenedBook() {
        _lastOpenedDynamicBookID = 0;
        _lastOpenedDynamicBookTitle.clear();
    }

} // namespace DynamicBookFramework