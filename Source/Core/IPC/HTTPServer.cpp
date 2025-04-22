#include "HTTPServer.h"
#include "httplib.h"
#include <iostream>

namespace IPC {

HTTPServer& HTTPServer::GetInstance() {
    static HTTPServer instance;
    return instance;
}

HTTPServer::~HTTPServer() {
    Stop();
}

bool HTTPServer::Start(int port) {
    if (m_running)
        return false;
    
    m_running = true;
    m_thread = std::make_unique<std::thread>(&HTTPServer::ServerThread, this, port);
    
    return true;
}

void HTTPServer::Stop() {
    if (!m_running)
        return;
    
    m_running = false;
    
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }
    
    m_thread.reset();
}

void HTTPServer::ServerThread(int port) {
  httplib::Server svr;
  
  // Set up a simple "Hello World" endpoint
  svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
      res.set_content("Hello World from Dolphin IPC Server!", "text/plain");
  });
  
  // Health check endpoint
  svr.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
      res.set_content("{\"status\":\"ok\"}", "application/json");
  });
  
  // Basic info endpoint
  svr.Get("/info", [](const httplib::Request& req, httplib::Response& res) {
      res.set_content("{\"name\":\"Dolphin IPC Server\",\"version\":\"1.0.0\"}", "application/json");
  });
  
  // Use a timeout for listen to allow proper shutdown
  svr.set_read_timeout(1); // 1 second
  svr.set_write_timeout(1); // 1 second
  
  // Only accept connections from localhost
  std::cout << "Starting IPC server on port " << port << std::endl;
  
  // Error handler for listen failures
  svr.set_error_handler([](const httplib::Request&, httplib::Response&) {
    std::cerr << "IPC Server Error in request handling" << std::endl;
    return false; // Continue handling errors
  });
  
  // Set exception handler to use a custom error handler instead of throwing
  svr.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
    std::cerr << "IPC Server Exception" << std::endl;
    res.status = 500;
    res.set_content("Internal Server Error", "text/plain");
  });
  
  svr.listen("127.0.0.1", port);
  
  // The server has stopped
  std::cout << "IPC server stopped" << std::endl;
  m_running = false;
}

} // namespace IPC