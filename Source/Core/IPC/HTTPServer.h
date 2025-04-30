// HTTPServer.h
#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include <nlohmann/json.hpp>

#include "httplib.h"
#include "IPC/MemWatch.h"

namespace IPC {

class HTTPServer {
public:
    // Singleton pattern
    static HTTPServer& GetInstance();
    
    // Delete copy constructor and assignment operator
    HTTPServer(const HTTPServer&) = delete;
    HTTPServer& operator=(const HTTPServer&) = delete;
    
    // Start/stop the server
    bool Start(int port = 58111);
    void Stop();
    
    // Check if server is running
    bool IsRunning() const { return m_running; }

private:
    HTTPServer() = default;
    ~HTTPServer();
    httplib::Server m_server;
    
    // Server implementation
    void ServerThread(int port);
    
    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_thread;
    std::optional<nlohmann::json_abi_v3_12_0::json> ParseJson(std::__1::string rawBody);
};

} // namespace IPC
