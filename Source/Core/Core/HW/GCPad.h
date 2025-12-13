// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"

class InputConfig;
enum class PadGroup;
struct GCPadStatus;

namespace ControllerEmu
{
class ControlGroup;
}

namespace Pad
{
void Shutdown();
void Initialize();
void LoadConfig();
void GenerateDynamicInputTextures();
bool IsInitialized();

InputConfig* GetConfig();

// [emubench]

/**
 * @brief Queues a controller input state to be active for a specific number of frames.
 * Only the button state from the status is used for timed input.
 * @param pad_num The controller port (0-3).
 * @param status The controller status containing the buttons to press.
 * @param frames The number of frames the buttons should remain pressed.
 */
void QueueTimedInput(int pad_num, const GCPadStatus& status, uint32_t frames);

/**
 * @brief Advances the frame counter for timed inputs for a specific controller port.
 * This should be called once per emulator frame for each port before GetStatus is called.
 * @param pad_num The controller port (0-3).
 */
void AdvanceFrame(int pad_num);
void UpdateControllerStateFromHTTP(int pad_num, const GCPadStatus& status);
void EnableHTTPController(int pad_num, bool enabled);

GCPadStatus GetStatus(int pad_num);
ControllerEmu::ControlGroup* GetGroup(int pad_num, PadGroup group);
void Rumble(int pad_num, ControlState strength);
void ResetRumble(int pad_num);

bool GetMicButton(int pad_num);
}  // namespace Pad
