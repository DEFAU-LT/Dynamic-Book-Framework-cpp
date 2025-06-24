#include "Utility.h"
#include "PCH.h"

namespace logger = SKSE::log;
std::unordered_map<std::wstring, std::wstring> g_dynamicBooks;
std::wstring g_iniPath = L"Data\\SKSE\\Plugins\\DynamicBookFramework.ini";


void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}

namespace HtmlFormatText {

    // // Helper function remains unchanged.
    // void FlushTextParagraphBuffer_Chunk(std::stringstream& resultHtml, std::string& paragraphContent, const std::string& paragraphAlign) {
    //     if (!paragraphContent.empty()) {
    //         if (!paragraphContent.empty() && paragraphContent.back() == '\n') {
    //             paragraphContent.pop_back();
    //         }
    //         resultHtml << "<p align='" << paragraphAlign << "'>" << paragraphContent << "</p>\n";
    //         paragraphContent.clear();
    //     }
    // }
    void FlushTextParagraphBuffer_Chunk(
        std::stringstream& resultHtml, 
        std::string& paragraphContent, 
        const std::string& paragraphAlign
    ) {
        if (paragraphContent.empty()) {
            return;
        }

        // Sanitize the content for display
        // First, trim leading/trailing newlines that the buffer might accumulate
        size_t start = paragraphContent.find_first_not_of("\n");
        if (start == std::string::npos) {
            paragraphContent.clear();
            return; // String was all newlines
        }
        size_t end = paragraphContent.find_last_not_of("\n");
        std::string contentToRender = paragraphContent.substr(start, end - start + 1);

        // Now, replace any internal newlines with HTML line breaks
        size_t pos = 0;
        while ((pos = contentToRender.find('\n', pos)) != std::string::npos) {
            contentToRender.replace(pos, 1, "<br>");
            pos += 4; // Move past the "<br>"
        }

        // Finally, wrap the sanitized content in a single <p> tag
        resultHtml << "<p align='" << paragraphAlign << "'>" << contentToRender << "</p>\n";
        
        paragraphContent.clear();
    }

    // This is the main processing function with the new trimming logic.
    // std::string ApplyGeneralBookMarkup_ProcessChunk(
    //     const std::string& plainTextChunk,
    //     const std::string& defaultParagraphAlign) {

    //     if (plainTextChunk.empty()) {
    //         return "";
    //     }

    //     std::stringstream resultChunkHtml;
    //     std::string currentTextParagraphContent;
    //     int consecutiveBlankLineCount = 0;
    //     bool currentlyInList = false;

    //     std::istringstream plainTextStream(plainTextChunk);
    //     std::string line;

    //     while (std::getline(plainTextStream, line)) {
    //         // --- Standard trim logic ---
    //         size_t start = line.find_first_not_of(" \t\r\n");
    //         std::string trimmedLine = (start == std::string::npos) ? "" : line.substr(start);
    //         size_t end = trimmedLine.find_last_not_of(" \t\r\n");
    //         trimmedLine = (end == std::string::npos) ? "" : trimmedLine.substr(0, end + 1);

    //         // --- NEW, ROBUST LOGIC STRUCTURE ---
    //         bool isListItem = !trimmedLine.empty() && trimmedLine[0] == '*';

    //         if (isListItem) {
    //             // --- HANDLE LIST ITEM ---

    //             // If we are switching from non-list to list, close any open paragraph and start the <ul> tag.
    //             if (!currentlyInList) {
    //                 FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
    //                 resultChunkHtml << "<ul>\n";
    //                 currentlyInList = true;
    //             }

    //             // Get the list item content, trimming the '*' and any following spaces.
    //             size_t contentStart = trimmedLine.find_first_not_of(" \t", 1);
    //             std::string listItemContent = (contentStart == std::string::npos) ? "" : trimmedLine.substr(contentStart);
                
    //             resultChunkHtml << "<li>" << listItemContent << "</li>\n";
    //             consecutiveBlankLineCount = 0;

    //         } else {
    //             // --- HANDLE ALL NON-LIST-ITEM LINES (Text, Blanks, Images, etc.) ---

