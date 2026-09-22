#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace minirv {

inline constexpr uint32_t kPmemBase = 0x80000000u;
inline constexpr uint32_t kUartAddress = 0x10000000u;
inline constexpr uint32_t kRtcLowAddress = 0x20000000u;
inline constexpr uint32_t kRtcHighAddress = 0x20000004u;

struct StepResult {
  bool ok = false;
  bool halted = false;
  bool store = false;
  uint32_t pc = 0;
  uint32_t inst = 0;
  uint32_t store_address = 0;
  uint32_t store_data = 0;
  uint8_t store_size = 0;
  std::string error;
};

class Emulator {
 public:
  void reset(const uint8_t *image, size_t image_size);
  StepResult step(uint64_t uptime_us);
  uint32_t pc() const { return pc_; }
  uint32_t reg(unsigned index) const { return index == 0 ? 0 : gpr_[index]; }
  bool reg_valid(unsigned index) const { return index == 0 || written_[index]; }
  uint8_t pmem_byte(uint32_t address) const;

 private:
  bool mapped(uint32_t address, uint32_t size) const;
  uint32_t read_pmem(uint32_t address, uint32_t size) const;

  std::array<uint32_t, 32> gpr_{};
  std::array<bool, 32> written_{};
  std::vector<uint8_t> pmem_;
  uint32_t pc_ = kPmemBase;
};

}  // namespace minirv
