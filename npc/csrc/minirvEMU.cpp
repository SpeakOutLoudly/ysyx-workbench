#include "minirvEMU.h"

#include "defines.h"

#include <cstring>
#include <stdexcept>

namespace minirv {
namespace {

uint32_t sign_extend(uint32_t value, unsigned bits) {
  const uint32_t sign = 1u << (bits - 1);
  return (value ^ sign) - sign;
}

}  // namespace

void Emulator::reset(const uint8_t *image, size_t image_size) {
  if (image_size > PMEM_SIZE || (image_size != 0 && image == nullptr)) {
    throw std::invalid_argument("invalid minirvEMU image");
  }
  gpr_.fill(0);
  written_.fill(false);
  pc_ = kPmemBase;
  pmem_.assign(PMEM_SIZE, 0);
  if (image_size != 0) std::memcpy(pmem_.data(), image, image_size);
}

bool Emulator::mapped(uint32_t address, uint32_t size) const {
  return address >= kPmemBase &&
         static_cast<uint64_t>(address - kPmemBase) + size <= pmem_.size();
}

uint32_t Emulator::read_pmem(uint32_t address, uint32_t size) const {
  uint32_t data = 0;
  const uint32_t offset = address - kPmemBase;
  for (uint32_t i = 0; i < size; ++i)
    data |= static_cast<uint32_t>(pmem_[offset + i]) << (8 * i);
  return data;
}

uint8_t Emulator::pmem_byte(uint32_t address) const {
  if (!mapped(address, 1)) throw std::out_of_range("PMEM address");
  return pmem_[address - kPmemBase];
}

StepResult Emulator::step(uint64_t uptime_us) {
  StepResult step;
  step.pc = pc_;
  if ((pc_ & 3u) != 0 || !mapped(pc_, 4)) {
    step.error = "instruction fetch outside aligned PMEM";
    return step;
  }

  const uint32_t inst = read_pmem(pc_, 4);
  step.inst = inst;
  if (inst == 0x00100073u) {
    step.ok = true;
    step.halted = true;
    return step;
  }

  const uint32_t opcode = inst & 0x7fu;
  const uint32_t rd = (inst >> 7) & 31u;
  const uint32_t funct3 = (inst >> 12) & 7u;
  const uint32_t rs1 = (inst >> 15) & 31u;
  const uint32_t rs2 = (inst >> 20) & 31u;
  const uint32_t funct7 = inst >> 25;
  const uint32_t a = gpr_[rs1];
  const uint32_t b = gpr_[rs2];
  const uint32_t imm_i = sign_extend(inst >> 20, 12);
  uint32_t next_pc = pc_ + 4;
  uint32_t result = 0;
  bool write_rd = false;

  auto invalid = [&step](const char *reason) {
    step.error = reason;
    return step;
  };

  switch (opcode) {
    case 0x37:  // LUI
      result = inst & 0xfffff000u;
      write_rd = true;
      break;
    case 0x17:  // AUIPC
      result = pc_ + (inst & 0xfffff000u);
      write_rd = true;
      break;
    case 0x13:  // OP-IMM
      write_rd = true;
      switch (funct3) {
        case 0: result = a + imm_i; break;  // ADDI
        case 1:  // SLLI
          if (funct7 != 0) return invalid("invalid SLLI encoding");
          result = a << rs2;
          break;
        case 2: result = static_cast<int32_t>(a) < static_cast<int32_t>(imm_i); break;
        case 3: result = a < imm_i; break;
        case 4: result = a ^ imm_i; break;
        case 5:  // SRLI/SRAI
          if (funct7 == 0) result = a >> rs2;
          else if (funct7 == 0x20)
            result = static_cast<uint32_t>(static_cast<int32_t>(a) >> rs2);
          else return invalid("invalid right-shift immediate encoding");
          break;
        case 6: result = a | imm_i; break;
        case 7: result = a & imm_i; break;
      }
      break;
    case 0x33:  // OP
      write_rd = true;
      if (funct3 == 0 && funct7 == 0x20) result = a - b;
      else if (funct3 == 5 && funct7 == 0x20)
        result = static_cast<uint32_t>(static_cast<int32_t>(a) >> (b & 31u));
      else if (funct7 == 0) {
        switch (funct3) {
          case 0: result = a + b; break;
          case 1: result = a << (b & 31u); break;
          case 2: result = static_cast<int32_t>(a) < static_cast<int32_t>(b); break;
          case 3: result = a < b; break;
          case 4: result = a ^ b; break;
          case 5: result = a >> (b & 31u); break;
          case 6: result = a | b; break;
          case 7: result = a & b; break;
        }
      } else return invalid("unsupported OP encoding");
      break;
    case 0x03: {  // LOAD
      uint32_t size;
      switch (funct3) {
        case 0: case 4: size = 1; break;
        case 1: case 5: size = 2; break;
        case 2: size = 4; break;
        default: return invalid("unsupported LOAD encoding");
      }
      const uint32_t address = a + imm_i;
      if ((address & (size - 1)) != 0) return invalid("misaligned load");
      if (address == kRtcLowAddress && size == 4)
        result = static_cast<uint32_t>(uptime_us);
      else if (address == kRtcHighAddress && size == 4)
        result = static_cast<uint32_t>(uptime_us >> 32);
      else if (mapped(address, size))
        result = read_pmem(address, size);
      else return invalid("load outside PMEM/MMIO");
      if (funct3 == 0) result = sign_extend(result, 8);
      if (funct3 == 1) result = sign_extend(result, 16);
      write_rd = true;
      break;
    }
    case 0x23: {  // STORE
      uint32_t size;
      switch (funct3) {
        case 0: size = 1; break;
        case 1: size = 2; break;
        case 2: size = 4; break;
        default: return invalid("unsupported STORE encoding");
      }
      const uint32_t imm_s = sign_extend(((inst >> 25) << 5) | rd, 12);
      const uint32_t address = a + imm_s;
      if ((address & (size - 1)) != 0) return invalid("misaligned store");
      if (address != kUartAddress || size != 1) {
        if (!mapped(address, size)) return invalid("store outside PMEM/MMIO");
        const uint32_t offset = address - kPmemBase;
        for (uint32_t i = 0; i < size; ++i)
          pmem_[offset + i] = static_cast<uint8_t>(b >> (8 * i));
      }
      step.store = true;
      step.store_address = address;
      step.store_data = b;
      step.store_size = static_cast<uint8_t>(size);
      break;
    }
    case 0x63: {  // BRANCH
      const uint32_t imm_b = sign_extend(
          ((inst >> 31) << 12) | (((inst >> 7) & 1u) << 11) |
          (((inst >> 25) & 63u) << 5) | (((inst >> 8) & 15u) << 1), 13);
      bool taken;
      switch (funct3) {
        case 0: taken = a == b; break;
        case 1: taken = a != b; break;
        case 4: taken = static_cast<int32_t>(a) < static_cast<int32_t>(b); break;
        case 5: taken = static_cast<int32_t>(a) >= static_cast<int32_t>(b); break;
        case 6: taken = a < b; break;
        case 7: taken = a >= b; break;
        default: return invalid("unsupported BRANCH encoding");
      }
      if (taken) next_pc = pc_ + imm_b;
      break;
    }
    case 0x6f: {  // JAL
      const uint32_t imm_j = sign_extend(
          ((inst >> 31) << 20) | (inst & 0x000ff000u) |
          (((inst >> 20) & 1u) << 11) | (((inst >> 21) & 0x3ffu) << 1), 21);
      result = pc_ + 4;
      next_pc = pc_ + imm_j;
      write_rd = true;
      break;
    }
    case 0x67:  // JALR
      if (funct3 != 0) return invalid("invalid JALR encoding");
      result = pc_ + 4;
      next_pc = (a + imm_i) & ~1u;
      write_rd = true;
      break;
    default:
      return invalid("unsupported instruction");
  }

  if (write_rd && rd != 0) {
    gpr_[rd] = result;
    written_[rd] = true;
  }
  gpr_[0] = 0;
  pc_ = next_pc;
  step.ok = true;
  return step;
}

}  // namespace minirv