    //             // If we are switching from list to non-list, close the <ul> tag.
    //             if (currentlyInList) {
    //                 resultChunkHtml << "</ul>\n";
    //                 currentlyInList = false;
    //             }

    //             // Now, handle the specific type of non-list-item line
    //             if (trimmedLine.rfind("[IMG=", 0) == 0 && trimmedLine.back() == ']') {
    //                 FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
    //                 // ... (Image logic is unchanged)
    //                 size_t contentStart = 5;
    //                 size_t contentLen = trimmedLine.length() - contentStart - 1;
    //                 std::string tagContent = trimmedLine.substr(contentStart, contentLen);
    //                 std::stringstream tagStream(tagContent);
    //                 std::string imagePath, widthStr, heightStr;
    //                 std::getline(tagStream, imagePath, '|');
    //                 std::getline(tagStream, widthStr, '|');
    //                 std::getline(tagStream, heightStr, '|');
    //                 if (widthStr.empty()) widthStr = "290";
    //                 if (heightStr.empty()) heightStr = "389";
    //                 std::replace(imagePath.begin(), imagePath.end(), '/', '\\');
    //                 resultChunkHtml << "<p align='center'><img src='img://" << imagePath << "' width='" << widthStr << "' height='" << heightStr << "'></p>\n";
    //                 consecutiveBlankLineCount = 0;

    //             } else if (trimmedLine.rfind("[pagebreak]", 0) == 0) {
    //                 FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
    //                 resultChunkHtml << "[Pagebreak]\n";
    //                 consecutiveBlankLineCount = 0;

    //             } else if (trimmedLine.empty()) {
    //                 // This is a blank line. Flush any text and increment the counter.
    //                 FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
    //                 consecutiveBlankLineCount++;

    //             } else {
    //                 // This is a normal line of text.
    //                 // --- FIXED BLANK LINE LOGIC ---
    //                 // First, check if the preceding blank lines should trigger a pagebreak.
    //                 if (consecutiveBlankLineCount >= 2) {
    //                     resultChunkHtml << "[Pagebreak]\n";
    //                 }
    //                 // Reset counter now that we have text.
    //                 consecutiveBlankLineCount = 0;

    //                 // Append text to the current paragraph buffer.
    //                 if (!currentTextParagraphContent.empty()) {
    //                     currentTextParagraphContent += "\n";
    //                 }
    //                 currentTextParagraphContent += trimmedLine;
    //             }
    //         }
    //     }

    //     // --- End of file cleanup ---
    //     if (currentlyInList) {
    //         resultChunkHtml << "</ul>\n";
    //     }
    //     //FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);

    //     return resultChunkHtml.str();
    // }

    std::string HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(
        const std::string& plainTextChunk,
        const std::string& defaultParagraphAlign) {

        if (plainTextChunk.empty()) {
            return "";
        }
        
        // --- STEP 1: Sanitize all line endings ---
        std::string sanitizedText = plainTextChunk;
        // Replace all Windows-style \r\n with Unix-style \n
        size_t pos = 0;
        while ((pos = sanitizedText.find("\r\n", pos)) != std::string::npos) {
            sanitizedText.replace(pos, 2, "\n");
        }
        // Replace all remaining Mac-style \r with Unix-style \n
        pos = 0;
        while ((pos = sanitizedText.find('\r', pos)) != std::string::npos) {
            sanitizedText.replace(pos, 1, "\n");
        }

        // --- STEP 2: Manually split the sanitized text into lines ---
        std::vector<std::string> lines = SplitString(sanitizedText, '\n');

        // --- STEP 3: Process the guaranteed-clean lines ---
        std::stringstream resultChunkHtml;
        std::string currentTextParagraphContent;
        int consecutiveBlankLineCount = 0;
        bool currentlyInList = false;

        for (const auto& line : lines) {
            // We no longer need to trim, as SplitString and sanitization handled it.
            std::string trimmedLine = line;

            bool isListItem = !trimmedLine.empty() && trimmedLine[0] == '*';

            if (isListItem) {
                if (!currentlyInList) {
                    FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                    resultChunkHtml << "<ul>\n";
                    currentlyInList = true;
                }
                size_t contentStart = trimmedLine.find_first_not_of(" \t", 1);
                std::string listItemContent = (contentStart == std::string::npos) ? "" : trimmedLine.substr(contentStart);
                resultChunkHtml << "<li>" << listItemContent << "</li>\n";
                consecutiveBlankLineCount = 0;
            } else {
                if (currentlyInList) {
                    resultChunkHtml << "</ul>\n";
                    currentlyInList = false;
                }

                if (trimmedLine.empty()) {
                    FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                    consecutiveBlankLineCount++;
                } else {
                    if (consecutiveBlankLineCount >= 2) {
                        resultChunkHtml << "[pagebreak]\n";
                    }
                    consecutiveBlankLineCount = 0;
                    
                    if (!currentTextParagraphContent.empty()) {
                        currentTextParagraphContent += "\n";
                    }
                    currentTextParagraphContent += trimmedLine;
                }
            }
        }

        // --- Final cleanup ---
        if (currentlyInList) {
            resultChunkHtml << "</ul>\n";
        }
        FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);

