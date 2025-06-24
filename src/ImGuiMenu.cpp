//ImGuiRender.cpp
#include "ImGuiMenu.h"
#include "PCH.h"
#include "SessionDataManager.h"
#include "Settings.h"
#include "BookUIManager.h"
#include "Utility.h"


namespace ImGuiRender {

    void InitializeSKSEMenuFramework() {
        if (!SKSEMenuFramework::IsInstalled()) {
            return;
        }
    }
    // Public function to register our window with the framework.
    void Register() {
        // This call is correct for this library.
        EditorWindow = SKSEMenuFramework::AddWindow(RenderEditorWindow);
        //ImGui::StyleColorsDark();
        logger::info("Registered Dynamic Book Editor window.");
    }

    // Public function to toggle the menu's visibility.
    void ToggleMenu() {
        if (EditorWindow) {
            EditorWindow->IsOpen = !EditorWindow->IsOpen;
        }
    }

    void CloseMenu() {
        if (EditorWindow) {
            KeyReleased = false;
            EditorWindow->IsOpen = false;
        }
    }
    
    bool CreateNewBookFileAndMapping(const std::string& bookTitle) {
        if (bookTitle.empty()) {
            return false;
        }

        // Generate a safe filename from the title
        std::string safeFilename = bookTitle;
        for (char& c : safeFilename) {
            if (!isalnum(c)) {
                c = '_';
            }
        }
        safeFilename += ".txt";

        std::filesystem::path bookPath = std::filesystem::path("Data/SKSE/Plugins/DynamicBookFramework/books") / safeFilename;
        std::filesystem::path iniPath = "Data/SKSE/Plugins/DynamicBookFramework/UserBooks.ini";

        // 1. Create the new book's .txt file
        std::ofstream bookFile(bookPath);
        if (!bookFile.is_open()) {
            logger::error("Failed to create new book file at: {}", bookPath.string());
            return false;
        }
        bookFile << "--- " << bookTitle << " ---\n\n";
        bookFile.close();

         bool sectionExists = false;
        // 1. Check if the [Books] section already exists.
        std::ifstream checkFile(iniPath);
        if (checkFile.is_open()) {
            std::string line;
            while (std::getline(checkFile, line)) {
                // Simple check for the section header
                if (line.find("[Books]") != std::string::npos) {
                    sectionExists = true;
                    break;
                }
            }
            checkFile.close();
        }

        // 2. Open the file in append mode to add the new content.
        std::ofstream iniFile(iniPath, std::ios::app);
        if (iniFile.is_open()) {
            // 3. If the section was not found, write it first.
            if (!sectionExists) {
                iniFile << "\n[Books]\n";
            }
            // 4. Now, write the new book mapping.
            iniFile << bookTitle << " = " << safeFilename << "\n";
            iniFile.close();
            logger::info("New book '{}' added to INI.", bookTitle);
        } else {
            logger::error("Failed to open INI file to add new book mapping.");
            // We don't return false, because the .txt file was still created.
            // The user can add the mapping manually if needed.
        }

        // 3. Reload the mappings so the new book appears in the list.
        LoadBookMappings();
        return true;
    }

