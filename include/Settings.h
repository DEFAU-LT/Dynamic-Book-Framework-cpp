//Settings.h
#pragma once
#include "PCH.h"
#include "SKSEMenuFramework.h"



namespace Settings {

    // These variables will hold our settings globally.
    extern std::string defaultFontFace;
    extern int defaultFontSize;
    extern std::vector<std::string> userDefinedFonts;
    extern int openMenuHotkey;
    extern int bookmarkPageHotkey; // Default: C key
    extern int nextBookmarkHotkey;
    extern int previousBookmarkHotkey;
    extern bool isWaitingForHotkey;

    extern std::map<std::string, std::vector<std::string>> g_bookmarks;

    // Function to load all settings from the INI file.
    void LoadSettings();

    // Function to save all settings to the INI file.
    void SaveSettings();

    std::string GetNameFromScancode(int a_key);
    int ImGuiKeyToDXScancode(ImGuiKey a_key);
    ImGuiKey DXScancodeToImGuiKey(int a_scancode);
    int GetScancodeFromName(const std::string& a_keyName);

    // Add new function declarations for bookmarks
    void SaveBookmark(const std::string& bookTitle, const std::string& anchorText);
    const std::vector<std::string>& GetBookmarksForBook(const std::string& bookTitle);
    const std::map<std::string, std::vector<std::string>>& GetAllBookmarks();
    void RemoveBookmark(const std::string& bookTitle);
    void ScanAllBooksForBookmarks();

    // Declare the new functions so other files know they exist.
    std::vector<std::string> ParseTagsFromText(const std::string& text);

} // namespace Settings