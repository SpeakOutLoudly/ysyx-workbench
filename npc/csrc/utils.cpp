// 用于 DiffTest 的工具文件
#include <cstdint>
#include <cinttypes>
#include <cstdio>

bool check_regs(){
    return false;
}

// 从文本文件读取 ROM。inst.txt 中每条指令使用一行十六进制数表示。
bool load_rom(uint32_t rom[], uint32_t &rom_size, uint32_t &pc, const char *filename = "inst.txt") {
  std::FILE *file = std::fopen(filename, "r");
  if (file == nullptr) {
    std::printf("cannot open ROM file: %s\n", filename);
    return false;
  }

  uint32_t loaded_rom[128] = {};
  uint32_t count = 0;
  uint32_t inst = 0;

  while (std::fscanf(file, "%" SCNx32, &inst) == 1) {
    if (count >= 128) {
      std::printf("ROM file contains more than 128 instructions\n");
      std::fclose(file);
      return false;
    }
    loaded_rom[count++] = inst;
  }

  if (!std::feof(file)) {
    std::printf("invalid instruction in ROM file: %s\n", filename);
    std::fclose(file);
    return false;
  }
  std::fclose(file);

  for (uint32_t i = 0; i < 128; ++i) {
    rom[i] = loaded_rom[i];
  }
  rom_size = count;
  pc = 0;
  return true;
}

extern "C" int pmem_read(int raddr) {
  // 总是读取地址为`raddr & ~0x3u`的4字节返回
}
extern "C" void pmem_write(int waddr, int wdata, char wmask) {
  // 总是往地址为`waddr & ~0x3u`的4字节按写掩码`wmask`写入`wdata`
  // `wmask`中每比特表示`wdata`中1个字节的掩码,
  // 如`wmask = 0x3`代表只写入最低2个字节, 内存中的其它字节保持不变
}

extern "C" int imem_read(int address){
  
}