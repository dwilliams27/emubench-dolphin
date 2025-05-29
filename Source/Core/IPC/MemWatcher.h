#pragma once

#include "Common/CommonTypes.h"

#include <future>
#include <map>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

namespace IPC
{
class CPUThreadGuard;

// Represents a memory watch configuration that matches the TypeScript interface
struct MemoryWatch
{
  std::string address;
  std::optional<std::string> offset;
  u32 size;
  std::optional<std::string> current_value;
};

// This guy based off of `MemoryWatcher.cpp`
class MemWatcher final
{
public:
  // Singleton pattern
  static MemWatcher& GetInstance();
      
  // Delete copy constructor and assignment operator
  MemWatcher(const MemWatcher&) = delete;
  MemWatcher& operator=(const MemWatcher&) = delete;
  void Step(const Core::CPUThreadGuard& guard);
  void WatchAddress(std::string name, const MemoryWatch& mw);
  std::string ReadNBytesAsHex(const Core::CPUThreadGuard& guard, u32 address, u32 size);
  std::optional<std::string> FetchDmcpValue(const std::string& name);
  
  // Get future that resolves when Step runs for the first time
  std::shared_future<void> GetFramesStartedFuture() { return m_frames_started.get_future().share(); }
  
  void ResetFramesStarted();

private:
  MemWatcher() = default;

  void UpdateDmcpValues(const Core::CPUThreadGuard& guard);
  void UpdateDmcpValue(const Core::CPUThreadGuard& guard, MemoryWatch& mw);
  std::string ChasePointer(const Core::CPUThreadGuard& guard, const MemoryWatch& mw);

  std::map<std::string, MemoryWatch> m_memwatches;
  std::promise<void> m_frames_started;
  bool m_step_called = false;
};

} // namespace IPC
