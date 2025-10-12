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
      UpdateValue(guard, mw);
    }
  }, true);
}

void MemWatcher::UpdateValues(const Core::CPUThreadGuard& guard)
{
  for (auto& [name, mw] : m_memwatches)
  {
    UpdateValue(guard, mw);
  }
}

void MemWatcher::UpdateValue(const Core::CPUThreadGuard& guard, MemoryWatch& mw)
{
  if (mw.offsets.has_value() && !mw.offsets->empty()) {
    std::string value = ChasePointer(guard, mw);
    mw.current_value = std::make_optional(value);
  } else {
    u32 address = std::stoul(mw.address, nullptr, 16);
    if (!PowerPC::MMU::HostIsRAMAddress(guard, address)) {
      NOTICE_LOG_FMT(CORE, "IPC: Direct address out of bounds: 0x{:08X}", address);
      mw.current_value = std::make_optional("0");
      return;
    }
    mw.current_value = std::make_optional(ReadNBytesAsHex(guard, address, mw.size));
  }
}

std::optional<std::string> MemWatcher::FetchValue(const std::string& name)
{
  return m_memwatches[name].current_value;
}

std::string MemWatcher::ChasePointer(const Core::CPUThreadGuard& guard, const MemoryWatch& mw)
{
  u32 current_address = std::stoul(mw.address, nullptr, 16);

  if (!PowerPC::MMU::HostIsRAMAddress(guard, current_address)) {
    NOTICE_LOG_FMT(CORE, "IPC: Initial pointer address out of bounds: 0x{:08X}", current_address);
    return "0";
  }

  if (!mw.offsets.has_value()) {
    // No offsets, just read directly from the address
    return ReadNBytesAsHex(guard, current_address, mw.size);
  }
  
  // Chase through each offset
  for (const auto& offset : *mw.offsets) {
    if (!PowerPC::MMU::HostIsRAMAddress(guard, current_address)) {
      NOTICE_LOG_FMT(CORE, "IPC: Pointer address out of bounds during chase: 0x{:08X}", current_address);
      return "0";
    }
    
    // Read the pointer value at current address
    u32 pointed_to_address = PowerPC::MMU::HostRead_U32(guard, current_address);
    
    // Calculate the next address by adding the offset
    current_address = pointed_to_address + offset;
    
    // Check bounds for the next address
    if (!PowerPC::MMU::HostIsRAMAddress(guard, current_address)) {
      NOTICE_LOG_FMT(CORE, "IPC: Pointer address out of bounds during chase: 0x{:08X}", current_address);
      return "0";
    }
  }

  return ReadNBytesAsHex(guard, current_address, mw.size);
}

std::string MemWatcher::ReadNBytesAsHex(const Core::CPUThreadGuard& guard, u32 address, u32 size)
{
  if (!PowerPC::MMU::HostIsRAMAddress(guard, address)) {
    NOTICE_LOG_FMT(CORE, "IPC: Read address out of bounds: 0x{:08X}", address);
    return "0";
  }
  
  if (size > 1 && !PowerPC::MMU::HostIsRAMAddress(guard, address + size - 1)) {
    NOTICE_LOG_FMT(CORE, "IPC: Read end address out of bounds: 0x{:08X}", address + size - 1);
    return "0";
  }
  
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
  if (!m_step_called) {
    m_step_called = true;
    m_frames_started.set_value();
  }
  UpdateValues(guard);
}

void MemWatcher::ResetFramesStarted()
{
  m_step_called = false;
  m_frames_started = std::promise<void>();
}

} // namespace IPC
