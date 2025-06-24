#pragma once

// This is the header you provided, which is all we need.
#include "SKSEMenuFramework.h"


namespace ImGuiRender {

    void InitializeSKSEMenuFramework();
    
    /**
     * @brief Registers the render function with the SKSE Menu Framework.
     */
    void Register();

    /**
     * @brief Toggles the visibility of the editor window.
     */
    void ToggleMenu();

    /**
     * @brief Closes the editor window.
     */
    void CloseMenu();

    static bool KeyReleased = false;

    void __stdcall RenderEditorWindow();

    // A handle to our created window, allowing us to control its state.
    //inline SKSEMenuFramework::Model::WindowInterface* EditorWindow;

    inline MENU_WINDOW EditorWindow;

} // namespace ImGuiRender