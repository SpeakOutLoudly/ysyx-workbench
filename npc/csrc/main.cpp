#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"


#include <cstdint>
#include <cstdio>
#include <cstdlib>

// 用 cpu_step 来 difftest
extern bool cpu_step();
extern void ref_get_regs();
extern bool check_regs();
extern bool load_rom(uint32_t rom[], uint32_t &rom_size, const char *filename);

uint32_t rom[256] = {};
int rom_size = 0;

extern "C" int pmem_read(int raddr) {
  // 总是读取地址为`raddr & ~0x3u`的4字节返回
    const uint32_t address = static_cast<uint32_t>(raddr);
    const uint32_t aligned_address = address & ~0x3u;

    if (aligned_address > PMEM_SIZE - 4) {
        std::fprintf(stderr,
                     "pmem_read out of bounds: address = 0x%08x\n",
                     address);
        std::abort();
    }

    const uint32_t data =
        static_cast<uint32_t>(pmem[aligned_address]) |
        (static_cast<uint32_t>(pmem[aligned_address + 1]) << 8) |
        (static_cast<uint32_t>(pmem[aligned_address + 2]) << 16) |
        (static_cast<uint32_t>(pmem[aligned_address + 3]) << 24);

    return static_cast<int>(data);
}
extern "C" void pmem_write(int waddr, int wdata, char wmask) {
  // 总是往地址为`waddr & ~0x3u`的4字节按写掩码`wmask`写入`wdata`
  // `wmask`中每比特表示`wdata`中1个字节的掩码,
  // 如`wmask = 0x3`代表只写入最低2个字节, 内存中的其它字节保持不变
    const uint32_t address = static_cast<uint32_t>(waddr);
    const uint32_t aligned = address & ~0x3u;
    const mask = static_cast<uint8_t>(wmask);
    const wdata = static_cast<uint32_t>(wdata);
    if (aligned_address > PMEM_SIZE - 4) {
        std::fprintf(stderr,
                     "pmem_write out of bounds: address = 0x%08x\n",
                     address);
        std::abort();
    }

    for(uint32_t t = 0; t < 4; t ++){
        if(mask & (1u << t) != 0){
            pmem[aligned + t] = static_cast<uint8_t>(wdata << (t * 8)); 
        }
    }
}

extern "C" int imem_read(int address){
    return rom[address / 4];
}

int main(int argc, char **argv) {
    VerilatedContext context;
    context.commandArgs(argc, argv);
    context.traceEverOn(true);

    Vtop top{&context};

    VerilatedFstC trace;
    top.trace(&trace, 99);
    trace.open("test_wave.fst");

    // 处理 ROM 和 Mem
    load_rom(rom, rom_size, "inst.txt");
    
    for (int i = 0; i < 100; i++) {
        int a = rand() & 1;
        int b = rand() & 1;
        top.a = a;
        top.b = b;
        top.eval();
        trace.dump(context.time());
        context.timeInc(1);
        printf("a = %d, b = %d, f = %d\n", a, b, top.f);
        assert(top.f == (a ^ b));
    }
    top.final();

    return 0;
}