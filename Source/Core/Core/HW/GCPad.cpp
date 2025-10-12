// Copyright 2010 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/HW/GCPad.h"

#include <cstring>
#include <mutex>
#include <optional>

#include "Common/Common.h"
#include "Core/HW/GCPadEmu.h"
#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/GCPadStatus.h"
#include "InputCommon/InputConfig.h"
// [dmcp]
#include "Common/Logging/Log.h"
#include <vector>

namespace Pad
{
static InputConfig s_config("GCPadNew", _trans("Pad"), "GCPad", "Pad");

// [dmcp]
struct TimedInput
{
  uint16_t button_mask;
  std::optional<uint8_t> stick_x;
  std::optional<uint8_t> stick_y;
  std::optional<uint8_t> substick_x;
  std::optional<uint8_t> substick_y;
  uint32_t remaining_frames;
};

struct HTTPControllerState
{
  std::mutex mutex;
  std::optional<GCPadStatus> persistent_status; // Renamed from 'status'
  std::vector<TimedInput> timed_inputs;         // Queue for timed inputs
  bool enabled = true;
};

// Global instance of HTTP controller state for each pad
static std::array<HTTPControllerState, 4> s_http_controllers;

void EnableHTTPController(int pad_num, bool enabled)
{
  if (pad_num < 0 || pad_num >= 4)
    return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  s_http_controllers[pad_num].enabled = enabled;
  s_http_controllers[pad_num].persistent_status = GCPadStatus{};
  s_http_controllers[pad_num].persistent_status->stickX = GCPadStatus::MAIN_STICK_CENTER_X;
  s_http_controllers[pad_num].persistent_status->stickY = GCPadStatus::MAIN_STICK_CENTER_Y;
  s_http_controllers[pad_num].persistent_status->substickX = GCPadStatus::C_STICK_CENTER_X;
  s_http_controllers[pad_num].persistent_status->substickY = GCPadStatus::C_STICK_CENTER_Y;
  NOTICE_LOG_FMT(CORE, "IPC Controller {} enabled {}",
                 pad_num, enabled ? "enabled" : "disabled");
}

void QueueTimedInput(int pad_num, const GCPadStatus& status, uint32_t frames)
{
  if (pad_num < 0 || pad_num >= 4 || frames == 0)
    return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  s_http_controllers[pad_num].timed_inputs.push_back({status.button, status.stickX, status.stickY, status.substickX, status.substickY, frames});
}

void AdvanceFrame(int pad_num)
{
  if (pad_num < 0 || pad_num >= 4)
      return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  auto& timed_queue = s_http_controllers[pad_num].timed_inputs;

  if (timed_queue.empty())
      return; // Nothing to do

  // Decrement frame counts and prepare to remove expired entries
  for (auto& timed_input : timed_queue) {
      if (timed_input.remaining_frames > 0) {
          timed_input.remaining_frames--;
      }
  }

  // Remove entries where frames reached 0 using erase-remove idiom
  timed_queue.erase(std::remove_if(timed_queue.begin(), timed_queue.end(),
                                   [](const TimedInput& input) { return input.remaining_frames == 0; }),
                    timed_queue.end());
}

void UpdateControllerStateFromHTTP(int pad_num, const GCPadStatus& status)
{
  if (pad_num < 0 || pad_num >= 4)
    return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  s_http_controllers[pad_num].persistent_status = status;

  NOTICE_LOG_FMT(CORE, "IPC Persistent Controller state updated for pad {}: "
                 "button: {}, stickX: {}, stickY: {}, substickX: {}, substickY: {}, "
                 "triggerLeft: {}, triggerRight: {}",
                 pad_num, status.button, status.stickX, status.stickY,
                 status.substickX, status.substickY, status.triggerLeft,
                 status.triggerRight);
}

InputConfig* GetConfig()
{
  return &s_config;
}

void Shutdown()
{
  s_config.UnregisterHotplugCallback();

  s_config.ClearControllers();
}

void Initialize()
{
  if (s_config.ControllersNeedToBeCreated())
  {
    for (unsigned int i = 0; i < 4; ++i)
      s_config.CreateController<GCPad>(i);
  }

  s_config.RegisterHotplugCallback();

  // Load the saved controller config
  s_config.LoadConfig();
}

void LoadConfig()
{
  s_config.LoadConfig();
}

void GenerateDynamicInputTextures()
{
  s_config.GenerateControllerTextures();
}

bool IsInitialized()
{
  return !s_config.ControllersNeedToBeCreated();
}

GCPadStatus GetStatus(int pad_num)
{
  // [dmcp] DEBUG
  // return static_cast<GCPad*>(s_config.GetController(pad_num))->GetInput();
  // [dmcp]
  if (pad_num < 0 || pad_num >= 4)
  {
    return {};
  }

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  auto& controller_state = s_http_controllers[pad_num];

  if (controller_state.enabled)
  {
    GCPadStatus current_status = controller_state.persistent_status.value_or(GCPadStatus{});

    // Combine active timed button presses
    uint16_t combined_timed_buttons = 0;
    for (const auto& timed_input : controller_state.timed_inputs)
    {
      // No need to check remaining_frames here, AdvanceFrame handles expiration
      combined_timed_buttons |= timed_input.button_mask;

      if (timed_input.stick_x.has_value()) current_status.stickX = *timed_input.stick_x;
      if (timed_input.stick_y.has_value()) current_status.stickY = *timed_input.stick_y;
      if (timed_input.substick_x.has_value()) current_status.substickX = *timed_input.substick_x;
      if (timed_input.substick_y.has_value()) current_status.substickY = *timed_input.substick_y;
    }

    // Apply timed buttons ON TOP of persistent buttons
    // (Timed press overrides persistent release for the duration)
    current_status.button |= combined_timed_buttons;

    return current_status;
  }
  else
  {
    // HTTP controller is disabled, fall back to standard input polling
    return static_cast<GCPad*>(s_config.GetController(pad_num))->GetInput();
  }
}

ControllerEmu::ControlGroup* GetGroup(int pad_num, PadGroup group)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetGroup(group);
}

void Rumble(const int pad_num, const ControlState strength)
{
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(strength);
}

void ResetRumble(const int pad_num)
{
  static_cast<GCPad*>(s_config.GetController(pad_num))->SetOutput(0.0);
}

bool GetMicButton(const int pad_num)
{
  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetMicButton();
}
}  // namespace Pad
