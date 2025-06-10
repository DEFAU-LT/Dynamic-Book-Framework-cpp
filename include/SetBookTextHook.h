// setbooktext_hook.h
#pragma once
#include "PCH.h"

namespace SetBookTextHook {
    // Call this function from your main InstallHooks() to set up the SetBookText hook.
    extern void (*g_rawOriginalThunkPtr)(RE::GFxMovieView* rawMovieView, const char* funcName, RE::FxResponseArgsBase* args, std::uintptr_t unknownV10);
    bool Install();
}