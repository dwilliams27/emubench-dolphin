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

namespace Pad
{
static InputConfig s_config("GCPadNew", _trans("Pad"), "GCPad", "Pad");

// Structure to hold HTTP controller input data
struct HTTPControllerState
{
  std::mutex mutex;
  std::optional<GCPadStatus> status;
  bool enabled = true;
};

// Global instance of HTTP controller state for each pad
static std::array<HTTPControllerState, 4> s_http_controllers;

// Function to update controller state from HTTP input
void UpdateControllerStateFromHTTP(int pad_num, const GCPadStatus& status)
{
  if (pad_num < 0 || pad_num >= 4)
    return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  s_http_controllers[pad_num].status = status;
}

// Function to enable/disable HTTP controller input
void EnableHTTPController(int pad_num, bool enabled)
{
  if (pad_num < 0 || pad_num >= 4)
    return;

  std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
  s_http_controllers[pad_num].enabled = enabled;
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
  // [dmcp]
  if (pad_num >= 0 && pad_num < 4)
  {
    std::lock_guard<std::mutex> lock(s_http_controllers[pad_num].mutex);
    if (s_http_controllers[pad_num].enabled && s_http_controllers[pad_num].status.has_value())
    {
      return *s_http_controllers[pad_num].status;
    }
  }

  return static_cast<GCPad*>(s_config.GetController(pad_num))->GetInput();
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
