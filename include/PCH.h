#pragma once

// --- Core SKSE / CommonLibSSE ---
#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"
#include <SKSE/Impl/Stubs.h>


// --- Logging (spdlog via CommonLibSSE) ---
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

// Allows us to check if a debugger is attached (optional, see below)
#include <Windows.h>
#include <filesystem>
#include <vector>
#include <thread> 
#include <chrono>
#include <shlobj.h>
#include <mutex>

// --- Formatting (Choose One - Add the include line here) ---
// #include <spdlog/fmt/fmt.h> // Option 1: Bundled {fmt} (Recommended C++17+)
#include <format>        // Option 2: Standard C++20 <format> (Requires C++20)

// --- Common Standard Library Headers ---
#include <unordered_map>
#include <memory>           // For std::shared_ptr, std::make_shared, std::move
#include <sstream>
#include <fstream>
#include <string>           // For std::string
#include <utility>          // For std::move (might be included by others)
#include <mutex> // If any globals need protection later
#include <streambuf>
#include <optional>
#include <vector>    // Potentially for more complex splitting if needed, but stringstream is fine
#include <algorithm> // For std::remove if you want to trim lines (optional)

// Add other frequently used standard headers if desired (e.g., <vector>, <map>, <algorithm>)

// --- Global using directives ---
using namespace std::literals; // Enables "sv" string view literals etc.
using namespace std::string_view_literals;


#ifdef SKYRIM_AE
#	define REL_ID(se, ae) REL::ID(ae)
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define REL_ID(se, ae) REL::ID(se)
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define REL_ID(se, ae) REL::ID(se)
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

#define DLLEXPORT __declspec(dllexport)
