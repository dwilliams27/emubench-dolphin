#include "IPC/HTTPServer.h"

#include <iostream>
#include <future>

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
	m_server.stop();
	
	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
	
	m_thread.reset();
}

void HTTPServer::ServerThread(int port) {
	m_server.Get("/", [](const httplib::Request& req, httplib::Response& res) {
		res.set_content("Hello World from Dolphin IPC Server!", "text/plain");
	});

  m_server.Get("/api/screenshot", [](const httplib::Request& req, httplib::Response& res) {
		NOTICE_LOG_FMT(CORE, "IPC: Screenshot request received");
		u32 rand_id = Common::Random::GenerateValue<u32>();
		const std::string screenshot_name = fmt::format("ipc_screenshot_{:08x}", rand_id);
		NOTICE_LOG_FMT(CORE, "IPC: Screenshot name: {}", screenshot_name);
		
		// Use a future/promise to wait for the job to complete
		std::promise<void> promise;
		std::future<void> future = promise.get_future();
		Common::Event* completion_event_ptr = nullptr;
		
		// Queue the screenshot operation on the Host thread
		Core::QueueHostJob([screenshot_name, &promise, &completion_event_ptr](Core::System& system) {
			completion_event_ptr = &Core::SaveScreenShotWithCallback(screenshot_name);
			promise.set_value();
		}, true);

		future.get();
		completion_event_ptr->Wait();
		
		NOTICE_LOG_FMT(CORE, "IPC: Screenshot done signal");
		std::string contents;
		if (File::ReadFileToString(fmt::format("{}{}.png", Core::GenerateScreenshotFolderPath(), screenshot_name), contents))
		{
			res.set_content(contents, "image/png");
		} else {
			res.status = 500;
			res.set_content("Failed to read screenshot", "text/plain");
		}
	});
	
	m_server.Post("/api/controller/:port", [this](const httplib::Request& req, httplib::Response& res) {
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
		std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(req.body);
		if (!json_data) {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
      return;
		}

		// Convert to ControllerInput structure
		IPCControllerInput input = ParseIPCControllerInput(json_data);

		// Convert to GCPadStatus and update Dolphin
		GCPadStatus status = ConvertToGCPadStatus(input);
		if (json_data->contains("frames") && (*json_data)["frames"].is_number_unsigned()) {
			uint32_t frame_count = (*json_data)["frames"].get<uint32_t>();
			if (frame_count > 0) {
        // Queue the input to be active for a specific number of frames
        Pad::QueueTimedInput(port, status, frame_count);
        NOTICE_LOG_FMT(CORE, "IPC: Queued timed input for pad {} for {} frames", port, frame_count);
			} else {
        res.status = 400;
				res.set_content("{\"error\":\"Invalid frames parameter; must be >0\"}", "application/json");
				return;
			}
		} else {
      // No 'frames' parameter, apply as persistent state
      Pad::UpdateControllerStateFromHTTP(port, status);
      NOTICE_LOG_FMT(CORE, "IPC: Applying persistent update for pad {}", port);
		}

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	// Set up watches on all of the passed memory locs
	m_server.Post("/api/memwatch", [this](const httplib::Request& req, httplib::Response& res) {
		// Parse JSON body
		std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(req.body);
		if (!json_data) {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
      return;
		}

		if (json_data->contains("addresses") && (*json_data)["addresses"].is_array()) {
			for (auto& address : (*json_data)["addresses"]) {
        IPC::MemWatch::GetInstance().WatchAddress(address);
    	}
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON value: addresses must be string[]\"}", "application/json");
      return;
		}

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	// Gets value of an active memwatch
	m_server.Get("/api/memwatch", [](const httplib::Request& req, httplib::Response& res) {
		if (req.has_param("addresses")) {
			std::string addresses_param = req.get_param_value("addresses");
        
			// Parse the addresses (assuming comma-separated values)
			std::vector<std::string> addresses;
			std::stringstream ss(addresses_param);
			std::string address;
			
			while (getline(ss, address, ',')) {
					addresses.push_back(address);
			}
			
			std::vector<u32> results;
			for (auto& addr : addresses) {
					std::optional<u32> value = IPC::MemWatch::GetInstance().FetchDmcpValue(addr);
					if (value) {
							results.push_back(*value);
					} else {
							res.status = 400;
							res.set_content("{\"error\":\"Couldn't read value of an address\"}", "application/json");
							return;
					}
			}
			
			nlohmann::json response = {{"values", results}};
			res.set_content(response.dump(), "application/json");
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Must pass 'addresses' query param\"}", "application/json");
      return;
		}
	});

	m_server.Post("/api/savestate/:slot", [](const httplib::Request& req, httplib::Response& res) {
		const std::string& slot_str = req.path_params.at("slot");
		bool valid_number = true;
		int slot = 0;

		for (char c : slot_str) {
			if (!std::isdigit(c)) {
				valid_number = false;
				break;
			}
		}

		if (valid_number && !slot_str.empty()) {
			slot = std::atoi(slot_str.c_str());
			
			// Check if slot is in valid range (0-99)
			if (slot < 0 || slot > 99) {
				res.status = 400;
				res.set_content("{\"error\":\"Invalid slot. Must be 0-99\"}", "application/json");
				return;
			}
		} else {
			res.status = 400;
			res.set_content("{\"error\":\"Invalid slot parameter\"}", "application/json");
			return;
		}

		IPC::SaveState::GetInstance().SaveToSlot(slot);
		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	m_server.Get("/api/savestate/:slot", [](const httplib::Request& req, httplib::Response& res) {
		const std::string& slot_str = req.path_params.at("slot");
		bool valid_number = true;
		int slot = 0;

		for (char c : slot_str) {
			if (!std::isdigit(c)) {
				valid_number = false;
				break;
			}
		}

		if (valid_number && !slot_str.empty()) {
			slot = std::atoi(slot_str.c_str());
			
			// Check if slot is in valid range (0-99)
			if (slot < 0 || slot > 99) {
				res.status = 400;
				res.set_content("{\"error\":\"Invalid slot. Must be 0-99\"}", "application/json");
				return;
			}
		} else {
			res.status = 400;
			res.set_content("{\"error\":\"Invalid slot parameter\"}", "application/json");
			return;
		}

		IPC::SaveState::GetInstance().LoadFromSlot(slot);
		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	m_server.Post("/api/config/emuspeed", [this](const httplib::Request& req, httplib::Response& res) {
		// Parse JSON body
		std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(req.body);
		if (!json_data) {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
      return;
		}

		if (json_data->contains("speed") && (*json_data)["speed"].is_number()) {
			Config::SetCurrent(Config::MAIN_EMULATION_SPEED, (*json_data)["speed"].get<float>());
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON value: addresses must be string[]\"}", "application/json");
      return;
		}

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});
    
	// Use a timeout for listen to allow proper shutdown
	m_server.set_read_timeout(1); // 1 second
	m_server.set_write_timeout(1); // 1 second

	NOTICE_LOG_FMT(CORE, "Starting IPC server on port {}", port);
	
	// Error handler for listen failures
	m_server.set_error_handler([](const httplib::Request&, httplib::Response&) {
		NOTICE_LOG_FMT(CORE, "IPC Server Error in request handling");
		return false; // Continue handling errors
	});
	
	// Set exception handler to use a custom error handler instead of throwing
	m_server.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
		NOTICE_LOG_FMT(CORE, "IPC Server Exception");
		res.status = 500;
		res.set_content("Internal Server Error", "text/plain");
	});
	
	m_server.listen("127.0.0.1", port);
	
	// The server has stopped
	NOTICE_LOG_FMT(CORE, "IPC server stopped");
	m_running = false;
}

std::optional<nlohmann::json_abi_v3_12_0::json> HTTPServer::ParseJson(std::__1::string rawBody) {
	nlohmann::json json_data;
	bool json_parse_success = false;

	json_parse_success = nlohmann::json::accept(rawBody);
	NOTICE_LOG_FMT(CORE, "IPC: Parsing request {}", rawBody);

	if (!json_parse_success) {
		return std::nullopt;
	}
	
	return nlohmann::json::parse(rawBody);
}

} // namespace IPC
