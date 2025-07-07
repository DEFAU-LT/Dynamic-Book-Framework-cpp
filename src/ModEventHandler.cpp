#include "ModEventHandler.h"
#include "Settings.h"
#include "utility.h"
#include "PCH.h"


ModEventHandler* ModEventHandler::GetSingleton()
{
	static ModEventHandler singleton;
	return &singleton;
}

void ModEventHandler::Register()
{
	// FIX: Use the correct function from your SKSE/API.h header
	auto* eventSource = SKSE::GetModCallbackEventSource(); //
	if (eventSource) {
		eventSource->AddEventSink(GetSingleton());
		logger::info("Registered custom mod event listener.");
	} else {
		logger::error("Could not get mod callback event source.");
	}
}

void ModEventHandler::Unregister()
{
	// FIX: Use the correct function from your SKSE/API.h header
	auto* eventSource = SKSE::GetModCallbackEventSource(); //
	if (eventSource) {
		eventSource->RemoveEventSink(GetSingleton());
		logger::info("Unregistered custom mod event listener.");
	}
}

RE::BSEventNotifyControl ModEventHandler::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
{
    if (!a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (_stricmp(a_event->eventName.c_str(), "DBF_TextInputEvent") == 0) {
		logger::info("Received text from UI input field: '{}'", a_event->strArg.c_str());
		return RE::BSEventNotifyControl::kStop;
	} else if (_stricmp(a_event->eventName.c_str(), "DBF_DebugLogEvent") == 0) {
		// Print the message from ActionScript to our C++ log file.
		logger::info("[AS2_DEBUG] {}", a_event->strArg.c_str());
		// We use kContinue because this is just a debug message.
		return RE::BSEventNotifyControl::kContinue;
	}

    return RE::BSEventNotifyControl::kContinue;
}