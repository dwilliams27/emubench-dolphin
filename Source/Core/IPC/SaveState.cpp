#include "IPC/SaveState.h"

namespace IPC
{

SaveState& SaveState::GetInstance() {
  static SaveState instance;
  return instance;
}

// Save to one of the numbered slots (0-99)
void SaveState::SaveToSlot(int slot_number, bool wait_for_completion)
{
  Core::System& system = Core::System::GetInstance();
  State::Save(system, slot_number, wait_for_completion);
}

// Get information about a save state in a slot
std::string SaveState::GetSlotInfo(int slot_number)
{
  return State::GetInfoStringOfSlot(slot_number);
}

void SaveState::LoadFromSlot(int slot_number)
{
  Core::System& system = Core::System::GetInstance();
  State::Load(system, slot_number);
}

} // namespace IPC
