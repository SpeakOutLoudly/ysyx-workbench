#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// 用 cpu_step 来 difftest
extern bool cpu_step();
extern void ref_get_regs();
extern bool check_regs();

int main(int argc, char **argv) {
    VerilatedContext context;
    context.commandArgs(argc, argv);
    context.traceEverOn(true);

    Vtop top{&context};

    VerilatedFstC trace;
    top.trace(&trace, 99);
    trace.open("test_wave.fst");
    
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