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


namespace HtmlFormatText { // Or your preferred namespace

    // Helper function to flush the accumulated paragraph content
    void FlushTextParagraphBuffer_Chunk(std::stringstream& resultHtml, std::string& paragraphContent, const std::string& paragraphAlign) {
        if (!paragraphContent.empty()) {
            if (!paragraphContent.empty() && paragraphContent.back() == '\n') {
                paragraphContent.pop_back();
            }
            resultHtml << "<p align='" << paragraphAlign << "'>" << paragraphContent << "</p>\n";
            paragraphContent.clear();
        }
    }

    // Helper to check if a line consists only of whitespace
    bool IsLineEffectivelyBlank_Chunk(const std::string& line) {
        return line.find_first_not_of(" \t\r\n") == std::string::npos;
    }

    // Helper to check for the image line marker.
    bool IsAnImageLine_Chunk(const std::string& s_line) {
        std::string trimmed_line = s_line;
        size_t first_char = trimmed_line.find_first_not_of(" \t\r\n");
        if (first_char == std::string::npos) { return false; }
        trimmed_line = trimmed_line.substr(first_char);
        if (trimmed_line.length() < 4) return false;
        std::string prefix = trimmed_line.substr(0, 4);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), 
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (prefix == "<img") {
            std::string lower_trimmed_line = trimmed_line;
            std::transform(lower_trimmed_line.begin(), lower_trimmed_line.end(), lower_trimmed_line.begin(), 
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            return lower_trimmed_line.find("src=") != std::string::npos;
        }
        return false;
    }

    // PROCESSES A CHUNK OF PLAIN TEXT - DOES NOT ADD GLOBAL <FONT> TAGS
    std::string ApplyGeneralBookMarkup_ProcessChunk(
        const std::string& plainTextChunk,
        const std::string& defaultParagraphAlign ) { // Font face/size handled globally

        if (plainTextChunk.empty()) {
            return ""; // Or "<p> </p>\n" if an empty chunk should still produce a paragraph
        }

        std::stringstream resultChunkHtml;
        std::string currentTextParagraphContent; 
        int consecutiveBlankLineCount = 0;

        std::istringstream plainTextStream(plainTextChunk);
        std::string line;

        while (std::getline(plainTextStream, line)) {
            if (IsAnImageLine_Chunk(line)) { 
                FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                resultChunkHtml << line << "\n"; // Output image line as-is (it's part of this "plain text" chunk)
                consecutiveBlankLineCount = 0; 
            } else { 
                if (IsLineEffectivelyBlank_Chunk(line)) { 
                    if (!currentTextParagraphContent.empty()) { 
                        FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                    }
                    consecutiveBlankLineCount++;
                } else { 
                    if (consecutiveBlankLineCount >= 2) { 
                        FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
                        resultChunkHtml << "[pagebreak]\n";
                    }
                    consecutiveBlankLineCount = 0; 

                    if (!currentTextParagraphContent.empty()) {
                        currentTextParagraphContent += "\n"; 
                    }
                    currentTextParagraphContent += line; 
                }
            }
        }
        FlushTextParagraphBuffer_Chunk(resultChunkHtml, currentTextParagraphContent, defaultParagraphAlign);
        
        return resultChunkHtml.str();
    }

}

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
