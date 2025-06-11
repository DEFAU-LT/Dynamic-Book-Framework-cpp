//SessionDataManager.cpp
#include "SessionDataManager.h"
#include "Utility.h"
#include "BookMenuWatcher.h"
#include "PCH.h" // For common headers like SKSE, RE, and standard library


namespace { // Use an anonymous namespace to keep it local to this file
    std::string StripExtension(const std::string& filename) {
        // Find the last occurrence of .ess
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".ess") {
            return filename.substr(0, filename.length() - 4);
        }
        return filename;
    }
    std::string GetFormattedTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm buf;
        localtime_s(&buf, &in_time_t);
        std::stringstream ss;
        ss << std::put_time(&buf, "%Y-%m-%d_%H-%M-%S");
        return ss.str();
    }
    // Helper function to parse a value from our new key-value format
    std::string ParseValue(const std::string& metadata, const std::string& key) {
        // 1. Creates a search pattern. If key is "PARENT", this becomes "PARENT=\""
        std::string keyPattern = key + "=\"";

        // 2. Finds where that pattern starts in the line.
        size_t startPos = metadata.find(keyPattern);
        if (startPos == std::string::npos) return ""; // Not found

        // 3. Moves the starting position to be right after the pattern.
        startPos += keyPattern.length();

        // 4. Finds the next quotation mark, which marks the end of the value.
        size_t endPos = metadata.find('\"', startPos);
        if (endPos == std::string::npos) return ""; // Malformed

        // 5. Extracts and returns the substring between the start and end.
        return metadata.substr(startPos, endPos - startPos);
    }
}

namespace DynamicBookFramework {

    SessionDataManager* SessionDataManager::GetSingleton() {
        static SessionDataManager singleton;
        return &singleton;
    }

    void SessionDataManager::OnGameLoad(const std::string& saveIdentifier) {
        std::lock_guard<std::mutex> lock(_dataMutex);

        // Use the helper function to get a clean, extension-free identifier
        std::string cleanIdentifier = StripExtension(saveIdentifier);

        // We now use the cleanIdentifier for all logic
        std::string newCharacter = _getCharacterNameFromIdentifier(cleanIdentifier);
        std::string oldCharacter = _getCharacterNameFromIdentifier(_currentSaveIdentifier);

        if (!_currentSaveIdentifier.empty() && !newCharacter.empty() && newCharacter != oldCharacter) {
            _currentTimelineID = GetFormattedTimestamp();
        } else if (_currentTimelineID.empty()) {
            _currentTimelineID = GetFormattedTimestamp();
        }

        _currentSaveIdentifier = cleanIdentifier;
        _sessionParentSaveIdentifier = cleanIdentifier; 
        _sessionPendingEntries.clear();
    }

    void SessionDataManager::OnGameSave(const std::string& newSaveIdentifier) {
        std::lock_guard<std::mutex> lock(_dataMutex);

        std::string cleanNewIdentifier = StripExtension(newSaveIdentifier);
        std::string cleanParentIdentifier = StripExtension(_sessionParentSaveIdentifier);

        // --- Step 1: ALWAYS log the save event to the master history file ---
        auto historyPath = g_historyLogPath;
        std::ofstream historyFile(historyPath, std::ios::app);
        if (historyFile.is_open()) {
            historyFile << "ID=\"" << cleanNewIdentifier 
                        << "\" TIMELINE=\"" << _currentTimelineID 
                        << "\" PARENT=\"" << cleanParentIdentifier << "\"\n";
            historyFile.close();
        }

        // --- Step 2: If there are pending entries, write them to their specific book files ---
        if (!_sessionPendingEntries.empty()) {
            for (auto& [bookKey, entries] : _sessionPendingEntries) {
                if (entries.empty()) continue;

                auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(bookKey));
                if (!pathOpt) continue;
                
                std::ofstream out(*pathOpt, std::ios::app);
                if (out.is_open()) {
                    out << "\n;;SAVE_BLOCK ID=\"" << cleanNewIdentifier 
                        << "\" TIMELINE=\"" << _currentTimelineID 
                        << "\" PARENT=\"" << cleanParentIdentifier << "\";;\n";
                    
                    for (const auto& entry : entries) {
                        out << entry << "\n";
                    }

                    out << ";;END_SAVE_DATA;;\n";
                    out.close();
                }
            }
        }

