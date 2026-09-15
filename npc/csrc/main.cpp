#include "Vtop.h"
#include "verilated.h"

int main(int argc, char **argv) {
    VerilatedContext context;
    context.commandArgs(argc, argv);

    Vtop top{&context};

    top.reset = 1;

    for (int cycle = 0; cycle < 20; cycle++) {
        top.clk = 0;
        top.eval();

        top.clk = 1;
        top.eval();

        if (cycle == 2)
            top.reset = 0;

        printf("cycle=%d count=%d\n",
               cycle, top.count);
    }

    return 0;
}