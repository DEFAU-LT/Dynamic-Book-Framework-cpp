//ImGuiRender.cpp
#include "ImGuiMenu.h"
#include "PCH.h"
#include "SessionDataManager.h"
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
        ImGui::StyleColorsDark();
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

        std::filesystem::path bookPath = std::filesystem::path("Data/SKSE/Plugins/books") / safeFilename;
        std::filesystem::path iniPath = "Data/SKSE/Plugins/DynamicBookFramework/UserBooks.ini";

        // 1. Create the new book's .txt file
        std::ofstream bookFile(bookPath);
        if (!bookFile.is_open()) {
            logger::error("Failed to create new book file at: {}", bookPath.string());
            return false;
        }
        bookFile << "--- " << bookTitle << " ---\n\n";
        bookFile.close();

        // 2. Append the new mapping to the INI file
        std::ofstream iniFile(iniPath, std::ios::app);
        if (iniFile.is_open()) {
            iniFile << "\n" << bookTitle << " = " << safeFilename;
            iniFile.close();
            logger::info("New book '{}' added to INI.", bookTitle);
        } else {
             logger::error("Failed to open INI file to add new book mapping.");
            // Don't return false, as the file was still created.
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

    // This is the main drawing function that we registered.
    void RenderEditorWindow() {

        if (!EditorWindow || !EditorWindow->IsOpen) {
            return;
        }

        if (ImGui::IsKeyReleased(ImGuiKey_F10)) {
            KeyReleased = true;
        }
        
        if (KeyReleased && ImGui::IsKeyPressed(ImGuiKey_F10, false) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            CloseMenu();
            return;
        }

        static char editorBuffer[16384] = "";

        static std::vector<std::string> bookTitles;
        static int selectedBookIndex = -1;


        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f); // soft rounded corners
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

        // Load book titles only once when the menu is first opened for a session.
        if (ImGui::IsWindowAppearing()) {
            bookTitles = GetAllBookTitles();
            selectedBookIndex = bookTitles.empty() ? -1 : 0;
        }

        // Use the ImGui:: namespace for all function calls.
        auto viewport = ImGui::GetMainViewport();

        auto center = ImGui::ImVec2Manager::Create();
        ImGui::ImGuiViewportManager::GetCenter(center, viewport);
        ImGui::SetNextWindowPos(*center, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
        ImGui::ImVec2Manager::Destroy(center);
        ImGui::SetNextWindowSize({ 600, 700 }, ImGuiCond_Appearing);

        bool is_open = EditorWindow->IsOpen.load();
        ImGui::Begin("Dynamic Book Editor##DBF_Editor", &is_open, ImGuiWindowFlags_MenuBar);
        if (!is_open) {
            EditorWindow->IsOpen.store(false);
        }

        // --- UI Content ---
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Editor")) {
                if (ImGui::MenuItem("Close Window", nullptr, false, true)) {
                    EditorWindow->IsOpen = false;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        ImGui::Text("Framework Configuration");
        ImGui::Separator();
        
        if (ImGui::Button("Reload Book Mappings", {0, 0})) {
            LoadBookMappings();
            bookTitles = GetAllBookTitles();
            selectedBookIndex = bookTitles.empty() ? -1 : 0;
            strcpy_s(editorBuffer, "");
        }
        ImGui::SameLine();
        if (ImGui::Button("Create New Book")) {
            ImGui::OpenPopup("Create New Book");
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Live Book Editor");
        ImGui::Separator();

        const char* currentSelection = (selectedBookIndex != -1 && !bookTitles.empty()) ? bookTitles[selectedBookIndex].c_str() : "No books found in INI";
        if (ImGui::BeginCombo("Book Title", currentSelection)) {
            for (int i = 0; i < bookTitles.size(); ++i) {
                const bool isSelected = (selectedBookIndex == i);
                if (ImGui::Selectable(bookTitles[i].c_str(), isSelected)) {
                    selectedBookIndex = i;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Load for Editing", {0, 0})) {
            if (selectedBookIndex != -1 && !bookTitles.empty()) {
                std::string bookTitle = bookTitles[selectedBookIndex];
                if (auto pathOpt = GetDynamicBookPathByTitle(string_to_wstring(bookTitle))) {
                    if (std::filesystem::exists(*pathOpt)) {
                        std::ifstream file(*pathOpt);
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        strcpy_s(editorBuffer, sizeof(editorBuffer), buffer.str().c_str());
                    } else { strcpy_s(editorBuffer, ""); }
                } else { strcpy_s(editorBuffer, ""); }
            }
        }
        

        ImGui::Text("Book Content:");
        ImGui::Separator();

        // Get available space for editor height
        ImVec2 avail{};
        ImGui::GetContentRegionAvail(&avail);

        // Reserve only enough height for the Save button + spacing
        float saveHeight = ImGui::GetFrameHeightWithSpacing();
        ImVec2 editorSize = ImVec2(avail.x, avail.y - saveHeight);


        // Begin scrollable region for editor
        ImGui::BeginChild("EditorRegion", editorSize, true, 0);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::InputTextMultiline("##Editor", editorBuffer, sizeof(editorBuffer),
                                ImVec2(-FLT_MIN, -FLT_MIN), 0, nullptr, nullptr);
        ImGui::PopTextWrapPos();
        ImGui::EndChild();

        // No extra spacing; stick the button to bottom
        if (ImGui::Button("Save Changes", ImVec2(avail.x, 0))) {
            if (!bookTitles.empty()) {
                std::string bookTitle = bookTitles[selectedBookIndex];
                std::string newContent = editorBuffer;
                WriteBookFile(bookTitle, newContent);
            }
        }

        // --- NEW: Popup window for creating a new book ---
        if (ImGui::BeginPopupModal("Create New Book", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char newBookTitleBuffer[128] = "";
            ImGui::InputText("New Book Title", newBookTitleBuffer, sizeof(newBookTitleBuffer));

            if (ImGui::Button("Create", { 0, 0 })) {
                if (CreateNewBookFileAndMapping(newBookTitleBuffer)) {
                    // Refresh the book list to include the new one
                    bookTitles = GetAllBookTitles();
                    // Optional: select the newly created book
                    for(int i = 0; i < bookTitles.size(); ++i) {
                        if (bookTitles[i] == newBookTitleBuffer) {
                            selectedBookIndex = i;
                            break;
                        }
                    }
                }
                strcpy_s(newBookTitleBuffer, ""); // Clear buffer
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", { 0, 0 })) {
                strcpy_s(newBookTitleBuffer, ""); // Clear buffer
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }   
        
        ImGui::End();
    }

} // namespace ImGuiRender
