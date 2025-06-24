// Settings.cpp
#include "PCH.h"
#include "Settings.h"
#include "Utility.h"


// We will assume you have a simple INI parser or will use one.
// For this example, we'll use standard file I/O to keep it simple,
namespace Settings {

    // Global variables with their default values
    std::string defaultFontFace = "$HandwrittenFont";
    int defaultFontSize = 20;
    std::vector<std::string> userDefinedFonts;
    int openMenuHotkey = 0x44; // Default to F10
    bool isWaitingForHotkey = false;

    // The single path for our settings file
    const std::string settingsPath = "Data/SKSE/Plugins/DynamicBookFramework/Settings.ini";
    
    // A constant list of the original default fonts. This helps us sort them back into the correct section when saving.
    const std::vector<std::string> officialDefaultFonts = {
        "$HandwrittenFont", "$SkyrimBooks", "$DaedricFont", "$DragonFont",
        "$FalmerFont", "$DwemerFont", "$MageScriptFont"
    };

    void SaveSettings() {
        std::ofstream iniFile(settingsPath);
        if (!iniFile.is_open()) {
            logger::error("Failed to open {} for writing.", settingsPath);
            return;
        }

        logger::info("Saving settings to {}", settingsPath);

        // Write Hotkeys section
        std::string hotkeyNameToSave = GetNameFromScancode(openMenuHotkey);        
        iniFile << "[Hotkeys]\n";
        iniFile << "; The DirectX Scancode for the key that opens the menu.\n";
        iniFile << "; A list of codes can be found here: https://ck.uesp.net/wiki/Input_Script#DXScanCodes\n";
        iniFile << "OpenMenu = " << hotkeyNameToSave << "\n\n";

        // Write Appearance section
        iniFile << "[Appearance]\n";
        iniFile << "; The font face and size to use for books that don't have RAW HTML markup.\n";
        iniFile << "FontFace = " << defaultFontFace << "\n";
        iniFile << "FontSize = " << defaultFontSize << "\n\n";

        // Write Default Fonts section
        iniFile << "[Default Fonts]\n";
        iniFile << "; The list of default fonts that come with the mod.\n";
        int defaultKey = 1;
        for (const auto& font : userDefinedFonts) {
            if (std::find(officialDefaultFonts.begin(), officialDefaultFonts.end(), font) != officialDefaultFonts.end()) {
                iniFile << "Font" << defaultKey++ << " = " << font << "\n";
            }
        }
        iniFile << "\n";

        // Write User Fonts section
        iniFile << "[User Fonts]\n";
        iniFile << "; Add your own custom fonts here (e.g., from other mods).\n";
        iniFile << "; The name must match a font recognized by Skyrim.\n";
        int userKey = 1;
        for (const auto& font : userDefinedFonts) {
            if (std::find(officialDefaultFonts.begin(), officialDefaultFonts.end(), font) == officialDefaultFonts.end()) {
                iniFile << "UserFont" << userKey++ << " = " << font << "\n";
            }
        }
    }

    void LoadSettings() {
        // Clear previous font list and set defaults before loading
        userDefinedFonts.clear();
        defaultFontFace = "$HandwrittenFont";
        defaultFontSize = 20;
        openMenuHotkey = 0x44; // Default to F10

        std::ifstream iniFile(settingsPath);
        if (!iniFile.is_open()) {
            logger::info("Settings.ini not found. Using default values and creating a new file.");
            userDefinedFonts = officialDefaultFonts;
            SaveSettings();
            return;
        }

        logger::info("Loading settings from {}", settingsPath);
        std::string line;
        std::string currentSection;
        while (std::getline(iniFile, line)) {
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);

            if (line.empty() || line[0] == ';') continue;

            if (line[0] == '[' && line.back() == ']') {
                currentSection = line;
            } else if (currentSection == "[Hotkeys]") {
                std::stringstream ss(line);
                std::string key, value;
                if (std::getline(ss, key, '=') && std::getline(ss, value)) {
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    if (key == "OpenMenu") {
                        // Read the string name (e.g., "F10") from the INI
                        std::string hotkeyName = value;
                        // Use our helper function to convert the name to a scancode
                        openMenuHotkey = GetScancodeFromName(hotkeyName);
                        // As a safety measure, if the name is invalid, default to F10
                        if (openMenuHotkey == 0) {
                            openMenuHotkey = 0x44; 
                        }
                    }
                }
            } else if (currentSection == "[Appearance]") {
                std::stringstream ss(line);
                std::string key, value;
                if (std::getline(ss, key, '=') && std::getline(ss, value)) {
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    if (key == "FontFace") defaultFontFace = value;
                    else if (key == "FontSize") defaultFontSize = std::stoi(value);
                }
            } else if (currentSection == "[Default Fonts]" || currentSection == "[User Fonts]") {
                std::stringstream ss(line);
                std::string key, value;
                if (std::getline(ss, key, '=') && std::getline(ss, value)) {
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    if (!value.empty()) {
                        userDefinedFonts.push_back(value);
                    }
                }
            }
        }
        
