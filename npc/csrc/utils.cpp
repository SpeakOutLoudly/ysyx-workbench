// 用于 DiffTest 的工具文件
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include "defines.h"

// 从 *.bin 文件中加载指令
bool load_img(uint8_t pmem[], size_t &nread, const char *filename){
  FILE *fp = std::fopen(filename, "rb");
  if(fp == nullptr){
    std::perror(filename);
    std::exit(EXIT_FAILURE);
  }

  std::fseek(fp, 0, SEEK_END);
  long fsize = std::ftell(fp);
  std::rewind(fp);

  if(static_cast<uint32_t>(fsize) > PMEM_SIZE || fsize < 0){
    std::fprintf(stderr, "bin file too large: %ld bytes\n", fsize);
    std::fclose(fp);
    std::exit(EXIT_FAILURE);
  }

  nread = std::fread(pmem, 1, static_cast<size_t>(fsize), fp);
  std::fclose(fp);

  if (nread != static_cast<size_t>(fsize)) {
      std::fprintf(stderr, "failed to read complete bin file\n");
      std::exit(EXIT_FAILURE);
  }

  return true;

}


// 从文本文件读取 ROM。inst.txt 中每条指令使用一行十六进制数表示。
bool load_rom(uint32_t rom[], uint32_t &rom_size, const char *filename = "inst.txt") {
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
  return true;
}
