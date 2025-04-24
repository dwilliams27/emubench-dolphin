#include "Common/Logging/Log.h"

#include "HTTPServer.h"
#include "httplib.h"
#include <nlohmann/json.hpp>
#include "Core/HW/GCPad.h"
#include <iostream>
#include "IPC/ControllerCommands.h"

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
	
	svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
		res.set_content("Hello World from Dolphin IPC Server!", "text/plain");
	});
	
	svr.Post("/api/controller/:port", [](const httplib::Request& req, httplib::Response& res) {
		// First check if the port is a valid number
		const std::string& port_str = req.path_params.at("port");
		bool valid_number = true;
		int port = 0;

		// Check if the string contains only digits
		for (char c : port_str) {
			if (!std::isdigit(c)) {
				valid_number = false;
				break;
			}
		}

		// If it's a valid number format, convert it safely
		if (valid_number && !port_str.empty()) {
			port = std::atoi(port_str.c_str());
			
			// Check if port is in valid range (0-3)
			if (port < 0 || port > 3) {
				res.status = 400;
				res.set_content("{\"error\":\"Invalid controller port. Must be 0-3\"}", "application/json");
				return;
			}
		} else {
			res.status = 400;
			res.set_content("{\"error\":\"Invalid port parameter\"}", "application/json");
			return;
		}
		
		// Parse JSON body
		// First check if the JSON is valid by using parse with a callback
		nlohmann::json json_data;
		bool json_parse_success = false;

		// Use nlohmann::json::parse's callback mechanism to check validity without exceptions
		nlohmann::json::parser_callback_t callback = [](int /*depth*/, nlohmann::json::parse_event_t /*event*/, 
																									nlohmann::json& /*parsed*/) {
				return true;  // Continue parsing
		};

		// Attempt to parse without throwing
		json_parse_success = nlohmann::json::accept(req.body);

		if (!json_parse_success) {
				res.status = 400;
				res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
				return;
		}

		// If parsing succeeded, actually parse the JSON
		json_data = nlohmann::json::parse(req.body);

		// Validate required fields
		if (!json_data.contains("buttons")) {
				res.status = 400;
				res.set_content("{\"error\":\"Missing required fields\"}", "application/json");
				return;
		}

		// Convert to ControllerInput structure
		IPCControllerInput input = ParseIPCControllerInput(json_data);

		// Convert to GCPadStatus and update Dolphin
		GCPadStatus status = ConvertToGCPadStatus(input);
		Pad::UpdateControllerStateFromHTTP(port, status);

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});
    
	// Use a timeout for listen to allow proper shutdown
	svr.set_read_timeout(1); // 1 second
	svr.set_write_timeout(1); // 1 second
	
	// Only accept connections from localhost
	INFO_LOG_FMT(CONSOLE, "Starting IPC server on port {}", port);
	
	// Error handler for listen failures
	svr.set_error_handler([](const httplib::Request&, httplib::Response&) {
		INFO_LOG_FMT(CONSOLE, "IPC Server Error in request handling");
		return false; // Continue handling errors
	});
	
	// Set exception handler to use a custom error handler instead of throwing
	svr.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
		INFO_LOG_FMT(CONSOLE, "IPC Server Exception");
		res.status = 500;
		res.set_content("Internal Server Error", "text/plain");
	});
	
	svr.listen("127.0.0.1", port);
	
	// The server has stopped
	INFO_LOG_FMT(CONSOLE, "IPC server stopped");
	m_running = false;
}

} // namespace IPC
