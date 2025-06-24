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
    extern bool isWaitingForHotkey;

    // Function to load all settings from the INI file.
    void LoadSettings();

    // Function to save all settings to the INI file.
    void SaveSettings();

    std::string GetNameFromScancode(int a_key);
    int ImGuiKeyToDXScancode(ImGuiKey a_key);
    ImGuiKey DXScancodeToImGuiKey(int a_scancode);
    int GetScancodeFromName(const std::string& a_keyName);

} // namespace Settings