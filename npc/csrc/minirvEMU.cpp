#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

extern bool load_rom(uint32_t rom[], uint32_t &rom_size, const char *filename);

// ROM 按字存放指令。PC 是字节地址，因此取指时使用 pc / 4 作为下标。
uint32_t rom[128] = {};
uint32_t rom_size = 0;

// RV32E 只有 x0 ~ x15，其中 x0 恒为 0。
uint32_t gpr[16] = {};
uint32_t pc = 0;
uint32_t next_pc = 0;

// 简单的 4 KiB 字节寻址内存，地址范围为 0x00000000 ~ 0x00000fff。
constexpr uint32_t MEMORY_SIZE = 4096;
uint8_t memory[MEMORY_SIZE] = {};

enum class AluOp {
  ADD,
  SUB,
  AND,
  OR,
  XOR,

  JAL,
  JARL,
};

enum class MemoryOp {
  NONE,
  LOAD,
  STORE,
};

struct DecodedInst {
  AluOp alu_op = AluOp::ADD;
  uint32_t rd = 0;
  uint32_t rs1 = 0;
  uint32_t rs2 = 0;
  uint32_t funct3 = 0;
  uint32_t imm = 0;
  bool use_imm = false;
  bool valid = false;

  bool update_pc = false;
  MemoryOp memory_op = MemoryOp::NONE;
};

// 对低 bit_width 位进行符号扩展或零扩展。
uint32_t extend(uint32_t value, uint32_t bit_width, bool sign_extend) {
  if (bit_width == 0 || bit_width >= 32) {
    return value;
  }

  const uint32_t mask = (1u << bit_width) - 1;
  value &= mask;
  if (sign_extend && (value & (1u << (bit_width - 1))) != 0) {
    value |= ~mask;
  }
  return value;
}

// Memory 访问阶段。funct3 决定读取字节数以及扩展方式。
bool load(uint32_t address, uint32_t funct3, uint32_t &data) {
  uint32_t bytes = 0;
  switch (funct3) {
  case 0x2: // LW
    bytes = 4;
    break;
  case 0x4: // LBU
    bytes = 1;
    break;
  default:
    return false;
  }

  if ((bytes == 4 && (address & 0x3u) != 0) ||
      address > MEMORY_SIZE - bytes) {
    std::printf("load memory error: address = 0x%08x\n", address);
    return false;
  }

  data = 0;
  for (uint32_t i = 0; i < bytes; ++i) {
    data |= static_cast<uint32_t>(memory[address + i]) << (i * 8);
  }

  // LBU 将读取的 8 位数据零扩展到 32 位；LW 保持原 32 位数据。
  data = extend(data, bytes * 8, false);
  return true;
}

// funct3 决定写入字节数，内存按照 RISC-V 小端序存放。
bool store(uint32_t address, uint32_t funct3, uint32_t data) {
  uint32_t bytes = 0;
  switch (funct3) {
  case 0x0: // SB
    bytes = 1;
    break;
  case 0x2: // SW
    bytes = 4;
    break;
  default:
    return false;
  }

  if ((bytes == 4 && (address & 0x3u) != 0) ||
      address > MEMORY_SIZE - bytes) {
    std::printf("store memory error: address = 0x%08x\n", address);
    return false;
  }

  for (uint32_t i = 0; i < bytes; ++i) {
    memory[address + i] = static_cast<uint8_t>(data >> (i * 8));
  }
  return true;
}

// 取指阶段。
bool fetch(uint32_t &inst) {
  if ((pc & 0x3u) != 0 || pc / 4 >= rom_size) {
    std::printf("fetch error: pc = 0x%08x\n", pc);
    return false;
  }

  inst = rom[pc / 4];
  return true;
}

