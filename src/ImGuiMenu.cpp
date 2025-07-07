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

    void RenderHotkeyButton(const char* label, int& hotkeySetting) {
        // Use the label to create a unique ID for the "waiting" state.
        // This map will store the waiting state for each button separately.
        static std::map<std::string, bool> waitingStates;
        bool& isWaiting = waitingStates[label];

        std::string keyName = Settings::GetNameFromScancode(hotkeySetting);
        
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", label);

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID(label); // Push ID to make buttons unique.

        if (isWaiting) {
            ImGui::Button("Press any key...", {-1.0f, 0.0f});
            ImGuiIO* io = ImGui::GetIO(); // Use a reference for io
            for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false)) {
                    int dxScancode = Settings::ImGuiKeyToDXScancode(static_cast<ImGuiKey>(key));
                    if (dxScancode != 0) {
                        hotkeySetting = dxScancode;
                    }
                    isWaiting = false;
                    break;
                }
            }
        } else {
            if (ImGui::Button(keyName.c_str(), {-1.0f, 0.0f})) {
                isWaiting = true;
            }
        }
        ImGui::PopID();
    }

    void RenderSingleHotkeyButton(const char* label, int& hotkeySetting, const ImVec2& button_size) {
        static std::map<std::string, bool> waitingStates;
        bool& isWaiting = waitingStates[label];
        std::string keyName = Settings::GetNameFromScancode(hotkeySetting);

        ImGui::PushID(label);
        if (isWaiting) {
            ImGui::Button("..."); // Small label while waiting
            ImGuiIO* io = ImGui::GetIO();
            for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false)) {
                    int dxScancode = Settings::ImGuiKeyToDXScancode(static_cast<ImGuiKey>(key));
                    if (dxScancode != 0) {
                        hotkeySetting = dxScancode;
                    }
                    isWaiting = false;
                    break;
                }
            }
        } else {
            if (ImGui::Button(keyName.c_str(), button_size)) {
                isWaiting = true;
            }
        }
        ImGui::PopID();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s Hotkey", label);
        }
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
            Settings::ScanAllBooksForBookmarks();
            selectedBookIndex = bookTitles.empty() ? -1 : 0;
        }
        auto viewport = ImGui::GetMainViewport();
        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
        ImGui::ImVec2Manager::Destroy(center);
        ImGui::SetNextWindowSize({ 750, 800 }, ImGuiCond_Appearing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));        
        bool is_open = EditorWindow->IsOpen.load();
        ImGui::Begin("Dynamic Book Editor##DBF_Editor", &is_open, 0);

        ImGui::PopStyleVar();

        if (!is_open) {
            EditorWindow->IsOpen.store(false);
        }

        if (ImGui::BeginTabBar("MainTabBar")) {
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));

        // --- TAB 1: EDITOR ---
            if (ImGui::BeginTabItem("Editor")) {
                if (ImGui::CollapsingHeader("Framework Configuration")) {
                    ImGui::Spacing();                    

                    if (ImGui::BeginTable("config_table", 2, ImGuiTableFlags_SizingStretchProp)) {
                        // Use the helper function to draw each hotkey row
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Hotkeys");

                        ImGui::TableSetColumnIndex(1);
                        
                        ImVec2 contentRegionAvail;
                        ImGui::GetContentRegionAvail(&contentRegionAvail);
                        float available_width = contentRegionAvail.x;
                        float spacing = ImGui::GetStyle()->ItemSpacing.x;
                        float button_width = (available_width - (spacing * 2.0f)) / 3.0f;

                        // Call the helper for each button, passing the calculated size.
                        RenderSingleHotkeyButton("Menu", Settings::openMenuHotkey, ImVec2(button_width, 0));
                        ImGui::SameLine(0.0f, spacing);
                        RenderSingleHotkeyButton("Next Bookmark", Settings::nextBookmarkHotkey, ImVec2(button_width, 0));
                        ImGui::SameLine(0.0f, spacing);
                        RenderSingleHotkeyButton("Previous Bookmark", Settings::previousBookmarkHotkey, ImVec2(button_width, 0));
                        
        

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
                        Settings::ScanAllBooksForBookmarks();
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

<<<<<<< Updated upstream
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
=======
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
>>>>>>> Stashed changes
                        }
                    }
                }
                ImVec2 contentRegionAvail;
                ImGui::GetContentRegionAvail(&contentRegionAvail);
                float available_width_buttons = contentRegionAvail.x;
                float spacing_buttons = ImGui::GetStyle()->ItemSpacing.x;
                float half_button_width = (available_width_buttons - spacing_buttons) / 2.0f;
                
                if (ImGui::Button("Load Vanilla", ImVec2(half_button_width, 0))) {
                    if (selectedBookIndex != -1 && !bookTitles.empty()) {
                        std::string bookTitle = bookTitles[selectedBookIndex];
                        
                        // Call our new cache-retrieval function
                        auto vanillaContentOpt = DynamicBookFramework::BookMenuWatcher::GetSingleton()->GetCachedVanillaText(bookTitle);

                        if (vanillaContentOpt) {
                            // If content was found in the cache, copy it into the editor buffer
                            strcpy_s(editorBuffer, sizeof(editorBuffer), vanillaContentOpt->c_str());
                            logger::info("Loaded cached vanilla content for '{}' into editor.", bookTitle);
                        } else {
                            // If not found, inform the user they need to open the book first
                            const char* message = "## Open the book in-game first to cache its vanilla content. ##";
                            strcpy_s(editorBuffer, sizeof(editorBuffer), message);
                            logger::warn("Could not load vanilla content for '{}'. It has not been cached yet.", bookTitle);
                        }
                    }
                }
                ImGui::SameLine(0.0f, spacing_buttons);
                if (ImGui::Button("Create New Mapping", ImVec2(half_button_width, 0))) {
                    ImGui::OpenPopup("Create New Mapping");
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
                    if (selectedBookIndex != -1 && !bookTitles.empty()) {
                        std::string bookTitle = bookTitles[selectedBookIndex];
                        std::string newContent = editorBuffer;

                        // Step 1: Write the new content to the file.
                        WriteBookFile(bookTitle, newContent);
                        logger::info("Saved new content for '{}' to file.", bookTitle);

                        // Step 2: Notify the file watcher that we just updated this file.
                        // This prevents the watcher from triggering a second, unnecessary refresh.
                        DynamicBookFramework::FileWatcher::NotifyFileUpdated(bookTitle);

                        // Step 3: Manually reload the watcher's cache with the new content.
                        if (auto* currentBook = RE::BookMenu::GetTargetForm()) {
                            if (currentBook->GetFullName() == bookTitle) {
                                DynamicBookFramework::BookMenuWatcher::GetSingleton()->ReloadAndCacheBook(currentBook);
                            }
                        }
                        
                        // Step 4: Manually trigger an instant UI refresh for immediate feedback.
                        DynamicBookFramework::BookUIManager::RefreshCurrentlyOpenBook();
                    }
                }
                if (ImGui::BeginPopupModal("Create New Mapping", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
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
            
            ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Bookmarks")) {
                std::string openBookTitle;
                RE::GPtr<RE::IMenu> menu = RE::UI::GetSingleton()->GetMenu(RE::BookMenu::MENU_NAME);
                auto* bookMenu = menu ? static_cast<RE::BookMenu*>(menu.get()) : nullptr;
                if (bookMenu && bookMenu->GetTargetForm()) {
                    openBookTitle = bookMenu->GetTargetForm()->GetName();
                }

                // Get all bookmarks from settings.
                const auto& allBookmarks = Settings::GetAllBookmarks();

                if (allBookmarks.empty()) {
                    ImGui::Text("No bookmarks found. Add tags like [bookmark1] to your books via the editor.");
                } else {
                    // Loop through each book that has bookmarks.
                    for (const auto& [bookTitle, anchors] : allBookmarks) {
                        // Use a collapsing header for each book title.
                        if (ImGui::CollapsingHeader(bookTitle.c_str())) {
                            
                            // Create a table to align the buttons.
                            if (ImGui::BeginTable(("table_" + bookTitle).c_str(), 2, ImGuiTableFlags_SizingStretchProp)) {
                                
                                // Loop through each individual bookmark anchor for this book.
                                for (const auto& anchor : anchors) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("%s", anchor.c_str()); // Display the bookmark tag

                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::PushID(anchor.c_str()); // Give button a unique ID

                                    // Disable the button if this book isn't currently open.
                                    if (bookTitle != openBookTitle) {
                                        ImGui::BeginDisabled();
                                    }
                                    const char* button_text = "Go To";
                                    // Calculate the width of the button's text
                                    ImVec2 button_size;
                                    ImGui::CalcTextSize(&button_size, button_text, NULL, true, -1.0f);
                                    // Move the cursor to the right, minus the button's width, to right-align it.
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - button_size.x - ImGui::GetStyle()->FramePadding.x * 2.0f);

                                    if (ImGui::Button(button_text)) {
                                        // If the button is clicked and the book is open...
                                        if (bookMenu && bookMenu->uiMovie) {
                                            auto& rtData = REL::RelocateMember<RE::BookMenu::RUNTIME_DATA>(bookMenu, 0x50, 0x60);
                                            auto* movieView = rtData.book.get();
                                            RE::FxResponseArgs<1> gotoArgs;
                                            gotoArgs.Add(anchor.c_str());
                                            RE::FxDelegate::Invoke(movieView, "GotoPageByAnchor", gotoArgs);
                                            // Optional: Close the editor window after jumping.
                                            // EditorWindow->IsOpen = false;
                                        }
                                    }

                                    if (bookTitle != openBookTitle) {
                                        ImGui::EndDisabled();
                                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                                            ImGui::SetTooltip("You must have this book open in-game to jump to the bookmark.");
                                        }
                                    }

                                    ImGui::PopID();
                                }
                                ImGui::EndTable();
                            }
                        }
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
        ImGui::PopStyleVar(2); // Pop FrameRounding and FramePadding
    }
}