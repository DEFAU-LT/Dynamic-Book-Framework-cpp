//setbooktexthook.cpp
#include "SetBookTextHook.h" // For the Install() declaration
#include "BookMenuWatcher.h"
#include "Utility.h"
#include "PCH.h"


// --- Namespace alias for convenience ---
namespace logger = SKSE::log;

namespace SetBookTextHook {

    // ... (constexpr IDs, using SetBookTextThunk_t with RE::GFxMovieView*, static g_rawOriginalThunkPtr) ...
    constexpr REL::ID FunctionWithCall_ID(51054); 
    constexpr uintptr_t OffsetToThunkCall = 0x318; 

    // CORRECTED Thunk Signature: Takes a raw RE::GFxMovieView*
    using SetBookTextThunk_t = void (*)(
        RE::GFxMovieView* rawMovieView, // << CHANGED
        const char* funcName, 
        RE::FxResponseArgsBase* args,
        std::uintptr_t unknownV10 
    );

    static SetBookTextThunk_t g_rawOriginalThunkPtr = nullptr;

    
    void Detour_SetBookTextThunk(
        RE::GFxMovieView* rawMovieView_param, 
        const char* funcName, 
        RE::FxResponseArgsBase* args,
        std::uintptr_t v10_param 
    ) {
        //logger::trace("SetBookTextThunk hooked (4-arg, raw ptr). Func: '{}', RawMovieView: {:X}, Args: {:X}, V10_Param: 0x{:X}", 
                    //funcName, reinterpret_cast<uintptr_t>(rawMovieView_param), 
                    //args ? reinterpret_cast<uintptr_t>(args) : 0, v10_param);

        if (rawMovieView_param == nullptr) {
            logger::critical("CRITICAL: rawMovieView_param is NULL on entry to detour!");
        }

        if (args && strcmp(funcName, "SetBookText") == 0) {
            RE::TESObjectBOOK* currentBookForDisplay = RE::BookMenu::GetTargetForm(); // Try to get current book

            if (currentBookForDisplay) {
                RE::FormID currentFormID = currentBookForDisplay->GetFormID();
                std::string currentTitle = currentBookForDisplay->GetFullName() ? currentBookForDisplay->GetFullName() : "";
                logger::info("SetBookTextHook: Intercepted SetBookText for '{}' (FormID {:X})", currentTitle, currentFormID);

                auto& textMap = BookMenuWatcher::GetSingleton()->dynamicBookTexts;
                auto it = textMap.find(currentFormID);

                if (it != textMap.end()) { // This IS one of our dynamic books
                    const std::string& customText = it->second;
                    logger::info("SetBookTextHook: Found custom text for FormID {:X}. Content length: {}", currentFormID, customText.length());
                    // Log a snippet of the custom text for verification:
                    //logger::trace("SetBookTextHook: Custom text snippet: {:.100}", customText);


                    auto* concreteArgs = static_cast<RE::FxResponseArgsEx<2>*>(args);
                    if (concreteArgs && concreteArgs->size() >= 2) {
                        RE::GFxValue& bookTextValue = (*concreteArgs)[0]; 
                        // RE::GFxValue& isNoteValue = (*concreteArgs)[1]; // Original abNote flag

                        // if (bookTextValue.IsString()) { // Check for narrow string first
                        // const char* originalNarrowText = bookTextValue.GetString();
                        // logger::info("Original Book Text (Narrow): \n---\n{}\n---", originalNarrowText ? originalNarrowText : "[[NULL NARROW STRING]]");
                        // } else if (bookTextValue.IsStringW()) { // Check for wide string
                        //     const wchar_t* originalWideText = bookTextValue.GetStringW();
                        //     if (originalWideText) {
                        //         // Convert wide string to UTF-8 for logging with spdlog/fmt
                        //         std::string originalUTF8Text = wstring_to_utf8(originalWideText); // Assuming you have this in Utility
                        //         logger::info("Original Book Text (Wide, UTF-8): \n---\n{}\n---", originalUTF8Text);
                        //     } else {
                        //         logger::info("Original Book Text (Wide): [[NULL WIDE STRING]]");
                        //     }
                        // } else {
                        //     logger::warn("Original book text (astrText) is not a recognized string type. Type: {}", static_cast<int>(bookTextValue.GetType()));
                        // }

                        if (bookTextValue.IsString() || bookTextValue.IsStringW()) {
                            // Ensure customText is not empty, otherwise Scaleform might show nothing
                            if (!customText.empty()) {
                                bookTextValue.SetString(customText.c_str());
                                //logger::info("Book text (astrText) MODIFIED with dynamic content for FormID {:X}.", currentFormID);
                            } else {
                                logger::warn("Dynamic text for FormID {:X} is empty. Setting a placeholder to avoid issues.", currentFormID);
                                bookTextValue.SetString("<p> </p>"); // Minimal valid HTML to clear/show empty
                            }
                        } else {
                            logger::warn("Book text argument (astrText) is not a string type as expected for FormID {:X}. Cannot modify.", currentFormID);
                        }
                    } else {
                        logger::warn("FxResponseArgs for SetBookText has unexpected argument count for FormID {:X}. Cannot modify.", currentFormID);
                    }
                } else {
                    //auto* concreteArgs = static_cast<RE::FxResponseArgsEx<2>*>(args);
                    //RE::GFxValue& bookTextValue = (*concreteArgs)[0]; 
                    //const char* originalNarrowText = bookTextValue.GetString();
                    // Not one of our dynamic books - do nothing, let original text pass through.
                    logger::info("SetBookTextHook: No dynamic text in map for FormID {:X} ('{}'). Original text will be used.", currentFormID, currentTitle);
                    //logger::info("Original Book Text (Narrow): \n---\n{}\n---", originalNarrowText ? originalNarrowText : "[[NULL NARROW STRING]]");
                }
            } else {
                logger::warn("SetBookTextHook: Could not get target form from BookMenu inside detour. Original text will be used.");
            }
        } else if (args) {
            logger::trace("SetBookTextHook: Intercepted for function: {} (not SetBookText)", funcName);
        } else {
            logger::warn("SetBookTextHook: Detour called with null FxResponseArgsBase.");
        }

        // Call the original thunk function
        if (g_rawOriginalThunkPtr) {
            g_rawOriginalThunkPtr(rawMovieView_param, funcName, args, v10_param);
        } else {
            logger::error("Original thunk (4-arg) RAW function pointer is null! Cannot call original function.");
        }
        
        //logger::trace("SetBookTextThunk: Exiting detour.");
    }

