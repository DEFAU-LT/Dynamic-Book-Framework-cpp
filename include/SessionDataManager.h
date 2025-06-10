//SessionDataManager.h
#pragma once
#include "PCH.h"


namespace DynamicBookFramework {

    //Manages text data that is buffered per session and committed to disk only on game save.
    //This implementation uses metadata blocks within single files to manage save-specific content.
    class SessionDataManager {
    public:
        static SessionDataManager* GetSingleton();

        
        //@brief Writes all pending entries for all tracked files to disk. Called on game save.
        void OnGameSave(const std::string& saveIdentifier);

        /**
         * @brief Clears pending buffers and sets the new save identifier. Called on kPostLoadGame or kNewGame.
         * @param saveIdentifier A unique string identifying the current character/save (e.g., the save name).
         */
        void OnGameLoad(const std::string& saveIdentifier);


        /**
         * @brief Adds a new entry to the in-memory buffer for a given file key.
         * @param fileKey A unique key for the file (this is typically the book's title, which maps to a .txt file).
         * @param entryText The new block of text to append.
         */
        void AppendEntry(const std::string& fileKey, const std::string& entryText);

        /**
         * @brief Gets the full content for a given file for the CURRENT save.
         * It combines what's on disk (from the correct metadata block) with what's pending in the session buffer.
         * This is what BookMenuWatcher should call to get the most up-to-date content for display.
         * @param fileKey The book title/key to look up.
         * @return The complete, ready-to-process plain text content.
         */
        std::string GetFullContent(const std::string& fileKey);

        std::string GetCurrentSaveIdentifier();
        void OnGameLoad();

        /**
         * @brief Extracts the character name from a save identifier string.
         * @param identifier The save identifier, expected format "prefix_CharacterName_suffix".
         * @return The extracted character name, or an empty string if the format is invalid.
         */
        std::string _getCharacterNameFromIdentifier(const std::string& identifier) const;

        /**
         * @brief Extracts the save number from a save identifier string.
         * @param identifier The save identifier, expected format "Save##_...".
         * @return The extracted save number, or -1 if the format is invalid.
         */
        int _getSaveNumberFromIdentifier(const std::string& identifier) const;

        std::string ExtractTimelineID(const std::string& saveName);
        
    private:
        SessionDataManager() = default;
        ~SessionDataManager() = default;
        SessionDataManager(const SessionDataManager&) = delete;
        SessionDataManager& operator=(const SessionDataManager&) = delete;
        
        // Map to hold pending entries for the current session.
        // Key: The fileKey (e.g., "PlayerChronicle.txt" or a book title)
        // Value: A vector of new entries for that file.
        std::map<std::string, std::vector<std::string>> _sessionPendingEntries;
        std::mutex _dataMutex; // Protects access to the _sessionPendingEntries map

        std::string _currentSaveIdentifier; // e.g., "MyNordWarriorSave01"

        std::string g_currentSaveName;

        std::string _currentTimelineID;

        std::string _sessionParentSaveIdentifier;
    };

} // namespace DynamicBookFramework