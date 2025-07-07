
#pragma once
#include "PCH.h"


namespace logger = SKSE::log;
namespace HtmlFormatText { // Or your preferred namespace if you have one for utilities
    std::string ApplyGeneralBookMarkup_ProcessChunk(
        const std::string& plainTextChunk,
        const std::string& defaultParagraphAlign = "justify" 
    );
    std::string ApplyGeneralBookMarkup(
        const std::string& plainText,
        const std::string& defaultFontFace = "$HandwrittenBold",
        int defaultFontSize = 20, // Common default size for books
        const std::string& defaultParagraphAlign = "justify" // Common alignment for books
    );
}

void SetupLog();
void LoadBookMappings();
std::optional<std::wstring> GetDynamicBookPathByTitle(const std::wstring& bookTitle);
std::wstring string_to_wstring(const std::string& str);
std::string wstring_to_utf8(const std::wstring& wstr);
std::string GetCurrentSaveIdentifier();
void OnGameLoad();
std::vector<std::string> GetAllBookTitles();
std::vector<std::string> SplitString(const std::string& str, char delimiter);
<<<<<<< Updated upstream
=======
std::string wstring_to_string(const std::wstring& wstr);
// Add this declaration alongside your other function declarations
std::vector<std::string> ExtractImagePathsFromText(const std::string& plainText);
const std::unordered_map<std::wstring, std::wstring>& GetAllBookMappings();

>>>>>>> Stashed changes
