#include "InputListener.h"
#include "ImGuiMenu.h" // We need this to call ToggleMenu()
#include "BookUIManager.h"
#include "Settings.h"
#include "SetBookTextHook.h" // You no longer need this for bookmarks
#include "Utility.h"

namespace { // Anonymous namespace for local helpers and state

	// Helper to find a book form from its title string
	RE::TESObjectBOOK* GetBookFormByTitle(const std::string& a_title) {
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return nullptr;
		for (const auto& book : dataHandler->GetFormArray<RE::TESObjectBOOK>()) {
			if (book && book->GetFullName() == a_title) {
				return book;
			}
		}
		return nullptr;
	}

	// State for tracking bookmark cycling
	static int g_currentBookmarkIndex = -1;
	static std::string g_lastBookTitleForCycle = "";
}

// All of your class's functions must be defined within its namespace

InputListener* InputListener::GetSingleton() {
    static InputListener singleton;
    return &singleton;
}

void InputListener::Install() {
    auto* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(InputListener::GetSingleton());
        SKSE::log::info("Registered input event listener.");
    }
}

RE::BSEventNotifyControl InputListener::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;

    if (ImGuiRender::EditorWindow && ImGuiRender::EditorWindow->IsOpen) {
        ImGuiIO* io = ImGui::GetIO();
        if (io->WantCaptureKeyboard) {
            // If ImGui is expecting keyboard input (e.g., you've clicked a text box),
            // we "sink" the event, stopping it from ever reaching the game.
            return RE::BSEventNotifyControl::kStop;
        }
    }

    for (RE::InputEvent* event = *a_event; event; event = event->next) {
        if (event->eventType.get() != RE::INPUT_EVENT_TYPE::kButton) continue;
        
        const auto* buttonEvent = event->AsButtonEvent();
        if (!buttonEvent || !buttonEvent->IsDown() || buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;
        
        const auto key = buttonEvent->GetIDCode();
        auto* ui = RE::UI::GetSingleton();

        // --- ImGui Editor Hotkey Logic ---
        if (ImGuiRender::EditorWindow && ImGuiRender::EditorWindow->IsOpen) {
            // ... (your logic for closing the editor) ...
        } else {
            if (key == static_cast<uint32_t>(Settings::openMenuHotkey)) {
                ImGuiRender::ToggleMenu();
                return RE::BSEventNotifyControl::kStop;
            }
        }

       
        // --- In-Game Book Menu Hotkey Logic ---
        if (ui && ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
            auto menu = ui->GetMenu(RE::BookMenu::MENU_NAME);
            auto* bookMenu = menu ? static_cast<RE::BookMenu*>(menu.get()) : nullptr;


            if (bookMenu && bookMenu->uiMovie) {
                auto& rtData = REL::RelocateMember<RE::BookMenu::RUNTIME_DATA>(bookMenu, 0x50, 0x60);
                auto* movieView = rtData.book.get();
                auto* currentBook = bookMenu->GetTargetForm();

                if (!currentBook) continue;
                std::string currentTitle = currentBook->GetName();

                if (g_lastBookTitleForCycle != currentTitle) {
                    g_currentBookmarkIndex = -1;
                    g_lastBookTitleForCycle = currentTitle;
                }

                if (key == 0x2D) { // DirectX scancode for 'X' key
                    RE::FxResponseArgs<0> emptyArgs;
                    RE::FxDelegate::Invoke(movieView, "FocusTextInput", emptyArgs);
                    logger::info("X key pressed, requesting text input focus...");
                    return RE::BSEventNotifyControl::kStop;
                }

                if (key == 0x1C) { // DirectX scancode for 'Enter' key
                    RE::FxResponseArgs<0> emptyArgs;
                    RE::FxDelegate::Invoke(movieView, "SubmitTextInput", emptyArgs);
                    logger::info("Enter key pressed, requesting text input submit...");
                    return RE::BSEventNotifyControl::kStop;
                }
                // 2. Next/Previous Bookmark Hotkeys
                if (key == static_cast<uint32_t>(Settings::nextBookmarkHotkey) || key == static_cast<uint32_t>(Settings::previousBookmarkHotkey)) {
                    const auto& anchors = Settings::GetBookmarksForBook(currentTitle);
                    if (anchors.empty()) {
                        continue; // Exit if there are no bookmarks for this book.
                    }

                    // --- NEW: Reset the index if the book has changed ---
                    if (g_lastBookTitleForCycle != currentTitle) {
                        g_currentBookmarkIndex = -1;
                        g_lastBookTitleForCycle = currentTitle;
                    }
                    // --- END NEW ---

                    if (key == static_cast<uint32_t>(Settings::nextBookmarkHotkey)) {
                        g_currentBookmarkIndex++;
                        if (g_currentBookmarkIndex >= static_cast<int>(anchors.size())) {
                            g_currentBookmarkIndex = 0; // Wrap around to the start
                        }
                    } else { // Previous
                        g_currentBookmarkIndex--;
                        if (g_currentBookmarkIndex < 0) {
                            g_currentBookmarkIndex = static_cast<int>(anchors.size()) - 1; // Wrap around to the end
                        }
                    }

                    std::string targetAnchor = anchors[g_currentBookmarkIndex];

                    RE::FxResponseArgs<1> gotoArgs;
                    gotoArgs.Add(targetAnchor.c_str());
                    RE::FxDelegate::Invoke(movieView, "GotoPageByAnchor", gotoArgs);

                    return RE::BSEventNotifyControl::kStop;
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}