        return resultChunkHtml.str();
    }
} // end namespace HtmlFormatText

// Helper function for string to wstring conversion (UTF-8)
std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    if (size_needed == 0) {
        // Handle error if needed
        return std::wstring();
    }
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed == 0) {
        // Handle error if needed
        return std::string();
    }
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}

// Reads all book mappings from the INI file into g_dynamicBooks
void LoadBookMappings()
{
    g_dynamicBooks.clear(); 

    // 1. Set the top-level folder you want to search.
    std::filesystem::path searchPath = "Data/SKSE/Plugins/DynamicBookFramework";

    if (!std::filesystem::exists(searchPath) || !std::filesystem::is_directory(searchPath)) {
        logger::warn("Search path folder not found at '{}'. No dynamic books will be loaded.", searchPath.string());
        return;
    }

    logger::info("Scanning for book mapping files in '{}' and all subfolders...", searchPath.string());

    // 2. Use the 'recursive' iterator to look inside subfolders.
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ini") {
            
            const std::wstring& iniPath = entry.path().wstring();
            logger::info("  -> Loading mappings from '{}'", wstring_to_utf8(iniPath));

            constexpr size_t bufferSize = 4096;
            wchar_t keysBuffer[bufferSize] = { 0 };
            GetPrivateProfileStringW(L"Books", nullptr, nullptr, keysBuffer, bufferSize, iniPath.c_str());

            // This path for the actual .txt files remains the same.
            std::filesystem::path booksFolder = "Data/SKSE/Plugins/DynamicBookFramework/books";

            wchar_t* currentKey = keysBuffer;
            while (*currentKey) {
                wchar_t valueBuffer[bufferSize] = { 0 };
                GetPrivateProfileStringW(L"Books", currentKey, nullptr, valueBuffer, bufferSize, iniPath.c_str());

                if (*valueBuffer) {
                    std::wstring key(currentKey);
                    std::filesystem::path fullTxtPath = booksFolder / valueBuffer;

                    g_dynamicBooks[key] = fullTxtPath.wstring();
                }
                currentKey += wcslen(currentKey) + 1;
            }
        }
    }
    
    // Logging results (no change here)
    if (!g_dynamicBooks.empty()) {
        std::stringstream titles_ss;
        bool first_title = true;
        for (const auto& pair : g_dynamicBooks) {
            if (!first_title) {
                titles_ss << ", ";
            }
            titles_ss << "'" << wstring_to_utf8(pair.first) << "'";
            first_title = false;
        }
        logger::info("Finished loading. Total dynamic books found: {}. Titles: {}", g_dynamicBooks.size(), titles_ss.str());
    } else {
        logger::info("Finished loading. No dynamic book mappings were found.");
    }
}
// Returns the path if the title matches one of our dynamic books
std::optional<std::wstring> GetDynamicBookPathByTitle(const std::wstring& bookTitle)
{
    auto it = g_dynamicBooks.find(bookTitle);
    if (it != g_dynamicBooks.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> GetAllBookTitles() {
    std::vector<std::string> titles;
    // Iterate through the global map and extract the keys (the book titles).
    for (auto const& [title_w, path_w] : g_dynamicBooks) {
        titles.push_back(wstring_to_utf8(title_w)); // Convert wstring title to string
    }
    return titles;
}
std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}