#include "ControllerCommands.h"
#include "Core/HW/GCPad.h"
#include "Core/HW/SI/SI.h"
#include "Core/HW/SI/SI_Device.h"
#include "Core/Core.h"
#include "Core/System.h"
#include "Common/Logging/Log.h"
#include "InputCommon/GCPadStatus.h"

#include <nlohmann/json.hpp>

namespace IPC {

IPCControllerInput ParseIPCControllerInput(const nlohmann::json& j) {
  IPCControllerInput input;
  
  // Parse connection status
  if (j.contains("connected") && j["connected"].is_boolean()) {
      input.connected = j["connected"].get<bool>();
  }
  
  // Parse buttons
  if (j.contains("buttons") && j["buttons"].is_object()) {
      const auto& buttons = j["buttons"];
      if (buttons.contains("a") && buttons["a"].is_boolean()) 
          input.buttons.a = buttons["a"].get<bool>();
      if (buttons.contains("b") && buttons["b"].is_boolean()) 
          input.buttons.b = buttons["b"].get<bool>();
      if (buttons.contains("x") && buttons["x"].is_boolean()) 
          input.buttons.x = buttons["x"].get<bool>();
      if (buttons.contains("y") && buttons["y"].is_boolean()) 
          input.buttons.y = buttons["y"].get<bool>();
      if (buttons.contains("z") && buttons["z"].is_boolean()) 
          input.buttons.z = buttons["z"].get<bool>();
      if (buttons.contains("start") && buttons["start"].is_boolean()) 
          input.buttons.start = buttons["start"].get<bool>();
      if (buttons.contains("up") && buttons["up"].is_boolean()) 
          input.buttons.up = buttons["up"].get<bool>();
      if (buttons.contains("down") && buttons["down"].is_boolean()) 
          input.buttons.down = buttons["down"].get<bool>();
      if (buttons.contains("left") && buttons["left"].is_boolean()) 
          input.buttons.left = buttons["left"].get<bool>();
      if (buttons.contains("right") && buttons["right"].is_boolean()) 
          input.buttons.right = buttons["right"].get<bool>();
      if (buttons.contains("l") && buttons["l"].is_boolean()) 
          input.buttons.l = buttons["l"].get<bool>();
      if (buttons.contains("r") && buttons["r"].is_boolean()) 
          input.buttons.r = buttons["r"].get<bool>();
  }
  
  // Parse main stick
  if (j.contains("mainStick") && j["mainStick"].is_object()) {
      const auto& stick = j["mainStick"];
      if (stick.contains("x") && stick["x"].is_number_unsigned()) 
          input.mainStick.x = stick["x"].get<uint8_t>();
      if (stick.contains("y") && stick["y"].is_number_unsigned()) 
          input.mainStick.y = stick["y"].get<uint8_t>();
  }
  
  // Parse C-stick
  if (j.contains("cStick") && j["cStick"].is_object()) {
      const auto& stick = j["cStick"];
      if (stick.contains("x") && stick["x"].is_number_unsigned()) 
          input.cStick.x = stick["x"].get<uint8_t>();
      if (stick.contains("y") && stick["y"].is_number_unsigned()) 
          input.cStick.y = stick["y"].get<uint8_t>();
  }
  
  // Parse triggers
  if (j.contains("triggers") && j["triggers"].is_object()) {
      const auto& triggers = j["triggers"];
      if (triggers.contains("l") && triggers["l"].is_number_unsigned()) 
          input.triggers.l = triggers["l"].get<uint8_t>();
      if (triggers.contains("r") && triggers["r"].is_number_unsigned()) 
          input.triggers.r = triggers["r"].get<uint8_t>();
  }
  
  return input;
}

GCPadStatus ConvertToGCPadStatus(const IPCControllerInput& input) {
  GCPadStatus status;
  memset(&status, 0, sizeof(status));
  
  // Set connection status
  status.isConnected = input.connected;
  
  // Set button states
  status.button = 0;
  if (input.buttons.a) status.button |= PAD_BUTTON_A;
  if (input.buttons.b) status.button |= PAD_BUTTON_B;
  if (input.buttons.x) status.button |= PAD_BUTTON_X;
  if (input.buttons.y) status.button |= PAD_BUTTON_Y;
  if (input.buttons.z) status.button |= PAD_TRIGGER_Z;
  if (input.buttons.start) status.button |= PAD_BUTTON_START;
  if (input.buttons.up) status.button |= PAD_BUTTON_UP;
  if (input.buttons.down) status.button |= PAD_BUTTON_DOWN;
  if (input.buttons.left) status.button |= PAD_BUTTON_LEFT;
  if (input.buttons.right) status.button |= PAD_BUTTON_RIGHT;
  if (input.buttons.l) status.button |= PAD_TRIGGER_L;
  if (input.buttons.r) status.button |= PAD_TRIGGER_R;
  
  // Set analog stick positions
  status.stickX = input.mainStick.x;
  status.stickY = input.mainStick.y;
  status.substickX = input.cStick.x;
  status.substickY = input.cStick.y;
  
  // Set analog triggers
  status.triggerLeft = input.triggers.l;
  status.triggerRight = input.triggers.r;
  
  return status;
}

} // namespace IPC
