#include "IPC/HTTPServer.h"

namespace IPC {

HTTPServer& HTTPServer::GetInstance(MainWindow& win) {
    static HTTPServer instance(&win);
    return instance;
}

HTTPServer& HTTPServer::GetInstance() {
    static HTTPServer instance;
    return instance;
}

HTTPServer::HTTPServer(MainWindow* window) : m_window(std::make_optional(window)) {}

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
	HTTPServer::SetupTest();

	m_server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
		NOTICE_LOG_FMT(CORE, "IPC: Received request: {}{}", req.get_header_value("Host"), req.target);
		
		return httplib::Server::HandlerResponse::Unhandled;
	});

	m_server.Get("/", [](const httplib::Request& req, httplib::Response& res) {
		NOTICE_LOG_FMT(CORE, "IPC: Hello World request received");
		res.set_content("Hello World from Dolphin IPC Server!", "text/plain");
	});

	m_server.Get("/api/test/start-state", [](const httplib::Request& req, httplib::Response& res) {
		NOTICE_LOG_FMT(CORE, "IPC: Starting test...");
		// TODO
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

	// Gets value of an active memwatch
	m_server.Get("/api/memwatch/values", [this](const httplib::Request& req, httplib::Response& res) {
		if (req.has_param("names")) {
			std::string names_param = req.get_param_value("names");
        
			// Parse the names (assuming comma-separated values)
			std::vector<std::string> names;
			std::stringstream ss(names_param);
			std::string name;
			
			while (getline(ss, name, ',')) {
				names.push_back(name);
			}
			
			std::map<std::string, std::string> results = HTTPServer::ReadMemWatches(names);

			nlohmann::json response = {{"values", results}};
			res.set_content(response.dump(), "application/json");
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Must pass 'addresses' query param\"}", "application/json");
      return;
		}
	});

	m_server.Post("/api/emulation/state", [this](const httplib::Request& req, httplib::Response& res) {
		std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(req.body);
		if (!json_data) {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
      return;
		}

		if (json_data->contains("action") && (*json_data)["action"].is_string()) {
			if ((*json_data)["action"].get<std::string>() == "save") {
				if (!json_data->contains("to")) {
					res.status = 400;
					res.set_content("{\"error\":\"Invalid JSON value: to is required\"}", "application/json");
					return;
				}

				if ((*json_data)["to"].is_string()) {
					IPC::SaveState::GetInstance().SaveToFile((*json_data)["to"].get<std::string>());
				} else if ((*json_data)["to"].is_number_unsigned()) {
					IPC::SaveState::GetInstance().SaveToSlot((*json_data)["to"].get<uint32_t>());
				} else {
					res.status = 400;
					res.set_content("{\"error\":\"Invalid JSON value: to must be filepath or slot number\"}", "application/json");
					return;
				}
			} else if ((*json_data)["action"].get<std::string>() == "load") {
				if ((*json_data)["to"].is_string()) {
					IPC::SaveState::GetInstance().LoadFromFile((*json_data)["to"].get<std::string>());
				} else if ((*json_data)["to"].is_number_unsigned()) {
					IPC::SaveState::GetInstance().LoadFromSlot((*json_data)["to"].get<uint32_t>());
				} else {
					res.status = 400;
					res.set_content("{\"error\":\"Invalid JSON value: to must be filepath or slot number\"}", "application/json");
					return;
				}
			} else if((*json_data)["action"].get<std::string>() == "pause") {
				Core::System& system = Core::System::GetInstance();
				Core::SetState(system, Core::State::Paused);
			} else if((*json_data)["action"].get<std::string>() == "play") {
				Core::System& system = Core::System::GetInstance();
				Core::SetState(system, Core::State::Running);
			} else {
				res.status = 400;
				res.set_content("{\"error\":\"Invalid action. Must be 'save', 'load', 'play', or 'pause'\"}", "application/json");
				return;
			}
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Must pass string 'action'\"}", "application/json");
      return;
		}

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	m_server.Post("/api/emulation/config", [this](const httplib::Request& req, httplib::Response& res) {
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
      res.set_content("{\"error\":\"Invalid JSON value: speed must be a number\"}", "application/json");
      return;
		}

		res.set_content("{\"status\":\"ok\"}", "application/json");
	});

	m_server.Post("/api/emulation/boot", [this](const httplib::Request& req, httplib::Response& res) {
		std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(req.body);
		if (!json_data) {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON payload\"}", "application/json");
      return;
		}

		if (json_data->contains("game_path") && (*json_data)["game_path"].is_string()) {
			std::string rvz_path = (*json_data)["game_path"].get<std::string>();
			
			// Make sure any previous game is stopped
			Core::System& system = Core::System::GetInstance();
			Core::Stop(system);
			
			NOTICE_LOG_FMT(CORE, "IPC: Booting game {} through IPC", rvz_path);
			
			// Start the game on the main thread
			if (m_window.has_value() && *m_window) {
				QMetaObject::invokeMethod(*m_window, "StartGameOnMainThread", 
                             Qt::BlockingQueuedConnection,
                             Q_ARG(std::string, rvz_path));
			} else {
				NOTICE_LOG_FMT(CORE, "IPC: Cannot boot game, main window not available");
				res.status = 500;
				res.set_content("{\"error\":\"Main window not available\"}", "application/json");
				return;
			}
		} else {
			res.status = 400;
      res.set_content("{\"error\":\"Invalid JSON value: game_path must be string[]\"}", "application/json");
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
	
	m_server.listen("0.0.0.0", port);
	
	// The server has stopped
	NOTICE_LOG_FMT(CORE, "IPC server stopped");
	m_running = false;
}

std::optional<nlohmann::json_abi_v3_12_0::json> HTTPServer::ParseJson(std::string rawBody) {
	nlohmann::json json_data;
	bool json_parse_success = false;

	json_parse_success = nlohmann::json::accept(rawBody);
	NOTICE_LOG_FMT(CORE, "IPC: Parsing request {}", rawBody);

	if (!json_parse_success) {
		return std::nullopt;
	}
	
	return nlohmann::json::parse(rawBody);
}

std::vector<std::string> HTTPServer::SetupMemWatchesFromJSON(const nlohmann::json_abi_v3_12_0::json& json_data) {
	std::vector<std::string> names;
	for (auto& watch : json_data["watches"].items()) {
		if (!watch.value().contains("address") || !watch.value()["address"].is_string()) {
			NOTICE_LOG_FMT(CORE, "IPC: Failed to read address for watch {}", watch.key());
			return {};
		}
		std::string address = watch.value()["address"].get<std::string>();

		if (!watch.value().contains("size") || !watch.value()["size"].is_number()) {
			NOTICE_LOG_FMT(CORE, "IPC: Failed to read size for watch {}", watch.key());
			return {};
		}
		int size = watch.value()["size"].get<int>();

		std::optional<std::string> offset = std::nullopt;
		if (watch.value().contains("offset") && watch.value()["offset"].is_string()) {
			offset = watch.value()["offset"].get<std::string>();
		}

		std::optional<std::string> current_value = std::nullopt;

		MemoryWatch mw;
		mw.address = address;
		mw.offset = offset;
		mw.size = size;
		mw.current_value = current_value;

		IPC::MemWatcher::GetInstance().WatchAddress(watch.key(), mw);
		names.push_back(watch.key());
	}
	return names;
}

std::map<std::string, std::string> HTTPServer::ReadMemWatches(std::vector<std::string> watch_names) {
	std::map<std::string, std::string> results;
	for (const auto& watch_name : watch_names) {
		std::optional<std::string> value = IPC::MemWatcher::GetInstance().FetchDmcpValue(watch_name);
		if (value.has_value()) {
			results[watch_name] = value.value();
		} else {
			NOTICE_LOG_FMT(CORE, "IPC: Failed to read value for watch {}", watch_name);
		}
	}
	return results;
}

void HTTPServer::SetupTest() {
	IPC::MemWatcher::GetInstance().GetFramesStartedFuture().wait();
	Core::System& system = Core::System::GetInstance();
	Core::SetState(system, Core::State::Paused);

	NOTICE_LOG_FMT(CORE, "IPC: Setting up test");
	// Startup values
	m_initial_watches = {};

	const char* value = std::getenv("MEMWATCHES");
	std::optional<nlohmann::json_abi_v3_12_0::json> json_data = ParseJson(value);
	std::vector<std::string> names = {};
	if (json_data && json_data->contains("watches")) {
		names = SetupMemWatchesFromJSON(*json_data);
		NOTICE_LOG_FMT(CORE, "IPC: Loaded {} memwatches from MEMWATCHES", names.size());
	} else {
		NOTICE_LOG_FMT(CORE, "IPC: No memwatches found in MEMWATCHES");
	}

	std::promise<void> promise;
	std::future<void> future = promise.get_future();
	Common::Event* completion_event_ptr = nullptr;
	
	// Queue the screenshot operation on the Host thread
	Core::QueueHostJob([&promise, &completion_event_ptr](Core::System& system) {
		completion_event_ptr = &Core::SaveScreenShotWithCallback("start");
		promise.set_value();
	}, true);

	future.get();
	Core::SetState(system, Core::State::Running);
	completion_event_ptr->Wait();
	IPC::MemWatcher::GetInstance().ResetFramesStarted();
	IPC::MemWatcher::GetInstance().GetFramesStartedFuture().wait();
	Core::SetState(system, Core::State::Paused);

	m_initial_watches = HTTPServer::ReadMemWatches(names);
	NOTICE_LOG_FMT(CORE, "IPC: Initial memwatches: {}", m_initial_watches.size());
}

} // namespace IPC
