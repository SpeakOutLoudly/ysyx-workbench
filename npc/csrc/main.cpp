#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"


#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>


// 用 cpu_step 来 difftest
extern bool cpu_step();
extern void ref_get_regs();
extern bool check_regs();
extern bool load_rom(uint32_t rom[], uint32_t &rom_size, const char *filename);

constexpr uint32_t PMEM_SIZE = 4096;
uint8_t pmem[PMEM_SIZE] = {};

uint32_t rom[256] = {};
uint32_t rom_size = 0;

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
    const uint8_t mask = static_cast<uint8_t>(wmask);
    const uint32_t data = static_cast<uint32_t>(wdata);
    if (aligned > PMEM_SIZE - 4) {
        std::fprintf(stderr,
                     "pmem_write out of bounds: address = 0x%08x\n",
                     address);
        std::abort();
    }

    for(uint32_t t = 0; t < 4; t ++){
        if((mask & (1u << t)) != 0){
            pmem[aligned + t] = static_cast<uint8_t>(data >> (t * 8)); 
        }
    }
}

extern "C" int imem_read(int address){
    return rom[address / 4];
}

void eval_and_dump(
    Vtop &top,
    VerilatedContext &context,
    VerilatedFstC &trace
) {
    top.eval();
    trace.dump(context.time());
    context.timeInc(1);
}

void reset_cycle(Vtop &top, VerilatedContext &context, VerilatedFstC &trace){
    top.reset = 1;
    top.clk = 0;
    eval_and_dump(top, context, trace);

    top.clk = 1;
    eval_and_dump(top, context, trace);

    top.reset = 0;
    eval_and_dump(top, context, trace);
}

void clock_cycle(Vtop &top, VerilatedContext &context, VerilatedFstC &trace){
    top.clk = 0;
    eval_and_dump(top, context, trace);

    top.clk = 1;
    eval_and_dump(top, context, trace);
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
    load_rom(rom, rom_size, "csrc/inst.txt");

    reset_cycle(top, context, trace);
    for (int i = 0; i < 100; i++) {
        clock_cycle(top, context, trace);

        if(top.ebreak == 1){
            std::fprintf(stdout, "cpu finish!\n");
            break;
        }
    }
    top.final();
    trace.close();

    return 0;
}