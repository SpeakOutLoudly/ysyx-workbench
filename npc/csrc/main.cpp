#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"
#include "defines.h"


#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>

#define ADDR_OFF 0x80000000

static constexpr uint64_t NPC_FREQ_HZ = 100000000;
static uint64_t sim_cycles = 0;

static uint64_t get_time() {
    return sim_cycles / (NPC_FREQ_HZ / 1000000);
}

uint8_t pmem[PMEM_SIZE] = {};
size_t nread;
int32_t code = -1;

// 用 cpu_step 来 difftest
extern bool cpu_step();
extern void ref_get_regs();
extern bool check_regs();
bool load_img(uint8_t pmem[], size_t &nread, const char *filename);

extern "C" int pmem_read(int raddr) {
  // 总是读取地址为`raddr & ~0x3u`的4字节返回
    const uint32_t raw_address = static_cast<uint32_t>(raddr);
    if (raw_address == 0x20000000u) {
        return static_cast<int>(static_cast<uint32_t>(get_time()));
    }
    if (raw_address == 0x20000004u) {
        return static_cast<int>(static_cast<uint32_t>(get_time() >> 32));
    }

    const uint32_t address = static_cast<uint32_t>(raddr) - ADDR_OFF;
    const uint32_t aligned_address = address & ~0x3u;

    if (aligned_address > PMEM_SIZE - 4) {
        std::fprintf(stderr,
                     "pmem_read out of bounds: address = 0x%08x, raddr = %d\n",
                     address, raddr);
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
    if (waddr == 0x10000000) {  // 写入UART
        fputc(wdata & 0xff, stderr);   // 在stdio.h中定义
        return;
    }

    const uint32_t address = static_cast<uint32_t>(waddr) - ADDR_OFF;
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
    ++sim_cycles;
}

int main(int argc, char **argv) {
    VerilatedContext context;
    context.commandArgs(argc, argv);
    context.traceEverOn(true);

    Vtop top{&context};

    VerilatedFstC trace;
    top.trace(&trace, 99);
    trace.open("test_wave.fst");

    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s IMAGE [ARGS...]\n", argv[0]);
        return 1;
    }
    // 处理 ROM 和 Mem
    load_img(pmem, nread, argv[1]);

    reset_cycle(top, context, trace);
    while(1) {
        clock_cycle(top, context, trace);

        if(top.ebreak == 1){
            code = static_cast<int32_t>(top.halt_code);
            if(code == 0){
                std::fprintf(stdout, "HIT GOOD TRAP\n");
            } else {
                std::fprintf(stdout, "HIT BAD TRAP\n");
            }
            break;
        }
    }
    top.final();
    trace.close();

    return code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
