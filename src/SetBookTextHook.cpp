//setbooktexthook.cpp
#include "SetBookTextHook.h" // For the Install() declaration
#include "BookMenuWatcher.h"
#include "BookMenuWatcher.h"
#include "Utility.h"
#include "PCH.h"
<<<<<<< Updated upstream
=======
// #include "RE/B/BSScaleformExternalTexture.h"
// #include "RE/B/BSTArray.h"
>>>>>>> Stashed changes


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

    // static SetBookTextThunk_t g_rawOriginalThunkPtr = nullptr;
    void (*g_rawOriginalThunkPtr)(RE::GFxMovieView*, const char*, RE::FxResponseArgsBase*, std::uintptr_t) = nullptr;

<<<<<<< Updated upstream
    
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

                auto& textMap = DynamicBookFramework::BookMenuWatcher::GetSingleton()->dynamicBookTexts;
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
=======
	auto* watcher = DynamicBookFramework::BookMenuWatcher::GetSingleton();
	
	RE::GFxValue* originalArgs = nullptr;
	a_args.GetValues(&originalArgs); 
	if (!originalArgs) {
		return g_original_Invoke(a_view, a_funcName, a_args);
	}
	
	const char* vanillaText = originalArgs[1].GetString();
	if (vanillaText) {
		watcher->CacheVanillaText(bookForm->GetName(), vanillaText);
	}
	
	std::string rawBookText = watcher->GetFullDynamicTextForBook(bookForm);
	if (rawBookText.empty()) {
		return g_original_Invoke(a_view, a_funcName, a_args);
	}
	
	// --- FINAL, SAFE IMAGE LOADING ---

	std::vector<std::string> imagePaths = ExtractImagePathsFromText(rawBookText);
	auto& textureArray = REL::RelocateMember<RE::BSTArray<RE::BSScaleformExternalTexture>>(bookMenu, 0x50, 0x60);
	
	// DO NOT CLEAR THE ARRAY. Only add textures that are new.
	for (const auto& path : imagePaths) {
		std::string fullImgPath = "img://" + path;
		bool alreadyExists = false;
		for (const auto& existingTexture : textureArray) {
			if (existingTexture.filePath == fullImgPath.c_str()) {
				alreadyExists = true;
				break;
			}
		}

		if (!alreadyExists) {
			auto& newTexture = textureArray.emplace_back();
			RE::BSFixedString bsPath(path);
			if (newTexture.LoadPNG(bsPath) && newTexture.gamebryoTexture) {
				newTexture.filePath = fullImgPath;
			}
		}
	}

	std::string finalHtml = HtmlFormatText::ApplyGeneralBookMarkup_ProcessChunk(rawBookText);
	originalArgs[1].SetString(finalHtml.c_str());
	
	g_original_Invoke(a_view, a_funcName, a_args);

    RE::FxResponseArgs<1> titleArg;
	titleArg.Add(bookForm->GetName());
	RE::FxDelegate::Invoke(a_view, "SetCurrentBookTitle", titleArg);

	RE::FxResponseArgs<2> infoArgs;
    infoArgs.Add(bookForm->GetFormID());
    infoArgs.Add(bookForm->GetName());
    RE::FxDelegate::Invoke2(a_view, "SetBookInfo", infoArgs);
}


	bool Install() {
		// This hooks the call to FxDelegate::Invoke inside the game code.
		REL::ID functionWithCall_ID;
		uintptr_t offsetToThunkCall;
		const auto& runtime = REL::Module::get();

		if (runtime.IsAE()) {
			functionWithCall_ID = REL::ID(51054);
			offsetToThunkCall = 0x318;
		} else if (runtime.IsSE()) {
			functionWithCall_ID = REL::ID(50123);
			offsetToThunkCall = 0x314;
		} else {
			return false;
		}

		uintptr_t callInstructionAddress = functionWithCall_ID.address() + offsetToThunkCall;
		
		auto& trampoline = SKSE::GetTrampoline();
		g_original_Invoke = trampoline.write_call<5>(callInstructionAddress, reinterpret_cast<uintptr_t>(Detour_SetBookText));

		if (g_original_Invoke.get()) {
			logger::info("Final SetBookText hook installed successfully.");
			return true;
		}
		
		logger::error("Failed to install SetBookText hook.");
		return false;
	}
}
>>>>>>> Stashed changes