        // --- Step 3: Update the internal state for the next session ---
        _sessionParentSaveIdentifier = cleanNewIdentifier;
        _currentSaveIdentifier = cleanNewIdentifier;
        _sessionPendingEntries.clear();
    }

    struct FileChunk {
        bool isDynamicBlock;   // True if this is a placeholder for a save entry
        std::string content;    // Holds static text OR the SaveID for a dynamic block
    };

    
    std::string SessionDataManager::GetFullContent(const std::string& fileKey) {
        std::lock_guard<std::mutex> lock(_dataMutex);

        if (_currentSaveIdentifier.empty()) return "";

        // --- PHASE 1: BUILD THE VALID HISTORY CHAIN FROM THE MASTER LOG ---
        std::set<std::string> validSaveIDs;
        auto historyPath = g_historyLogPath; // Using your global variable for the history log path

        if (std::filesystem::exists(historyPath)) {
            std::string parentTracer = StripExtension(_currentSaveIdentifier);
            
            // This loop now correctly traces the history using the master log file.
            while (!parentTracer.empty() && validSaveIDs.find(parentTracer) == validSaveIDs.end()) {
                validSaveIDs.insert(parentTracer);

                // <<< THE FIX IS HERE: This now correctly opens the history log file.
                std::ifstream historyFile(historyPath);
                std::string line;
                bool foundParentInLog = false;

                while (std::getline(historyFile, line)) {
                    if (line.find("ID=\"" + parentTracer + "\"") != std::string::npos) {
                        parentTracer = StripExtension(ParseValue(line, "PARENT"));
                        foundParentInLog = true;
                        break;
                    }
                }
                if (!foundParentInLog) break;
            }
        }

        // --- PHASE 2: PARSE THE SPECIFIC BOOK FILE'S LAYOUT ---
        auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(fileKey));
        if (!pathOpt || !std::filesystem::exists(*pathOpt)) {
            // ... (pending entry handling when file doesn't exist) ...
            return ""; // Simplified
        }

        std::vector<FileChunk> fileLayout;
        std::map<std::string, std::string> dynamicContentMap;
        std::ifstream bookFile(*pathOpt);
        std::string line;
        std::stringstream staticBuffer, dynamicBuffer;
        std::string currentBlockId;
        bool inDynamicBlock = false;

        while (std::getline(bookFile, line)) {
            if (line.rfind(";;SAVE_BLOCK ", 0) == 0) {
                if (staticBuffer.tellp() > 0) {
                    fileLayout.push_back({false, staticBuffer.str()});
                    staticBuffer.str("");
                }
                inDynamicBlock = true;
                currentBlockId = ParseValue(line, "ID");
                fileLayout.push_back({true, currentBlockId});
                dynamicBuffer.str("");
            } else if (line.rfind(";;END_SAVE_DATA;;", 0) == 0) {
                if (inDynamicBlock) {
                    dynamicContentMap[currentBlockId] = dynamicBuffer.str();
                }
                inDynamicBlock = false;
            } else {
                if (inDynamicBlock) dynamicBuffer << line << '\n';
                else staticBuffer << line << '\n';
            }
        }
        if (staticBuffer.tellp() > 0) {
            fileLayout.push_back({false, staticBuffer.str()});
        }

        // --- PHASE 3: ASSEMBLE THE FINAL CONTENT ---
        std::stringstream finalContent;
        for (const auto& chunk : fileLayout) {
            if (!chunk.isDynamicBlock) {
                finalContent << chunk.content;
            } else {
                if (validSaveIDs.count(chunk.content)) {
                    finalContent << dynamicContentMap[chunk.content];
                }
            }
        }

        // --- PHASE 4: APPEND PENDING ENTRIES ---
        if (_sessionPendingEntries.count(fileKey) && !_sessionPendingEntries.at(fileKey).empty()) {
            if (finalContent.tellp() > 0) finalContent << "\n";
            for (const auto& entry : _sessionPendingEntries.at(fileKey)) {
                finalContent << entry << "\n";
            }
        }
        
        return finalContent.str();
    }
    
    // This is the public API function that your addon calls
    void SessionDataManager::AppendEntry(const std::string& fileKey, const std::string& entryText) {
        if (_currentSaveIdentifier.empty()) {
            logger::warn("SessionDataManager::AppendEntry: No save identifier set. Buffering entry temporarily.");
        }
        std::lock_guard<std::mutex> lock(_dataMutex);
        _sessionPendingEntries[fileKey].push_back(entryText);
        logger::info("SessionDataManager::AppendEntry: Added entry to session buffer for key '{}'. Total pending for this key: {}", fileKey, _sessionPendingEntries[fileKey].size());
    }

    std::string SessionDataManager::_getCharacterNameFromIdentifier(const std::string& identifier) const {
        auto firstUnderscore = identifier.find('_');
        if (firstUnderscore == std::string::npos) {
            return "";
        }

        auto secondUnderscore = identifier.find('_', firstUnderscore + 1);
        if (secondUnderscore == std::string::npos) {
            return "";
        }

        return identifier.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
    }

    int SessionDataManager::_getSaveNumberFromIdentifier(const std::string& identifier) const {
        constexpr std::string_view PREFIX = "Save";
        if (identifier.rfind(PREFIX.data(), 0, PREFIX.length()) != 0) {
            return -1; // Does not start with "Save"
        }
        
        try {
            // Get the substring immediately after "Save"
            std::string numberPart = identifier.substr(PREFIX.length());
            // std::stoi will parse integers and stop at the first non-digit character (like '_')
            return std::stoi(numberPart);
        } catch (const std::invalid_argument& e) {
            logger::warn("Could not parse save number from identifier '{}': {}", identifier, e.what());
            return -1;
        } catch (const std::out_of_range& e) {
            logger::warn("Save number in identifier '{}' is out of range: {}", identifier, e.what());
            return -1;
        }
    }
    std::string SessionDataManager::ExtractTimelineID(const std::string& saveName) {
        size_t lastUnderscore = saveName.rfind('_');
        if (lastUnderscore == std::string::npos) return "";
        return saveName.substr(lastUnderscore + 1);  // assumes timeline ID is at the end
    }
} // namespace DynamicBookFramework
