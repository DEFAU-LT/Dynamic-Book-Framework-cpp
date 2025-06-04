#include "BookMenuWatcher.h"
#include "Utility.h"


std::map<RE::FormID, std::string> BookMenuWatcher::dynamicBookTexts;

RE::BSEventNotifyControl BookMenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
    if (!a_event || !a_event->opening || a_event->menuName != RE::BookMenu::MENU_NAME) {
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::TESObjectBOOK* currentBookObject = RE::BookMenu::GetTargetForm();
    // ... (null checks for currentBookObject and title as before) ...
    const char* bookTitleCStr = currentBookObject->GetFullName();
    if (!bookTitleCStr || *bookTitleCStr == '\0') { /* log & return */ return RE::BSEventNotifyControl::kContinue;}
    std::string currentTitle(bookTitleCStr);
    RE::FormID currentFormID = currentBookObject->GetFormID();
    logger::info("BookMenuWatcher: BookMenu target form: '{}' (FormID: {:X})", currentTitle.c_str(), currentFormID);


    std::wstring wCurrentTitle = string_to_wstring(currentTitle);
    if (auto dynamicBookPathOpt = GetDynamicBookPathByTitle(wCurrentTitle)) {
        const std::wstring& dynamicBookPath_w = *dynamicBookPathOpt;
        std::string dynamicBookPath_utf8 = wstring_to_utf8(dynamicBookPath_w);
        
        std::ifstream file(dynamicBookPath_w);
        std::string finalHtmlBodyContent;    // Content that goes BETWEEN <font> tags
        std::string currentPlainTextChunk;
        bool inRawHtmlBlock = false;

        const std::string rawHtmlBeginMarker = ";;BEGIN_RAW_HTML;;";
        const std::string rawHtmlEndMarker = ";;END_RAW_HTML;;";

        if (file.is_open()) {
            logger::info("BookMenuWatcher: Processing dynamic book file: {}", dynamicBookPath_utf8.c_str());
            std::string line;
            while (std::getline(file, line)) {
                if (line.rfind(rawHtmlBeginMarker, 0) == 0) {
                    if (!currentPlainTextChunk.empty()) {
                        finalHtmlBodyContent += HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(currentPlainTextChunk);
                        currentPlainTextChunk.clear();
                    }
                    inRawHtmlBlock = true;
                    logger::trace("BookMenuWatcher: Entering RAW HTML block.");
                    continue; 
                } else if (line.rfind(rawHtmlEndMarker, 0) == 0) {
                    // If there was any raw content, it's already added.
                    // We just switch mode. Any plain text after this will start a new chunk.
                    inRawHtmlBlock = false;
                    logger::trace("BookMenuWatcher: Exiting RAW HTML block.");
                    continue; 
                }

                if (inRawHtmlBlock) {
                    finalHtmlBodyContent += line + "\n"; 
                } else {
                    currentPlainTextChunk += line + "\n"; 
                }
            }
            file.close();

            // After loop, process any remaining plain text chunk
            if (!currentPlainTextChunk.empty()) {
                finalHtmlBodyContent += HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(currentPlainTextChunk);
            }
            
            // Wrap the entire assembled body with the main font tags
            std::string defaultFontFace = "$HandwrittenFont"; // Or get from config
            int defaultFontSize = 20;                   // Or get from config
            
            std::string textToStoreForBook = 
                "<font face='" + defaultFontFace + "' size='" + std::to_string(defaultFontSize) + "'><b>\n" +
                finalHtmlBodyContent + 
                "</font>";
            
            GetSingleton()->dynamicBookTexts[currentFormID] = textToStoreForBook;
            logger::info("BookMenuWatcher: Stored final assembled text for FormID {:X}. Length: {}", currentFormID, textToStoreForBook.length());
            logger::trace("BookMenuWatcher: Final text snippet for {:X}: {:.1500}", currentFormID, textToStoreForBook);

        } else {
            logger::error("BookMenuWatcher: Failed to open dynamic book file: {}", dynamicBookPath_utf8.c_str());
            // Create a default error page, also wrapped in font tags
            std::string errorContent = HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(
                "; Error: Dynamic Journal Framework\n; Failed to read file:\n; " + dynamicBookPath_utf8
            );
            GetSingleton()->dynamicBookTexts[currentFormID] = 
                "<font face='$PrintedFont' size='20'>\n" + errorContent + "</font>";
        }
    } else {
        // ... (not a dynamic book, erase from map) ...
        GetSingleton()->dynamicBookTexts.erase(currentFormID);
    }
    return RE::BSEventNotifyControl::kContinue;
}