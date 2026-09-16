#include <cstdint>
#include <cstdio>

extern uint32_t rom[128];
extern uint32_t rom_size;
extern uint32_t gpr[16];
extern uint32_t pc;

bool load_rom(uint32_t rom[], uint32_t &rom_size, uint32_t &pc,
              const char *filename);
bool cpu_step();

int main(int argc, char **argv) {
  const char *inst_file = argc > 1 ? argv[1] : "csrc/inst.txt";
  if (!load_rom(rom, rom_size, pc, inst_file)) {
    return 1;
  }

  // halt 位于 ROM 的最后一条指令。在执行 EBREAK 前验证 1 + ... + 10。
  const uint32_t halt_pc = (rom_size - 1) * 4;
  while (true) {
    if (pc == halt_pc && gpr[10] != 55) {
      std::printf("test failed: x10 = %u, expected 55\n", gpr[10]);
      return 1;
    }

    if (!cpu_step()) {
      return 1;
    }
  }
}
