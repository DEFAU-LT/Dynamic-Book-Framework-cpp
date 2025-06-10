#pragma once
#include "PCH.h"


namespace DynamicBookFramework{ // Or your main plugin namespace

    namespace FileWatcher {
        
        // Starts the background watcher thread.
        // Call this once during plugin initialization (e.g., kDataLoaded).
        void Start();

        // Signals the background watcher thread to stop and waits for it to finish.
        // Call this during plugin shutdown (e.g., SKSEPlugin_Unload).
        void Stop();

        // Tells the watcher to begin monitoring a specific book file for changes.
        // This should be called by BookMenuWatcher when a dynamic book is opened.
        // @param bookTitle A unique identifier for the book (its title).
        // @param filePath The full, absolute path to the .txt file to monitor.
        void MonitorBookFile(const std::string& bookTitle, const std::filesystem::path& filePath);

        // Tells the watcher to stop monitoring a specific book file.
        // This should be called by BookMenuWatcher when the book menu is closed.
        // @param bookTitle The unique identifier for the book to stop watching.
        void StopMonitoringBookFile(const std::string& bookTitle);

    } // namespace FileWatcher

} // 