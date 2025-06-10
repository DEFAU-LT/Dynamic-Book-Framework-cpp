#include "FileWatcher.h"
#include "BookUIManager.h" // To call RefreshCurrentlyOpenBook()
#include "Utility.h"       // For logger alias
#include "PCH.h"           // For common headers

namespace DynamicBookFramework { // Using your project-wide namespace
    namespace FileWatcher {

        namespace { // Anonymous namespace for internal variables
            std::thread g_watcherThread;
            std::atomic<bool> g_stopWatcherFlag(false);

            struct WatchedFileInfo {
                std::filesystem::path path;
                std::filesystem::file_time_type lastWriteTime;
            };

            std::map<std::string, WatchedFileInfo> g_monitoredFiles;
            std::mutex g_monitoredFilesMutex;
        }

        // The RefreshBookTask class is no longer needed with the std::function approach

        void WatchFilesLoop() {
            while (!g_stopWatcherFlag.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                if (g_stopWatcherFlag.load()) break;

                std::map<std::string, WatchedFileInfo> filesToCheck;
                {
                    std::lock_guard<std::mutex> lock(g_monitoredFilesMutex);
                    filesToCheck = g_monitoredFiles;
                }

                if (filesToCheck.empty()) {
                    continue;
                }
                
                for (auto& pair : filesToCheck) {
                    const std::string& bookTitle = pair.first;
                    const WatchedFileInfo& fileInfo = pair.second;

                    try {
                        if (std::filesystem::exists(fileInfo.path)) {
                            auto currentWriteTime = std::filesystem::last_write_time(fileInfo.path);

                            if (currentWriteTime > fileInfo.lastWriteTime) {
                                logger::info("FileWatcher: Detected change in '{}'.", wstring_to_utf8(fileInfo.path.wstring()).c_str());
                                {
                                    std::lock_guard<std::mutex> lock(g_monitoredFilesMutex);
                                    if (g_monitoredFiles.count(bookTitle)) {
                                        g_monitoredFiles[bookTitle].lastWriteTime = currentWriteTime;
                                    }
                                }
                                
                                // --- FIX: Use the std::function overload of AddTask with a lambda ---
                                if (auto* taskInterface = SKSE::GetTaskInterface()) {
                                    taskInterface->AddTask([]() {
                                        // This code will be executed on the main game thread
                                        logger::info("FileWatcher Task: Running RefreshCurrentlyOpenBook() on main thread via lambda.");
                                        BookUIManager::RefreshCurrentlyOpenBook();
                                    });
                                }
                            }
                        }
                    } catch (const std::filesystem::filesystem_error& e) {
                        logger::error("FileWatcher: Filesystem error while checking '{}': {}", wstring_to_utf8(fileInfo.path.wstring()).c_str(), e.what());
                    }
                }
            }
            logger::info("FileWatcher: Watcher thread loop has finished.");
        }

        void Start() {
            if (g_watcherThread.joinable()) {
                logger::warn("FileWatcher: Start() called, but watcher thread is already running.");
                return;
            }
            logger::info("FileWatcher: Starting watcher thread...");
            g_stopWatcherFlag.store(false);
            g_watcherThread = std::thread(WatchFilesLoop);
        }

        void Stop() {
            if (g_watcherThread.joinable()) {
                logger::info("FileWatcher: Stopping watcher thread...");
                g_stopWatcherFlag.store(true);
                g_watcherThread.join(); 
                logger::info("FileWatcher: Watcher thread stopped.");
            }
        }

        void MonitorBookFile(const std::string& bookTitle, const std::filesystem::path& filePath) {
            std::lock_guard<std::mutex> lock(g_monitoredFilesMutex);
            try {
                if (std::filesystem::exists(filePath)) {
                    WatchedFileInfo info;
                    info.path = filePath;
                    info.lastWriteTime = std::filesystem::last_write_time(filePath);
                    g_monitoredFiles[bookTitle] = info;
                    logger::info("FileWatcher: Now monitoring '{}' for changes.", wstring_to_utf8(filePath.wstring()).c_str());
                } else {
                    logger::warn("FileWatcher: Cannot monitor file '{}' because it does not exist.", wstring_to_utf8(filePath.wstring()).c_str());
                }
            } catch (const std::filesystem::filesystem_error& e) {
                logger::error("FileWatcher: Filesystem error accessing '{}': {}", wstring_to_utf8(filePath.wstring()).c_str(), e.what());
            }
        }

        void StopMonitoringBookFile(const std::string& bookTitle) {
            std::lock_guard<std::mutex> lock(g_monitoredFilesMutex);
            if (g_monitoredFiles.erase(bookTitle) > 0) {
                logger::info("FileWatcher: Stopped monitoring book '{}'.", bookTitle);
            }
        }

    } // namespace FileWatcher
} // namespace DynamicBookFramework
