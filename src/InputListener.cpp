#include "InputListener.h"
#include "ImGuiMenu.h" // We need this to call ToggleMenu()
#include "BookUIManager.h"
#include "Settings.h"

InputListener* InputListener::GetSingleton()
{
    static InputListener singleton;
    return &singleton;
}

void InputListener::Install()
{
    auto* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(InputListener::GetSingleton());
        SKSE::log::info("Registered input event listener.");
    }
}

RE::BSEventNotifyControl InputListener::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
{
    if (!a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (Settings::isWaitingForHotkey) {
        for (RE::InputEvent* event = *a_event; event; event = event->next) {
            if (event->eventType.get() == RE::INPUT_EVENT_TYPE::kButton) {
                const auto* buttonEvent = event->AsButtonEvent();
                if (buttonEvent->IsDown() && buttonEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
                    const auto key = buttonEvent->GetIDCode();
                    // Don't allow Escape (0x01) or unassigned keys (0) as the hotkey
                    if (key != 0 && key != 0x01) {
                        Settings::openMenuHotkey = key;
                    }
                    // We're done waiting, whether a key was set or not (e.g., user pressed Esc)
                    Settings::isWaitingForHotkey = false;
                    // Stop the key press from doing anything else in the game
                    return RE::BSEventNotifyControl::kStop;
                }
            }
        }
    }

    constexpr uint32_t escapeKey = 27;      // ESC

    for (RE::InputEvent* event = *a_event; event; event = event->next) {
        if (event->eventType.get() != RE::INPUT_EVENT_TYPE::kButton)
            continue;

        const auto* buttonEvent = event->AsButtonEvent();
        if (buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard)
            continue;

        const auto key = buttonEvent->GetIDCode();

        // ESC always closes the menu
        if (key == escapeKey && buttonEvent->IsDown()) {
            if (ImGuiRender::EditorWindow && ImGuiRender::EditorWindow->IsOpen) {
                ImGuiRender::EditorWindow->IsOpen = false;
                return RE::BSEventNotifyControl::kStop;
            }
        }

        // Use the dynamic key from settings to toggle the menu
        if (key == static_cast<uint32_t>(Settings::openMenuHotkey) && buttonEvent->IsDown()) {
            ImGuiRender::ToggleMenu();
            return RE::BSEventNotifyControl::kStop;
        }

        constexpr uint32_t leftControlKey = 0x1D; // DirectX Scancode for Left Control
        static bool g_isControlHeld = false;
        if (key == leftControlKey) {
            // If the key is now down, AND it was NOT down before...
            if (buttonEvent->IsDown() && !g_isControlHeld) {
                
                // Mark it as held to prevent spamming on subsequent frames.
                g_isControlHeld = true;

                // Now, send our single event.
                auto* ui = RE::UI::GetSingleton();
                if (ui && ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
                    RE::GPtr<RE::IMenu> menu = ui->GetMenu(RE::BookMenu::MENU_NAME);
                    if (menu && menu->uiMovie) {
                        SKSE::ModCallbackEvent modEvent{ "DBF_onToggleInputMode", "", 0.0f, nullptr };
                        auto* modEventSource = SKSE::GetModCallbackEventSource();
                        if (modEventSource) {
                            modEventSource->SendEvent(&modEvent);
                            SKSE::log::info("InputListener: Sent ONE 'DBF_onToggleInputMode' event to SWF.");
                        }
                    }
                }
            }
            // If the key is now up, reset our state tracker.
            else if (buttonEvent->IsUp()) {
                g_isControlHeld = false;
            }
            // Stop the event either way to prevent the game from using the Ctrl key.
            return RE::BSEventNotifyControl::kStop;
        }
    }

    // If the menu is open, block all other input
    if (ImGuiRender::EditorWindow && ImGuiRender::EditorWindow->IsOpen) {
        return RE::BSEventNotifyControl::kStop;
    }

    return RE::BSEventNotifyControl::kContinue;
}