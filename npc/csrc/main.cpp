#include "Vtop.h"
#include "Vtop___024root.h"
#include "verilated.h"
#include "verilated_fst_c.h"
#include "defines.h"
#include "minirvEMU.h"


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

bool load_img(uint8_t pmem[], size_t &nread, const char *filename);

struct NpcStoreEvent {
    bool valid = false;
    uint32_t address = 0;
    uint32_t data = 0;
    uint8_t mask = 0;
};

static NpcStoreEvent npc_store;

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
    npc_store = {true, static_cast<uint32_t>(waddr),
                 static_cast<uint32_t>(wdata), static_cast<uint8_t>(wmask)};
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

bool compare_state(const Vtop &top, const minirv::Emulator &ref,
                   const minirv::StepResult &step, uint64_t step_count) {
    bool match = true;
    if (top.debug_pc != ref.pc()) {
        std::fprintf(stderr,
                     "DIFFTEST step=%llu inst_pc=0x%08x inst=0x%08x: "
                     "PC npc=0x%08x ref=0x%08x\n",
                     static_cast<unsigned long long>(step_count), step.pc,
                     step.inst, top.debug_pc, ref.pc());
        match = false;
    }
    for (unsigned i = 1; i < 32; ++i) {
        // 寄存器堆没有复位；未写过的寄存器不具有可比较的初值。
        if (!ref.reg_valid(i)) continue;
        // 仿真专用：读取 Verilator 生成的寄存器堆层级，不改变综合接口。
        const uint32_t npc_reg =
            top.rootp->top__DOT__u_idu__DOT__regfile__DOT__rf[i];
        if (npc_reg != ref.reg(i)) {
            std::fprintf(stderr,
                         "DIFFTEST step=%llu inst_pc=0x%08x inst=0x%08x: "
                         "x%u npc=0x%08x ref=0x%08x\n",
                         static_cast<unsigned long long>(step_count), step.pc,
                         step.inst, i, npc_reg, ref.reg(i));
            match = false;
        }
    }

    if (npc_store.valid != step.store) {
        std::fprintf(stderr,
                     "DIFFTEST step=%llu: store enable npc=%u ref=%u\n",
                     static_cast<unsigned long long>(step_count),
                     npc_store.valid, step.store);
        match = false;
    } else if (step.store) {
        const unsigned offset = step.store_address & 3u;
        const uint8_t expected_mask =
            static_cast<uint8_t>(((1u << step.store_size) - 1u) << offset);
        const uint32_t expected_data = step.store_data << (offset * 8);
        uint32_t selected_bytes = 0;
        for (unsigned i = 0; i < step.store_size; ++i)
            selected_bytes |= 0xffu << ((offset + i) * 8);
        if (npc_store.address != step.store_address ||
            npc_store.mask != expected_mask ||
            ((npc_store.data ^ expected_data) & selected_bytes) != 0) {
            std::fprintf(stderr,
                         "DIFFTEST step=%llu: store npc=(0x%08x,0x%08x,0x%02x) "
                         "ref=(0x%08x,0x%08x,0x%02x)\n",
                         static_cast<unsigned long long>(step_count),
                         npc_store.address, npc_store.data, npc_store.mask,
                         step.store_address, expected_data, expected_mask);
            match = false;
        }
        if (step.store_address >= minirv::kPmemBase) {
            for (unsigned i = 0; i < step.store_size; ++i) {
                const uint32_t address = step.store_address + i;
                const uint8_t npc_byte = pmem[address - minirv::kPmemBase];
                const uint8_t ref_byte = ref.pmem_byte(address);
                if (npc_byte != ref_byte) {
                    std::fprintf(stderr,
                                 "DIFFTEST step=%llu: PMEM[0x%08x] "
                                 "npc=0x%02x ref=0x%02x\n",
                                 static_cast<unsigned long long>(step_count),
                                 address, npc_byte, ref_byte);
                    match = false;
                }
            }
        }
    }
    return match;
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
    // NPC 和参考模型使用同一份镜像，但各自维护独立的 PMEM。
    load_img(pmem, nread, argv[1]);

    minirv::Emulator ref;
    ref.reset(pmem, nread);

    reset_cycle(top, context, trace);
    bool passed = true;
    uint64_t step_count = 0;
    while (true) {
        if(!top.exec_valid){
            clock_cycle(top, context, trace);
            continue;
        }
        const minirv::StepResult step = ref.step(get_time());
        if (!step.ok) {
            std::fprintf(stderr,
                         "DIFFTEST reference error at pc=0x%08x inst=0x%08x: %s\n",
                         step.pc, step.inst, step.error.c_str());
            passed = false;
            break;
        }
        if (top.debug_pc != step.pc || top.debug_inst != step.inst) {
            std::fprintf(stderr,
                         "DIFFTEST step=%llu fetch mismatch: "
                         "npc=(pc=0x%08x inst=0x%08x) "
                         "ref=(pc=0x%08x inst=0x%08x)\n",
                         static_cast<unsigned long long>(step_count),
                         top.debug_pc, top.debug_inst, step.pc, step.inst);
            passed = false;
            break;
        }
        if (step.halted) {
            code = static_cast<int32_t>(top.halt_code);
            if (ref.reg_valid(10) && code != static_cast<int32_t>(ref.reg(10))) {
                std::fprintf(stderr, "DIFFTEST halt code mismatch: npc=%d ref=%d\n",
                             code, static_cast<int32_t>(ref.reg(10)));
                passed = false;
            }
            std::fprintf(stdout, code == 0 ? "HIT GOOD TRAP\n" : "HIT BAD TRAP\n");
            if (passed)
                std::fprintf(stdout, "DIFFTEST PASS: %llu instructions\n",
                             static_cast<unsigned long long>(step_count));
            break;
        }

        npc_store.valid = false;
        clock_cycle(top, context, trace);
        ++step_count;
        if (!compare_state(top, ref, step, step_count)) {
            passed = false;
            break;
        }
    }
    top.final();
    trace.close();
    if (passed && code == 0) {
        if (sim_cycles != 0) {
            std::fprintf(stdout, "IPC: %.4f (%llu instructions / %llu cycles)\n",
                         static_cast<double>(step_count) / sim_cycles,
                         static_cast<unsigned long long>(step_count),
                         static_cast<unsigned long long>(sim_cycles));
        } else {
            std::fprintf(stdout, "IPC: N/A (0 cycles)\n");
        }
    }
    if(passed) std::fprintf(stdout, "DIFFTEST: PASSED\n");
    else std::fprintf(stdout, "DIFFTEST: FAILED! code = %d\n", code);
    return passed && code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
