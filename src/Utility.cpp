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

    // Helper function remains unchanged.
    void FlushTextParagraphBuffer_Chunk(std::stringstream& resultHtml, std::string& paragraphContent, const std::string& paragraphAlign) {
        if (!paragraphContent.empty()) {
            if (!paragraphContent.empty() && paragraphContent.back() == '\n') {
                paragraphContent.pop_back();
            }
            resultHtml << "<p align='" << paragraphAlign << "'>" << paragraphContent << "</p>\n";
            paragraphContent.clear();
        }
    }

    // This is the main processing function with the new trimming logic.
    std::string ApplyGeneralBookMarkup_ProcessChunk(
        const std::string& plainTextChunk,
        const std::string& defaultParagraphAlign) {

        if (plainTextChunk.empty()) {
            return "";
        }

        std::stringstream resultChunkHtml;
        std::string currentTextParagraphContent;
        int consecutiveBlankLineCount = 0;

        std::istringstream plainTextStream(plainTextChunk);
        std::string line;

        while (std::getline(plainTextStream, line)) {
            // --- NEW: Trim whitespace from the start and end of the line ---
            size_t start = line.find_first_not_of(" \t\r\n");
            std::string trimmedLine = (start == std::string::npos) ? "" : line.substr(start);
            size_t end = trimmedLine.find_last_not_of(" \t\r\n");
            trimmedLine = (end == std::string::npos) ? "" : trimmedLine.substr(0, end + 1);
            // --- All subsequent checks will use 'trimmedLine' ---

            // PRIORITY 1: Check for our custom [IMG=...] tag on the trimmed line
            if (trimmedLine.rfind("[IMG=", 0) == 0 && trimmedLine.back() == ']') {
                FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);

                size_t contentStart = 5; // Length of "[IMG="
                size_t contentLen = trimmedLine.length() - contentStart - 1;
                std::string tagContent = trimmedLine.substr(contentStart, contentLen);

                std::stringstream tagStream(tagContent);
                std::string imagePath, widthStr, heightStr;

                std::getline(tagStream, imagePath, '|');
                std::getline(tagStream, widthStr, '|');
                std::getline(tagStream, heightStr, '|');
                
                if (widthStr.empty()) widthStr = "290"; // Defaulting to your example's dimensions
                if (heightStr.empty()) heightStr = "389";

                std::replace(imagePath.begin(), imagePath.end(), '/', '\\');

                resultChunkHtml << "<p align='center'><img src='img://" << imagePath << "' width='" << widthStr << "' height='" << heightStr << "'></p>\n";
                
                consecutiveBlankLineCount = 0;
                continue;
            }

            // PRIORITY 2: Check for the explicit [pagebreak] marker
            if (trimmedLine.rfind("[pagebreak]", 0) == 0) {
                FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                resultChunkHtml << "<p>&nbsp;</p>\n";
                consecutiveBlankLineCount = 0;
                continue;
            }

            // PRIORITY 3: Handle normal text and blank lines
            if (trimmedLine.empty()) {
                if (!currentTextParagraphContent.empty()) {
                    FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                }
                consecutiveBlankLineCount++;
            } else { 
                if (consecutiveBlankLineCount >= 2 && !resultChunkHtml.str().empty()) {
                    resultChunkHtml << "<p>&nbsp;</p>\n";
                }
                consecutiveBlankLineCount = 0;

                if (!currentTextParagraphContent.empty()) {
                    currentTextParagraphContent += "\n"; 
                }
                currentTextParagraphContent += trimmedLine; // Use the trimmed line
            }
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
void LoadBookMappings(const std::wstring& iniPath)
{
    g_dynamicBooks.clear(); // Clear existing mappings

    constexpr size_t bufferSize = 4096;
    wchar_t keysBuffer[bufferSize] = { 0 };
    GetPrivateProfileStringW(L"Books", nullptr, nullptr, keysBuffer, bufferSize, iniPath.c_str());

    // Derive base path to the `books/` folder (relative to the INI)
    std::filesystem::path iniDir = std::filesystem::path(iniPath).parent_path();
    std::filesystem::path booksFolder = iniDir / L"books";

    wchar_t* currentKey = keysBuffer;
    while (*currentKey)
    {
        wchar_t valueBuffer[bufferSize] = { 0 };
        GetPrivateProfileStringW(L"Books", currentKey, nullptr, valueBuffer, bufferSize, iniPath.c_str());

        if (*valueBuffer)
        {
            std::wstring key(currentKey);
            std::filesystem::path fullTxtPath = booksFolder / valueBuffer;

            // Store in your map
            g_dynamicBooks[key] = fullTxtPath.wstring();
        }

        currentKey += wcslen(currentKey) + 1;
    }
    //logger::info("Loaded {} dynamic book mappings.", g_dynamicBooks.size()); 
    if (!g_dynamicBooks.empty()) {
        std::stringstream titles_ss; // Use a stringstream to build the list
        bool first_title = true;

        for (const auto& pair : g_dynamicBooks) {
            // pair.first is the std::wstring title
            std::string title_utf8 = wstring_to_utf8(pair.first); // Your existing conversion function

            if (!first_title) {
                titles_ss << ", "; // Add a comma and space before subsequent titles
            }
            titles_ss << "'" << title_utf8 << "'"; // Add the title in single quotes

            first_title = false; // Set flag to false after processing the first title
        }
        // Log the compiled string of titles
        logger::info("Dynamic Book Titles Loaded: {}", titles_ss.str());
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

