#pragma once
#include "PCH.h"



class ModEventHandler : public RE::BSTEventSink<SKSE::ModCallbackEvent>
{
public:
	static ModEventHandler* GetSingleton();

	// The function signature must exactly match the base class
	virtual RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource) override;

	static void Register();
	static void Unregister();

private:
	ModEventHandler() = default;
	~ModEventHandler() = default;
	ModEventHandler(const ModEventHandler&) = delete;
	ModEventHandler(ModEventHandler&&) = delete;
	ModEventHandler& operator=(const ModEventHandler&) = delete;
	ModEventHandler& operator=(ModEventHandler&&) = delete;
};