// HTTPServer.h
#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <optional>
#include <iostream>
#include <future>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
#include "httplib.h"

#include "Common/Config/Config.h"
#include "Core/Config/MainSettings.h"
#include "Core/Boot/Boot.h"
#include "Core/BootManager.h"
#include "Common/Event.h"
#include "Common/FileUtil.h"
#include "Common/IOFile.h"
#include "Common/Logging/Log.h"
#include "Common/Random.h"
#include "Common/WindowSystemInfo.h"
#include "Core/HW/GCPad.h"
#include "Core/Core.h"
#include "DolphinQt/MainWindow.h"
#include "HTTPServer.h"
#include "InputCommon/GCPadStatus.h"
#include "IPC/ControllerCommands.h"
#include "IPC/MemWatcher.h"
#include "IPC/SaveState.h"

namespace IPC {

class HTTPServer {
public:
    // Singleton pattern
    static HTTPServer& GetInstance(MainWindow& win);
    static HTTPServer& GetInstance();
    
    // Delete copy constructor and assignment operator
    HTTPServer(const HTTPServer&) = delete;
    HTTPServer& operator=(const HTTPServer&) = delete;
    
    // Start/stop the server
    bool Start(int port = 8080);
    void Stop();
    
    // Check if server is running
    bool IsRunning() const { return m_running; }

private:
    explicit HTTPServer(MainWindow* window = nullptr);
    ~HTTPServer();
    httplib::Server m_server;
    std::optional<MainWindow*> m_window;
    
    // Server implementation
    void ServerThread(int port);
    
    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_thread;
    std::optional<nlohmann::json_abi_v3_12_0::json> ParseJson(std::string rawBody);

    std::map<std::string, std::string> ReadMemWatches(std::vector<std::string> watch_names);
    std::vector<std::string> SetupMemWatchesFromJSON(const nlohmann::json_abi_v3_12_0::json& json_data);
    void SetupTest();

    std::map<std::string, std::string> m_initial_watches;
};

} // namespace IPC
