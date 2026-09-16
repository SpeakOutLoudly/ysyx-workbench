#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

extern bool load_rom(uint32_t rom[], uint32_t &rom_size, uint32_t &pc, const char *filename);

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
  LW,
  SW,
};

struct DecodedInst {
  AluOp alu_op = AluOp::ADD;
  uint32_t rd = 0;
  uint32_t rs1 = 0;
  uint32_t rs2 = 0;
  uint32_t imm = 0;
  bool use_imm = false;
  bool valid = false;

  bool update_pc = false;
  MemoryOp memory_op = MemoryOp::NONE;
};

// Memory 访问阶段。RV32I 使用小端序，本简易实现只允许 4 字节对齐访问。
bool load_word(uint32_t address, uint32_t &data) {
  if ((address & 0x3u) != 0 || address > MEMORY_SIZE - 4) {
    std::printf("lw memory error: address = 0x%08x\n", address);
    return false;
  }

  data = static_cast<uint32_t>(memory[address]) |
         (static_cast<uint32_t>(memory[address + 1]) << 8) |
         (static_cast<uint32_t>(memory[address + 2]) << 16) |
         (static_cast<uint32_t>(memory[address + 3]) << 24);
  return true;
}

bool store_word(uint32_t address, uint32_t data) {
  if ((address & 0x3u) != 0 || address > MEMORY_SIZE - 4) {
    std::printf("sw memory error: address = 0x%08x\n", address);
    return false;
  }

  memory[address] = static_cast<uint8_t>(data);
  memory[address + 1] = static_cast<uint8_t>(data >> 8);
  memory[address + 2] = static_cast<uint8_t>(data >> 16);
  memory[address + 3] = static_cast<uint8_t>(data >> 24);
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

  switch (opcode) {
  case 0x13: { // OP-IMM
    const bool is_addi = (opcode == 0x13) && (funct3 == 0x0);
    const bool is_xori = (opcode == 0x13) && (funct3 == 0x4);
    const bool is_ori = (opcode == 0x13) && (funct3 == 0x6);
    const bool is_andi = (opcode == 0x13) && (funct3 == 0x7);

    decoded.use_imm = true;
    decoded.imm = inst >> 20;
    if ((decoded.imm & 0x800u) != 0) {
      decoded.imm |= 0xfffff000u;
    }

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
    if (!is_lw) {
      return decoded;
    }

    decoded.memory_op = MemoryOp::LW;
    decoded.use_imm = true;
    decoded.imm = inst >> 20;
    if ((decoded.imm & 0x800u) != 0) {
      decoded.imm |= 0xfffff000u;
    }

    decoded.valid = decoded.rd < 16 && decoded.rs1 < 16;
    return decoded;
  }
  case 0x23: { // STORE
    const bool is_sw = (opcode == 0x23) && (funct3 == 0x2);
    if (!is_sw) {
      return decoded;
    }

    decoded.memory_op = MemoryOp::SW;
    decoded.use_imm = true;
    decoded.imm = ((inst >> 25) << 5) | ((inst >> 7) & 0x1fu);
    if ((decoded.imm & 0x800u) != 0) {
      decoded.imm |= 0xfffff000u;
    }

    decoded.valid = decoded.rs1 < 16 && decoded.rs2 < 16;
    return decoded;
  }
  case 0x6f: {  // jal
    const bool is_jal = (opcode == 0x6f);
    if (!is_jal) {
      return decoded;
    }

    decoded.alu_op = AluOp::JAL;
    decoded.imm = (((inst >> 31) & 0x1u) << 20) |
                  (((inst >> 12) & 0xffu) << 12) |
                  (((inst >> 20) & 0x1u) << 11) |
                  (((inst >> 21) & 0x3ffu) << 1);
    if ((decoded.imm & 0x100000u) != 0) {
      decoded.imm |= 0xffe00000u;
    }

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
    decoded.imm = inst >> 20;
    if ((decoded.imm & 0x800u) != 0) {
      decoded.imm |= 0xfffff000u;
    }

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

  if (decoded.memory_op == MemoryOp::LW) {
    const uint32_t address = src1 + decoded.imm;
    if (!load_word(address, result)) {
      return false;
    }
    if (decoded.rd != 0) {
      gpr[decoded.rd] = result;
    }
    gpr[0] = 0;
    return true;
  }

  if (decoded.memory_op == MemoryOp::SW) {
    const uint32_t address = src1 + decoded.imm;
    if (!store_word(address, gpr[decoded.rs2])) {
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
