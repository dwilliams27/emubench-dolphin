#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "Common/FileUtil.h"
#include "Core/HW/SystemTimers.h"
#include "Core/PowerPC/MMU.h"
#include "Core/Core.h"
#include "Common/Logging/Log.h"
#include "Core/System.h"
#include "IPC/MemWatcher.h"

namespace IPC {

MemWatcher& MemWatcher::GetInstance() {
  static MemWatcher instance;
  return instance;
}

void MemWatcher::WatchAddress(std::string name, const MemoryWatch& mw)
{
  m_memwatches[name] = mw;
  
  // Queue a job to run on the CPU thread to populate the watch value
  Core::QueueHostJob([this, name](Core::System& system) {
    const Core::CPUThreadGuard guard(system);

    if (m_memwatches.count(name) > 0) {
      MemoryWatch& mw = m_memwatches[name];
      UpdateDmcpValue(guard, mw);
    }
  }, true);
}

void MemWatcher::UpdateDmcpValues(const Core::CPUThreadGuard& guard)
{
  for (auto& [name, mw] : m_memwatches)
  {    
    UpdateDmcpValue(guard, mw);
  }
}

void MemWatcher::UpdateDmcpValue(const Core::CPUThreadGuard& guard, MemoryWatch& mw)
{
  if (mw.offset.has_value()) {
    std::string value = ChasePointer(guard, mw);
    mw.current_value = std::make_optional(value);
  } else {
    mw.current_value = std::make_optional(ReadNBytesAsHex(guard, std::stoul(mw.address, nullptr, 16), mw.size));
  }
}

std::optional<std::string> MemWatcher::FetchDmcpValue(const std::string& name)
{
  return m_memwatches[name].current_value;
}

// For now, just one level of chasing
std::string MemWatcher::ChasePointer(const Core::CPUThreadGuard& guard, const MemoryWatch& mw)
{
  u32 raw_address = std::stoul(mw.address, nullptr, 16);
  u32 raw_offset = mw.offset.has_value() ? std::stoul(mw.offset.value(), nullptr, 16) : 0;
  u32 pointed_to_address = PowerPC::MMU::HostRead_U32(guard, raw_address);
  if (!PowerPC::MMU::HostIsRAMAddress(guard, pointed_to_address + raw_offset)) {
    NOTICE_LOG_FMT(CORE, "IPC: Pointer address out of bounds");
    return 0;
  }

  return ReadNBytesAsHex(guard, pointed_to_address + raw_offset, mw.size);
}

std::string MemWatcher::ReadNBytesAsHex(const Core::CPUThreadGuard& guard, u32 address, u32 size)
{
  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0');

  switch (size) {
    case 1: { // 1 byte
      u8 val8 = PowerPC::MMU::HostRead_U8(guard, address);
      ss << std::setw(2) << static_cast<u32>(val8);
      break;
    }
    case 2: { // 2 bytes
      u16 val16 = PowerPC::MMU::HostRead_U16(guard, address);
      ss << std::setw(4) << val16;
      break;
    }
    case 4: { // 4 bytes
      u32 val32 = PowerPC::MMU::HostRead_U32(guard, address);
      ss << std::setw(8) << val32;
      break;
    }
    case 8: { // 8 bytes
      u64 val64 = PowerPC::MMU::HostRead_U64(guard, address);
      ss << std::setw(16) << val64;
      break;
    }
    default: { // For other sizes, read byte by byte
      for (u32 i = 0; i < size; i++) {
        u8 val = PowerPC::MMU::HostRead_U8(guard, address + i);
        ss << std::setw(2) << static_cast<u32>(val);
      }
      break;
    }
  }
  std::string value = ss.str();
  return value;
}

void MemWatcher::Step(const Core::CPUThreadGuard& guard)
{
  UpdateDmcpValues(guard);
}

} // namespace IPC