    // Helper function to write to files.
    bool WriteBookFile(const std::string& bookTitle, const std::string& content) {
        if (auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(bookTitle))) {
            std::filesystem::path bookPath = *pathOpt;
            std::ofstream bookFile(bookPath, std::ios::out | std::ios::trunc);
            if (!bookFile.is_open()) return false;
            bookFile << content;
            bookFile.close();
            return true;
        }
        return false;
    }

    void RenderEditorWindow() {
        if (!EditorWindow || !EditorWindow->IsOpen) {
            return;
        }

        int hotkeyScancode = Settings::openMenuHotkey;

        // Use our new helper function to look up the correct ImGuiKey
        ImGuiKey hotkeyAsImGuiKey = Settings::DXScancodeToImGuiKey(hotkeyScancode);

        // Now, use this CORRECT ImGuiKey value in all your ImGui checks
        if (ImGui::IsKeyReleased(hotkeyAsImGuiKey)) {
            KeyReleased = true;
        }
        if ((KeyReleased && ImGui::IsKeyPressed(hotkeyAsImGuiKey, false)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            CloseMenu();
            return;
        }
        
        static char editorBuffer[16384] = "";
        static std::vector<std::string> bookTitles;
        static int selectedBookIndex = -1;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
        if (ImGui::IsWindowAppearing()) {
            bookTitles = GetAllBookTitles();
            selectedBookIndex = bookTitles.empty() ? -1 : 0;
        }
        auto viewport = ImGui::GetMainViewport();
        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
        ImGui::ImVec2Manager::Destroy(center);
        ImGui::SetNextWindowSize({ 750, 800 }, ImGuiCond_Appearing);
        bool is_open = EditorWindow->IsOpen.load();
        ImGui::Begin("Dynamic Book Editor##DBF_Editor", &is_open, ImGuiWindowFlags_MenuBar);
        if (!is_open) {
            EditorWindow->IsOpen.store(false);
        }
    
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Editor")) {
                if (ImGui::MenuItem("Close Window", nullptr, false, true)) {
                    EditorWindow->IsOpen = false;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::CollapsingHeader("Framework Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            if (ImGui::BeginTable("config_table", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Menu Hotkey");

                ImGui::TableSetColumnIndex(1);
                
                // This static bool now lives only inside this function
                static bool isWaitingForHotkey = false;
                
                // Get the name of the currently set hotkey
                std::string keyName = Settings::GetNameFromScancode(Settings::openMenuHotkey);

                if (isWaitingForHotkey) {
                    ImGui::Button("Press any key...", {-1.0f, 0.0f});

                    // Your new and improved capture loop
                    ImGuiIO* io = ImGui::GetIO();
                    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                        // Check if a key was just pressed
                        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false)) {
                            // Convert the captured ImGuiKey to a DirectX Scancode
                            int dxScancode = Settings::ImGuiKeyToDXScancode(static_cast<ImGuiKey>(key));
                            
                            // If it's a valid key, update our setting
                            if (dxScancode != 0) {
                                Settings::openMenuHotkey = dxScancode;
                            }

                            // Stop waiting for input
                            isWaitingForHotkey = false;
                            break;
                        }
                    }
                } else {
                    if (ImGui::Button(keyName.c_str(), {-1.0f, 0.0f})) {
                        isWaitingForHotkey = true;
                    }
                }
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Default Font Face");

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-1.0f); // Stretch combo box
                int currentFontIndex = -1;
                for (int i = 0; i < Settings::userDefinedFonts.size(); i++) {
                    if (Settings::defaultFontFace == Settings::userDefinedFonts[i]) {
                        currentFontIndex = i;
                        break;
                    }
                }
                const char* comboPreview = (currentFontIndex != -1) ? Settings::userDefinedFonts[currentFontIndex].c_str() : Settings::defaultFontFace.c_str();
                if (ImGui::BeginCombo("##FontFace", comboPreview)) {
                    for (int i = 0; i < Settings::userDefinedFonts.size(); i++) {
                        const bool isSelected = (currentFontIndex == i);
                        if (ImGui::Selectable(Settings::userDefinedFonts[i].c_str(), isSelected)) {
                            Settings::defaultFontFace = Settings::userDefinedFonts[i];
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Default Font Size");

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-1.0f); // Stretch input
                ImGui::InputInt("##FontSize", &Settings::defaultFontSize, 1, 5);
                
                ImGui::EndTable();
            }

            ImGui::Spacing();
            // Group the action buttons together
            ImVec2 contentRegionAvail;
            ImGui::GetContentRegionAvail(&contentRegionAvail);
            float available_width = contentRegionAvail.x;
            float spacing = ImGui::GetStyle()->ItemSpacing.x;
            float button_width = (available_width - (spacing * 2.0f)) / 3.0f;

            if (ImGui::Button("Save & Apply", ImVec2(button_width, 0))) {
                Settings::SaveSettings();
                DynamicBookFramework::BookUIManager::RefreshCurrentlyOpenBook();
            }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::Button("Reload Settings", ImVec2(button_width, 0))) {
                Settings::LoadSettings();
            }
            ImGui::SameLine(0.0f, spacing);
            if (ImGui::Button("Reload Mappings", ImVec2(button_width, 0))) {
                LoadBookMappings();
                bookTitles = GetAllBookTitles();
                selectedBookIndex = bookTitles.empty() ? -1 : 0;
                strcpy_s(editorBuffer, "");
            }
            ImGui::Spacing();
        }

        // Live Editor Section
        ImGui::Text("Live Book Editor");
        ImGui::Separator();

        ImVec2 region;
        ImGui::GetContentRegionAvail(&region);
        float usableWidth = region.x;
        float spacing = ImGui::GetStyle()->ItemSpacing.x;

        // Determine width ratios
        ImVec2 textSize;
        ImGui::CalcTextSize(&textSize, "Load for Editing", nullptr, false, 0.0f);
        float buttonWidth = textSize.x + ImGui::GetStyle()->FramePadding.x * 2.0f + 10.0f;
        //float buttonWidth = ImGui::CalcTextSize("Load for Editing").x + ImGui::GetStyle()->FramePadding.x * 2.0f + 10.0f; // Some padding
        float comboWidth = usableWidth - spacing - buttonWidth;

        // Combo
        ImGui::SetNextItemWidth(comboWidth);
        const char* currentSelection = (selectedBookIndex != -1 && !bookTitles.empty()) ? bookTitles[selectedBookIndex].c_str() : "No books found in INI";
        if (ImGui::BeginCombo("##BookTitle", currentSelection)) {
            for (int i = 0; i < bookTitles.size(); ++i) {
                const bool isSelected = (selectedBookIndex == i);
                if (ImGui::Selectable(bookTitles[i].c_str(), isSelected)) {
                    selectedBookIndex = i;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        // Button
        if (ImGui::Button("Load for Editing", ImVec2(buttonWidth, 0))) {
            if (selectedBookIndex != -1 && !bookTitles.empty()) {
                std::string bookTitle = bookTitles[selectedBookIndex];
                if (auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(bookTitle))) {
                    if (std::filesystem::exists(*pathOpt)) {
                        std::ifstream file(*pathOpt);
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        strcpy_s(editorBuffer, sizeof(editorBuffer), buffer.str().c_str());
                    } else {
                        strcpy_s(editorBuffer, "");
                    }
                } else {
                    strcpy_s(editorBuffer, "");
                }
            }
        }
        if (ImGui::Button("Create New Book", ImVec2(-1.0f, 0.0f))) {
            ImGui::OpenPopup("Create New Book");
        }

        ImGui::Text("Book Content:");
        ImGui::Separator();
        ImVec2 avail{};
        ImGui::GetContentRegionAvail(&avail);
        float saveHeight = ImGui::GetFrameHeightWithSpacing();
        ImVec2 editorSize = ImVec2(avail.x, avail.y - saveHeight);
        ImGui::BeginChild("EditorRegion", editorSize, true, 0);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::InputTextMultiline("##Editor", editorBuffer, sizeof(editorBuffer),
                                ImVec2(-FLT_MIN, -FLT_MIN), 0, nullptr, nullptr);
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        if (ImGui::Button("Save Changes", ImVec2(avail.x, 0))) {
            if (!bookTitles.empty()) {
                std::string bookTitle = bookTitles[selectedBookIndex];
                std::string newContent = editorBuffer;
                WriteBookFile(bookTitle, newContent);
            }
        }
        if (ImGui::BeginPopupModal("Create New Book", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char newBookTitleBuffer[128] = "";
            ImGui::InputText("New Book Title", newBookTitleBuffer, sizeof(newBookTitleBuffer));
            if (ImGui::Button("Create", { 0, 0 })) {
                if (CreateNewBookFileAndMapping(newBookTitleBuffer)) {
                    bookTitles = GetAllBookTitles();
                    for(int i = 0; i < bookTitles.size(); ++i) {
                        if (bookTitles[i] == newBookTitleBuffer) {
                            selectedBookIndex = i;
                            break;
                        }
                    }
                }
                strcpy_s(newBookTitleBuffer, "");
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", { 0, 0 })) {
                strcpy_s(newBookTitleBuffer, "");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::End();
        ImGui::PopStyleVar(2); // Pop FrameRounding and FramePadding
    }
}