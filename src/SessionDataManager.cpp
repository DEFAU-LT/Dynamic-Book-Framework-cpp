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
            _currentTimelineID = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        } else if (_currentTimelineID.empty()) {
            _currentTimelineID = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        }

        _currentSaveIdentifier = cleanIdentifier;
        _sessionParentSaveIdentifier = cleanIdentifier; 
        _sessionPendingEntries.clear();
    }

    void SessionDataManager::OnGameSave(const std::string& newSaveIdentifier) {
        std::lock_guard<std::mutex> lock(_dataMutex);

        if (_sessionPendingEntries.empty()) {
            return;
        }

        // Also strip the extension from the new save name to be 100% consistent
        std::string cleanNewIdentifier = StripExtension(newSaveIdentifier);
        // And ensure the parent identifier is also clean
        std::string cleanParentIdentifier = StripExtension(_sessionParentSaveIdentifier);

        for (auto& [bookKey, entries] : _sessionPendingEntries) {
            if (entries.empty()) continue;

            auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(bookKey));
            if (!pathOpt) continue;
            
            std::ofstream out(*pathOpt, std::ios::app);
            if (!out.is_open()) continue;

            out << "\n\n;;BEGIN_SAVE_DATA "
                << cleanNewIdentifier << ";"      // Use the clean name
                << _currentTimelineID << ";"
                << cleanParentIdentifier << ";;\n"; // Use the clean parent name

            for (size_t i = 0; i < entries.size(); ++i) {
                out << entries[i];
                if (i < entries.size() - 1) out << "\n";
            }
            out << "\n;;END_SAVE_DATA;;\n";
            out.close();
        }

        _sessionParentSaveIdentifier = cleanNewIdentifier;
        _currentSaveIdentifier = cleanNewIdentifier;
        _sessionPendingEntries.clear();
    }

    struct FileChunk {
        bool isDynamicBlock;   // True if this is a placeholder for a save entry
        std::string content;    // Holds static text OR the SaveID for a dynamic block
    };

    // And you can keep this struct for holding parsed block data.
    struct SaveBlock {
        std::string id;
        std::string timelineID;
        std::string parentID;
        std::string content;
    };

    // --- This is the final, corrected GetFullContent function ---
    std::string SessionDataManager::GetFullContent(const std::string& fileKey) {
        std::lock_guard<std::mutex> lock(_dataMutex);

        if (_currentSaveIdentifier.empty()) return "";

        auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(fileKey));
        if (!pathOpt || !std::filesystem::exists(*pathOpt)) {
            // Handle case where file doesn't exist but there are pending entries
            if (_sessionPendingEntries.count(fileKey) && !_sessionPendingEntries.at(fileKey).empty()) {
                std::string pendingText;
                for(const auto& entry : _sessionPendingEntries.at(fileKey)) {
                    pendingText += entry + "\n";
                }
                return pendingText;
            }
            return "";
        }

        // --- PHASE 1: A SINGLE, SMART PARSING PASS ---
        // We will build both the file layout and a map of all save blocks in one go.
        std::vector<FileChunk> fileLayout;
        std::map<std::string, SaveBlock> allBlocks;

        std::ifstream file(*pathOpt);
        std::string line;
        std::stringstream staticBuffer;
        std::stringstream dynamicBuffer;
        std::string currentBlockId;
        bool inDynamicBlock = false;

        // This loop now populates BOTH the layout and the map of all save blocks.
        while (std::getline(file, line)) {
            // NOTE: This code assumes the original ";;BEGIN_SAVE_DATA..." format.
            // If you use the human-readable ";;SAVE_BLOCK ID=..." format, the parsing here needs to be updated.
            if (line.rfind(";;BEGIN_SAVE_DATA ", 0) == 0) {
                if (staticBuffer.tellp() > 0) {
                    fileLayout.push_back({false, staticBuffer.str()});
                    staticBuffer.str("");
                }
                
                inDynamicBlock = true;
                std::string metadata = line.substr(18, line.length() - 20);
                std::stringstream ss(metadata);
                std::string id, timeline, parent;
                std::getline(ss, id, ';');
                std::getline(ss, timeline, ';');
                std::getline(ss, parent, ';');
                
                currentBlockId = id;
                allBlocks[currentBlockId] = {id, timeline, parent, ""}; // Add block with empty content
                fileLayout.push_back({true, currentBlockId}); // Add placeholder to layout
                dynamicBuffer.str("");

            } else if (line.rfind(";;END_SAVE_DATA;;", 0) == 0) {
                if (inDynamicBlock && allBlocks.count(currentBlockId)) {
                    allBlocks[currentBlockId].content = dynamicBuffer.str(); // Set the content for the block
                }
                inDynamicBlock = false;
                currentBlockId = "";

            } else {
                if (inDynamicBlock) {
                    dynamicBuffer << line << '\n';
                } else {
                    staticBuffer << line << '\n';
                }
            }
        }
        if (staticBuffer.tellp() > 0) {
            fileLayout.push_back({false, staticBuffer.str()});
        }

        // --- PHASE 2: BUILD THE VALID HISTORY CHAIN (The Correct Way) ---
        // This logic works from the in-memory map and is efficient and correct.
        std::set<std::string> validSaveIDs;
        std::string currentIdInChain = StripExtension(_currentSaveIdentifier);

        while (!currentIdInChain.empty() && allBlocks.count(currentIdInChain)) {
            validSaveIDs.insert(currentIdInChain);
            const auto& block = allBlocks.at(currentIdInChain);
            if (block.parentID.empty() || block.parentID == currentIdInChain) {
                break; // Reached the start of the chain
            }
            currentIdInChain = StripExtension(block.parentID);
        }

        // --- PHASE 3: ASSEMBLE THE FINAL CONTENT ---
        std::stringstream finalContent;
        for (const auto& chunk : fileLayout) {
            if (!chunk.isDynamicBlock) {
                // It's static text, always include it.
                finalContent << chunk.content;
            } else {
                // It's a dynamic block. Check if its SaveID is in our valid history.
                if (validSaveIDs.count(chunk.content)) {
                    finalContent << allBlocks.at(chunk.content).content;
                }
            }
        }

        // --- PHASE 4: APPEND PENDING (UNSAVED) ENTRIES ---
        if (_sessionPendingEntries.count(fileKey) && !_sessionPendingEntries.at(fileKey).empty()) {
            if (finalContent.tellp() > 0) {
                // Add a separator only if there's existing content
                std::string currentContent = finalContent.str();
                if (currentContent.length() < 2 || currentContent.substr(currentContent.length() - 2) != "\n\n") {
                    finalContent << "\n";
                }
            }
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
