#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "Common/FileUtil.h"
#include "Core/HW/SystemTimers.h"
#include "Core/PowerPC/MMU.h"
#include "Common/Logging/Log.h"
#include "IPC/MemWatch.h"

namespace IPC {

MemWatch& MemWatch::GetInstance() {
  static MemWatch instance;
  return instance;
}

void MemWatch::WatchAddress(const std::string& addr)
{
  m_dmcp_values[addr] = 0;
  m_dmcp_addresses[addr] = std::vector<u32>();

  std::istringstream offsets(addr);
  offsets >> std::hex;
  u32 offset;
  while (offsets >> offset)
    m_dmcp_addresses[addr].push_back(offset);
}

void MemWatch::UpdateDmcpValues(const Core::CPUThreadGuard& guard) {
  for (auto& entry : m_dmcp_values)
  {
    std::string address = entry.first;
    u32& current_value = entry.second;

    u32 new_value = ChasePointer(guard, address);
    if (new_value != current_value)
    {
      current_value = new_value;
    }
  }
}

std::optional<u32> MemWatch::FetchDmcpValue(const std::string& addr) {
  auto it = m_dmcp_values.find(addr);

  if (it != m_dmcp_values.end()) {
    return m_dmcp_values.at(it->first);
  } else {
    return std::nullopt;
  }
}

u32 MemWatch::ChasePointer(const Core::CPUThreadGuard& guard, const std::string& line)
{
  u32 value = 0;

  for (u32 offset : m_dmcp_addresses[line])
  {
    value = PowerPC::MMU::HostRead_U32(guard, value + offset);
    if (!PowerPC::MMU::HostIsRAMAddress(guard, value))
      break;
  }
  return value;
}

void MemWatch::Step(const Core::CPUThreadGuard& guard)
{
  UpdateDmcpValues(guard);
}

} // namespace IPC
