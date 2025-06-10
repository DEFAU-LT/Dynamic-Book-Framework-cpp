
#pragma once
#include "PCH.h"

namespace DynamicBookFramework {

	class BookMenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
	public:
		static BookMenuWatcher* GetSingleton();
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// --- Public methods for other components ---
		bool ReloadAndCacheBook(RE::TESObjectBOOK* bookToReload);
		std::optional<std::string> GetCachedHtmlForBook(RE::FormID bookFormID);

		void SetLastOpenedBook(RE::FormID a_formID, const std::string& a_title);
		RE::FormID GetLastOpenedDynamicBook();
		std::string GetLastOpenedDynamicBookTitle();
		void ClearLastOpenedBook();

		std::map<RE::FormID, std::string> dynamicBookTexts;

	private:
		BookMenuWatcher() = default;
		~BookMenuWatcher() override = default;
		BookMenuWatcher(const BookMenuWatcher&) = delete;
		BookMenuWatcher(BookMenuWatcher&&) = delete;
		BookMenuWatcher& operator=(const BookMenuWatcher&) = delete;
		BookMenuWatcher& operator=(BookMenuWatcher&&) = delete;
        
        // --- FIX: Added missing declaration for the helper function ---
        void PrepareAndCacheBookContent(RE::TESObjectBOOK* bookToPrepare);
        
        // --- Private Members ---
        RE::FormID _lastOpenedDynamicBookID{ 0 };
        std::string _lastOpenedDynamicBookTitle;
	};

} // namespace DynamicBookFramework
