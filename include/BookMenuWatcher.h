
#pragma once

#include "PCH.h"

class BookMenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    static BookMenuWatcher* GetSingleton()
    {
        static BookMenuWatcher singleton; // Create the single static instance the first time this is called
        return &singleton; // Return the address of that single instance
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
    // Static member to store dynamic book texts, keyed by FormID
    static std::map<RE::FormID, std::string> dynamicBookTexts;
private:
    // Private constructor to ensure singleton pattern
    BookMenuWatcher() = default;
    // Delete copy and move constructors/assignment operators for singleton
    BookMenuWatcher(const BookMenuWatcher&) = delete;
    BookMenuWatcher(BookMenuWatcher&&) = delete;
    BookMenuWatcher& operator=(const BookMenuWatcher&) = delete;
    BookMenuWatcher& operator=(BookMenuWatcher&&) = delete;
    
};

