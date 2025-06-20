#pragma once
#include "PCH.h"

class InputListener : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    // Gets the singleton instance of the listener.
    static InputListener* GetSingleton();

    // Registers this class to receive input events from the game.
    static void Install();

    // This is the main callback that will be called by the game for every input event.
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

private:
    InputListener() = default;
    ~InputListener() override = default;
    InputListener(const InputListener&) = delete;
    InputListener& operator=(const InputListener&) = delete;
};
