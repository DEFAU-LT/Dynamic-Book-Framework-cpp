//API.h
#pragma once
#include <cstdint>

namespace DynamicBookFramework_API
{
	// Define constants for the API
	constexpr auto FrameworkPluginName = "DynamicBookFramework"; // The name of your plugin's DLL
	constexpr auto InterfaceVersion = 1;

	// Define unique message types. Using FourCC codes is a good practice to avoid conflicts.
	enum APIMessageType : std::uint32_t
	{
		kAppendEntry = 'DBFA' // 'D' 'B' 'F' 'A' for Dynamic Book Framework Append
	};

	// Define the data structure for the message.
	// This must be a simple C-style struct to ensure a stable memory layout between different DLLs.
	// Do NOT use std::string or other complex C++ classes here.
	struct AppendEntryMessage
	{
		const char* bookTitleKey;   // The title of the book, which acts as the file key
		const char* textToAppend;   // The new entry text to add
	};

} // namespace DynamicBookFramework_API
