#include "Core/State.h"
#include "Core/System.h"
#include <string>

namespace IPC
{

class SaveState final
{
public:
  // Singleton pattern
  static SaveState& GetInstance();
  
  // Delete copy constructor and assignment operator
  SaveState(const SaveState&) = delete;
  SaveState& operator=(const SaveState&) = delete;

  void SaveToSlot(int slot_number, bool wait_for_completion = false);
  void LoadFromSlot(int slot_number);
  std::string GetSlotInfo(int slot_number);
  void SaveToFile(const std::string& filepath, bool wait_for_completion = false);
  void LoadFromFile(const std::string& filepath);

private:
  SaveState() = default;
};

} // namespace IPC