// 译码阶段。位域 inst[high:low] 在 C++ 中用移位和掩码取得。
DecodedInst decode(uint32_t inst) {
  DecodedInst decoded;
  const uint32_t opcode = inst & 0x7fu;           // inst[6:0]
  const uint32_t funct3 = (inst >> 12) & 0x7u;   // inst[14:12]
  const uint32_t funct7 = (inst >> 25) & 0x7fu;  // inst[31:25]

  decoded.rd = (inst >> 7) & 0x1fu;
  decoded.rs1 = (inst >> 15) & 0x1fu;
  decoded.rs2 = (inst >> 20) & 0x1fu;
  decoded.funct3 = funct3;

  switch (opcode) {
  case 0x13: { // OP-IMM
    const bool is_addi = (opcode == 0x13) && (funct3 == 0x0);
    const bool is_xori = (opcode == 0x13) && (funct3 == 0x4);
    const bool is_ori = (opcode == 0x13) && (funct3 == 0x6);
    const bool is_andi = (opcode == 0x13) && (funct3 == 0x7);

    decoded.use_imm = true;
    decoded.imm = extend(inst >> 20, 12, true);

    if (is_addi) {
      decoded.alu_op = AluOp::ADD;
    } else if (is_xori) {
      decoded.alu_op = AluOp::XOR;
    } else if (is_ori) {
      decoded.alu_op = AluOp::OR;
    } else if (is_andi) {
      decoded.alu_op = AluOp::AND;
    } else {
      return decoded;
    }

    // I 型 ALU 指令只使用 rd 和 rs1。
    decoded.valid = decoded.rd < 16 && decoded.rs1 < 16;
    return decoded;
  }

  case 0x33: { // OP  算术指令
    const bool is_add = (opcode == 0x33) && (funct3 == 0x0) &&
                        (funct7 == 0x00);
    const bool is_sub = (opcode == 0x33) && (funct3 == 0x0) &&
                        (funct7 == 0x20);
    const bool is_xor = (opcode == 0x33) && (funct3 == 0x4) &&
                        (funct7 == 0x00);
    const bool is_or = (opcode == 0x33) && (funct3 == 0x6) &&
                       (funct7 == 0x00);
    const bool is_and = (opcode == 0x33) && (funct3 == 0x7) &&
                        (funct7 == 0x00);

    if (is_add) {
      decoded.alu_op = AluOp::ADD;
    } else if (is_sub) {
      decoded.alu_op = AluOp::SUB;
    } else if (is_xor) {
      decoded.alu_op = AluOp::XOR;
    } else if (is_or) {
      decoded.alu_op = AluOp::OR;
    } else if (is_and) {
      decoded.alu_op = AluOp::AND;
    } else {
      return decoded;
    }

    decoded.valid = decoded.rd < 16 && decoded.rs1 < 16 && decoded.rs2 < 16;
    return decoded;
  }
  case 0x03: { // LOAD
    const bool is_lw = (opcode == 0x03) && (funct3 == 0x2);
    const bool is_lbu = (opcode == 0x03) && (funct3 == 0x4);
    if (!is_lw && !is_lbu) {
      return decoded;
    }

    decoded.memory_op = MemoryOp::LOAD;
    decoded.use_imm = true;
    decoded.imm = extend(inst >> 20, 12, true);

    decoded.valid = decoded.rd < 16 && decoded.rs1 < 16;
    return decoded;
  }
  case 0x23: { // STORE
    const bool is_sw = (opcode == 0x23) && (funct3 == 0x2);
    const bool is_sb = (opcode == 0x23) && (funct3 == 0x0);
    if (!is_sw && !is_sb) {
      return decoded;
    }

    decoded.memory_op = MemoryOp::STORE;
    decoded.use_imm = true;
    decoded.imm = extend(((inst >> 25) << 5) | ((inst >> 7) & 0x1fu),
                         12, true);

    decoded.valid = decoded.rs1 < 16 && decoded.rs2 < 16;
    return decoded;
  }
  case 0x6f: {  // jal
    const bool is_jal = (opcode == 0x6f);
    if (!is_jal) {
      return decoded;
    }

    decoded.alu_op = AluOp::JAL;
    decoded.imm = extend((((inst >> 31) & 0x1u) << 20) |
                             (((inst >> 12) & 0xffu) << 12) |
                             (((inst >> 20) & 0x1u) << 11) |
                             (((inst >> 21) & 0x3ffu) << 1),
                         21, true);

    decoded.valid = decoded.rd < 16;
    decoded.update_pc = true;
    return decoded;
  }
  case 0x67: {  // jalr
    const bool is_jalr = (opcode == 0x67) && (funct3 == 0x0);

    if (is_jalr) {
      decoded.alu_op = AluOp::JARL;
    }

    decoded.use_imm = true;
    decoded.imm = extend(inst >> 20, 12, true);

    decoded.valid = decoded.rd < 16;
    decoded.update_pc = true;
    return decoded;
  }
  default:
    return decoded;
  }


}

// 执行阶段，同时完成结果写回。无符号运算自然保留低 32 位。
bool execute(const DecodedInst &decoded) {
  if (!decoded.valid) {
    return false;
  }

  const uint32_t src1 = gpr[decoded.rs1];
  const uint32_t src2 = decoded.use_imm ? decoded.imm : gpr[decoded.rs2];
  uint32_t result = 0;

  if (decoded.memory_op == MemoryOp::LOAD) {
    const uint32_t address = src1 + decoded.imm;
    if (!load(address, decoded.funct3, result)) {
      return false;
    }
    if (decoded.rd != 0) {
      gpr[decoded.rd] = result;
    }
    gpr[0] = 0;
    return true;
  }

  if (decoded.memory_op == MemoryOp::STORE) {
    const uint32_t address = src1 + decoded.imm;
    if (!store(address, decoded.funct3, gpr[decoded.rs2])) {
      return false;
    }
    gpr[0] = 0;
    return true;
  }

  switch (decoded.alu_op) {
  case AluOp::ADD:
    result = src1 + src2;
    break;
  case AluOp::SUB:
    result = src1 - src2;
    break;
  case AluOp::AND:
    result = src1 & src2;
    break;
  case AluOp::OR:
    result = src1 | src2;
    break;
  case AluOp::XOR:
    result = src1 ^ src2;
    break;
  case AluOp::JAL:
    result = pc + 4;
    next_pc = pc + decoded.imm;
    break;
  case AluOp::JARL:
    result = pc + 4;
    next_pc = src1 + src2;
    break;
  }

  if (decoded.rd != 0) {
    gpr[decoded.rd] = result;
  }
  gpr[0] = 0;
  return true;
}

// PC 更新阶段。目前没有分支和跳转，因此顺序执行下一条指令。
void update_pc(const DecodedInst &decoded) {
  if(decoded.update_pc){
    pc = next_pc;
  }
  else{
    pc += 4;
  }
}

// 执行一条指令。
bool cpu_step() {
  uint32_t inst = 0;
  if (!fetch(inst)) {
    return false;
  }

  // EBREAK 的固定编码为 0x00100073，执行到这里立即正常退出。
  if (inst == 0x00100073u) {
    std::printf("ebreak: exit cpu\n");
    std::exit(0);
  }

  const DecodedInst decoded = decode(inst);
  if (!execute(decoded)) {
    std::printf("illegal instruction: pc = 0x%08x, inst = 0x%08x\n", pc,
                inst);
    return false;
  }

  update_pc(decoded);
  return true;
}

// ===================== tools ======================
void ref_get_regs(){

}

// ====================== end =======================
