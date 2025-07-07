// setbooktext_hook.h
#pragma once
#include "PCH.h"

<<<<<<< Updated upstream
namespace SetBookTextHook {
    // Call this function from your main InstallHooks() to set up the SetBookText hook.
    extern void (*g_rawOriginalThunkPtr)(RE::GFxMovieView* rawMovieView, const char* funcName, RE::FxResponseArgsBase* args, std::uintptr_t unknownV10);
    bool Install();
}
=======
namespace BookHooks {

	extern std::string g_jumpToBookmarkAnchor;
	// The public function to install our hook.
	bool Install();

} // namespace BookHooks

// namespace SetBookTextHook {

//     inline RE::TESObjectBOOK* g_currentlyOpeningBook = nullptr;

//     // Call this function from your main InstallHooks() to set up the SetBookText hook.
//     extern void (*g_rawOriginalThunkPtr)(RE::GFxMovieView* rawMovieView, const char* funcName, RE::FxResponseArgsBase* args, std::uintptr_t unknownV10);
//     bool Install_a();
//     bool Install_b();
// }

// namespace RE
// {
// 	class TESObjectBOOK;
// }

// namespace BookHooks {

// 	// This global pointer is set by the BookMenuWatcher event and read by our hook.
// 	// We define it here as 'inline' so it can be included in multiple files
// 	// without causing a "multiple definition" linker error.
// 	inline RE::TESObjectBOOK* g_bookToOpen = nullptr;

// 	// Declares the function that will install our hook.
// 	// The implementation is in the .cpp file.
// 	bool Install();

// } // namespace BookHooks

// namespace ActivateBookHook {
//     void Install();
// }


// namespace GetBookTextHook {
//     extern RE::FormID g_currentlyOpeningBookID;
//     bool Install();
// }


// namespace BookHooks {

//     using _BookDisplayFunction = int64_t (*)(RE::BookMenu*);
//     void PrepareAndInjectBookData(RE::BookMenu* bookMenu);

//     bool Install();
//     void CallOriginalBookDisplay(RE::BookMenu* bookMenu);

//     extern const char** g_bookTextSourcePtr;
// }

// namespace RE
// {
// 	class BookMenu; // Forward-declare
// }

// namespace BookHooks {

// 	// Function pointer type for our hook target
// 	using _BookDisplayFunction = int64_t (*)(RE::BookMenu*);

// 	// Use 'extern' to DECLARE that this pointer exists. The definition is in the .cpp file.
// 	extern const char** g_bookTextSourcePtr;

// 	// The public function to install the hook
// 	bool Install();

// } // namespace BookHooks
>>>>>>> Stashed changes