    bool Install() {
        //logger::info("Installing SetBookText hook (4-arg, raw ptr attempt)...");

        uintptr_t functionWithCallBase = FunctionWithCall_ID.address(); 
        if (!functionWithCallBase) {
            logger::critical("SetBookTextHook: Failed to find address for ID {}. Hook not installed.", FunctionWithCall_ID.id());
            return false;
        }

        uintptr_t callInstructionAddress = functionWithCallBase + OffsetToThunkCall; 
        //logger::info("SetBookTextHook: Hook target 'call' instruction address: 0x{:X}", callInstructionAddress);

        auto& trampoline = SKSE::GetTrampoline();
        
        std::uintptr_t detourFunctionAddress = reinterpret_cast<std::uintptr_t>(&Detour_SetBookTextThunk); 
        std::uintptr_t originalThunkAddressAsUintPtr = trampoline.write_call<5>(callInstructionAddress, detourFunctionAddress);
        g_rawOriginalThunkPtr = reinterpret_cast<SetBookTextThunk_t>(originalThunkAddressAsUintPtr); 
        
        if (g_rawOriginalThunkPtr) {
           // logger::info("SetBookTextHook: Successfully hooked. Original thunk (4-arg, raw ptr) RAW ptr at 0x{:X}", reinterpret_cast<uintptr_t>(g_rawOriginalThunkPtr));
            return true;
        } else {
            logger::error("SetBookTextHook: Failed to install hook at 0x{:X} (trampoline returned null or invalid address).", callInstructionAddress);
            return false;
        }
    }

} // namespace SetBookTextHook