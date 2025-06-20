#include "InputListener.h"
#include "ImGuiMenu.h" // We need this to call ToggleMenu()

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

    constexpr uint32_t toggleKey = 0x44;     // F10
    constexpr uint32_t escapeKey = 27;       // ESC

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

        // F10 toggles the menu
        if (key == toggleKey && buttonEvent->IsDown()) {
            
            ImGuiRender::ToggleMenu();
            return RE::BSEventNotifyControl::kStop;
        }
    }

    // If the menu is open, block all other input
    if (ImGuiRender::EditorWindow && ImGuiRender::EditorWindow->IsOpen) {
        return RE::BSEventNotifyControl::kStop;
    }

    return RE::BSEventNotifyControl::kContinue;
}