        logger::info("Finished loading settings:");
        logger::info("  OpenMenu Hotkey -> {} ({})", openMenuHotkey, GetNameFromScancode(openMenuHotkey));
        logger::info("  FontFace -> {}", defaultFontFace);
        logger::info("  FontSize -> {}", defaultFontSize);
        for(const auto& font : userDefinedFonts) {
            logger::info("  Loaded Font -> {}", font);
        }
    }

    // --- The new struct to hold all key information ---
    struct KeyInfo {
        std::string name;
        ImGuiKey    imKey;
    };

    static const std::map<int, KeyInfo> KEY_MAP = {
        { 0x01, { "Escape", ImGuiKey_Escape } }, { 0x02, { "1", ImGuiKey_1 } },
        { 0x03, { "2", ImGuiKey_2 } }, { 0x04, { "3", ImGuiKey_3 } },
        { 0x05, { "4", ImGuiKey_4 } }, { 0x06, { "5", ImGuiKey_5 } },
        { 0x07, { "6", ImGuiKey_6 } }, { 0x08, { "7", ImGuiKey_7 } },
        { 0x09, { "8", ImGuiKey_8 } }, { 0x0A, { "9", ImGuiKey_9 } },
        { 0x0B, { "0", ImGuiKey_0 } }, { 0x0C, { "Minus", ImGuiKey_Minus } },
        { 0x0D, { "Equals", ImGuiKey_Equal } }, { 0x0E, { "Backspace", ImGuiKey_Backspace } },
        { 0x0F, { "Tab", ImGuiKey_Tab } }, { 0x10, { "Q", ImGuiKey_Q } },
        { 0x11, { "W", ImGuiKey_W } }, { 0x12, { "E", ImGuiKey_E } },
        { 0x13, { "R", ImGuiKey_R } }, { 0x14, { "T", ImGuiKey_T } },
        { 0x15, { "Y", ImGuiKey_Y } }, { 0x16, { "U", ImGuiKey_U } },
        { 0x17, { "I", ImGuiKey_I } }, { 0x18, { "O", ImGuiKey_O } },
        { 0x19, { "P", ImGuiKey_P } }, { 0x1A, { "Left Bracket", ImGuiKey_LeftBracket } },
        { 0x1B, { "Right Bracket", ImGuiKey_RightBracket } }, { 0x1C, { "Enter", ImGuiKey_Enter } },
        { 0x1D, { "Left Control", ImGuiKey_LeftCtrl } }, { 0x1E, { "A", ImGuiKey_A } },
        { 0x1F, { "S", ImGuiKey_S } }, { 0x20, { "D", ImGuiKey_D } },
        { 0x21, { "F", ImGuiKey_F } }, { 0x22, { "G", ImGuiKey_G } },
        { 0x23, { "H", ImGuiKey_H } }, { 0x24, { "J", ImGuiKey_J } },
        { 0x25, { "K", ImGuiKey_K } }, { 0x26, { "L", ImGuiKey_L } },
        { 0x27, { "Semicolon", ImGuiKey_Semicolon } }, { 0x28, { "Apostrophe", ImGuiKey_Apostrophe } },
        { 0x29, { "Tilde", ImGuiKey_GraveAccent } }, { 0x2A, { "Left Shift", ImGuiKey_LeftShift } },
        { 0x2B, { "Backslash", ImGuiKey_Backslash } }, { 0x2C, { "Z", ImGuiKey_Z } },
        { 0x2D, { "X", ImGuiKey_X } }, { 0x2E, { "C", ImGuiKey_C } },
        { 0x2F, { "V", ImGuiKey_V } }, { 0x30, { "B", ImGuiKey_B } },
        { 0x31, { "N", ImGuiKey_N } }, { 0x32, { "M", ImGuiKey_M } },
        { 0x33, { "Comma", ImGuiKey_Comma } }, { 0x34, { "Period", ImGuiKey_Period } },
        { 0x35, { "Slash", ImGuiKey_Slash } }, { 0x36, { "Right Shift", ImGuiKey_RightShift } },
        { 0x38, { "Left Alt", ImGuiKey_LeftAlt } }, { 0x39, { "Space", ImGuiKey_Space } },
        { 0x3A, { "Caps Lock", ImGuiKey_CapsLock } }, { 0x3B, { "F1", ImGuiKey_F1 } },
        { 0x3C, { "F2", ImGuiKey_F2 } }, { 0x3D, { "F3", ImGuiKey_F3 } },
        { 0x3E, { "F4", ImGuiKey_F4 } }, { 0x3F, { "F5", ImGuiKey_F5 } },
        { 0x40, { "F6", ImGuiKey_F6 } }, { 0x41, { "F7", ImGuiKey_F7 } },
        { 0x42, { "F8", ImGuiKey_F8 } }, { 0x43, { "F9", ImGuiKey_F9 } },
        { 0x44, { "F10", ImGuiKey_F10 } }, { 0x57, { "F11", ImGuiKey_F11 } },
        { 0x58, { "F12", ImGuiKey_F12 } }, { 0x9D, { "Right Control", ImGuiKey_RightCtrl } },
        { 0xB8, { "Right Alt", ImGuiKey_RightAlt } }, { 0xC7, { "Home", ImGuiKey_Home } },
        { 0xC8, { "Up Arrow", ImGuiKey_UpArrow } }, { 0xC9, { "Page Up", ImGuiKey_PageUp } },
        { 0xCB, { "Left Arrow", ImGuiKey_LeftArrow } }, { 0xCD, { "Right Arrow", ImGuiKey_RightArrow } },
        { 0xCF, { "End", ImGuiKey_End } }, { 0xD0, { "Down Arrow", ImGuiKey_DownArrow } },
        { 0xD1, { "Page Down", ImGuiKey_PageDown } }, { 0xD2, { "Insert", ImGuiKey_Insert } },
        { 0xD3, { "Delete", ImGuiKey_Delete } }
        // Numpad keys are excluded for simplicity, but could be added here.
    };

    // --- All our helper functions now use the single master map ---

    std::string GetNameFromScancode(int a_key) {
        auto it = KEY_MAP.find(a_key);
        return (it != KEY_MAP.end()) ? it->second.name : "Unknown Key";
    }

    ImGuiKey DXScancodeToImGuiKey(int a_scancode) {
        auto it = KEY_MAP.find(a_scancode);
        return (it != KEY_MAP.end()) ? it->second.imKey : ImGuiKey_None;
    }

    int ImGuiKeyToDXScancode(ImGuiKey a_key) {
        for (const auto& pair : KEY_MAP) {
            if (pair.second.imKey == a_key) {
                return pair.first; // Return the DirectX Scancode
            }
        }
        return 0;
    }

    // You can even create a replacement for the framework's GetKeyBinding if you want
    int GetScancodeFromName(const std::string& a_keyName) {
        for (const auto& pair : KEY_MAP) {
            // Simple case-insensitive comparison
            if (_stricmp(pair.second.name.c_str(), a_keyName.c_str()) == 0) {
                return pair.first;
            }
        }
        return 0;
    }

} // namespace Settings