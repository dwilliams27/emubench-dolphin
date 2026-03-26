#pragma once

#include "InputCommon/GCPadStatus.h"

#include <nlohmann/json.hpp>

#include <string>
#include <cstdint>
#include <vector>

namespace IPC {

struct IPCControllerInput {
  struct Buttons {
    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;
    bool z = false;
    bool start = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool l = false;
    bool r = false;
  };

  struct StickPosition {
    uint8_t x = 128;  // 0-255, center at 128
    uint8_t y = 128;  // 0-255, center at 128
  };

  struct TriggerValues {
    uint8_t l = 0;  // 0-255
    uint8_t r = 0;  // 0-255
  };

  bool connected = true;
  Buttons buttons;
  StickPosition mainStick;
  StickPosition cStick;
  TriggerValues triggers;
  uint32_t frames = 0;
};

IPCControllerInput ParseIPCControllerInput(const nlohmann::json& j);
std::vector<IPCControllerInput> ParseIPCControllerInputSequence(const nlohmann::json& j);
GCPadStatus ConvertToGCPadStatus(const IPCControllerInput& input);

} // Namespace IPC
