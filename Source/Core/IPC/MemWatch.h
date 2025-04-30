#pragma once

#include "Common/CommonTypes.h"

#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <vector>

namespace IPC
{
class CPUThreadGuard;

// This guy based off of `MemoryWatcher.cpp`
class MemWatch final
{
public:
  // Singleton pattern
  static MemWatch& GetInstance();
      
  // Delete copy constructor and assignment operator
  MemWatch(const MemWatch&) = delete;
  MemWatch& operator=(const MemWatch&) = delete;
  void Step(const Core::CPUThreadGuard& guard);
  void WatchAddress(const std::string& addr);
  std::optional<u32> FetchDmcpValue(const std::string& addr);

private:
  MemWatch() = default;

  void UpdateDmcpValues(const Core::CPUThreadGuard& guard);
  u32 ChasePointer(const Core::CPUThreadGuard& guard, const std::string& line);

  std::map<std::string, u32> m_dmcp_values;
  std::map<std::string, std::vector<u32>> m_dmcp_addresses;
};

} // namespace IPC
