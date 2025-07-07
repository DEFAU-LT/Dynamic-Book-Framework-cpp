#include "BookMenuWatcher.h"
#include "FileWatcher.h"
#include "SessionDataManager.h"
#include "Utility.h"
#include "BookUIManager.h"
#include "Settings.h"
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
<<<<<<< Updated upstream
		
		std::wstring wCurrentTitle = string_to_wstring(currentTitle);
		auto dynamicBookPathOpt = GetDynamicBookPathByTitle(wCurrentTitle);
        
=======

		Settings::g_bookmarks.erase(currentTitle);
		std::string rawText = SessionDataManager::GetSingleton()->GetFullContent(currentTitle);
		auto foundTags = Settings::ParseTagsFromText(rawText);

		if (!foundTags.empty()) {
			Settings::g_bookmarks[currentTitle] = foundTags;
			logger::info("Found and registered {} bookmark tags for {}.", foundTags.size(), currentTitle);
		}

		auto dynamicBookPathOpt = GetDynamicBookPathByTitle(string_to_wstring(currentTitle));

>>>>>>> Stashed changes
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
                std::string defaultFontFace = Settings::defaultFontFace; 
    			int defaultFontSize = Settings::defaultFontSize;
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

<<<<<<< Updated upstream
	std::optional<std::string> BookMenuWatcher::GetCachedHtmlForBook(RE::FormID bookFormID) {
		auto it = this->dynamicBookTexts.find(bookFormID);
		if (it != this->dynamicBookTexts.end()) {
=======
	bool BookMenuWatcher::ReloadAndCacheBook(RE::TESObjectBOOK* bookToReload) {
		this->PrepareAndCacheBookContent(bookToReload);
		return _dynamicBookTexts.contains(bookToReload->GetFormID());
	}

	std::string BookMenuWatcher::GetFullDynamicTextForBook(RE::TESObjectBOOK* book) {
		if (!book) return "";

		std::string bookTitle = book->GetFullName() ? book->GetFullName() : "";
		std::string fileContent = SessionDataManager::GetSingleton()->GetFullContent(bookTitle);

		if (fileContent.empty()) return "";

		std::string finalText;
		if (fileContent.rfind(";;RAW_HTML;;", 0) == 0) {
			size_t markerLineEnd = fileContent.find('\n');
			finalText = (markerLineEnd != std::string::npos) ? fileContent.substr(markerLineEnd + 1) : "";
		} else {
			std::string body = HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(fileContent);
			size_t end = body.find_last_not_of(" \t\n\r");
			if (end != std::string::npos) {
				body = body.substr(0, end + 1);
			} else {
				body.clear(); // Body was all whitespace, so clear it.
			}
			std::string defaultFontFace = Settings::defaultFontFace;
			int defaultFontSize = Settings::defaultFontSize;
			finalText = "<font face=\"" + defaultFontFace + "\" size=\"" + std::to_string(defaultFontSize) + "\">\n" + body + "</font>";
		}
		return finalText;
	}

	// --- Vanilla Text Caching Implementation ---
	void BookMenuWatcher::CacheVanillaText(const std::string& bookTitle, const std::string& text) {
		if (!bookTitle.empty()) {
			_vanillaBookTexts[bookTitle] = text;
		}
	}

	std::optional<std::string> BookMenuWatcher::GetCachedVanillaText(const std::string& bookTitle) {
		auto it = _vanillaBookTexts.find(bookTitle);
		if (it != _vanillaBookTexts.end()) {
>>>>>>> Stashed changes
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