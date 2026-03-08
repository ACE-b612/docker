#include"../include/common.h"
#include <string.h>

uint32_t cpu_gpr[32] = {0};

const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

extern "C" void set_gpr_value(int idx, int data) {
    if (idx > 0 && idx < 32) {
        cpu_gpr[idx] = data;
    }
}

void isa_reg_display() {
    printf(ANSI_FG_GREEN "--- Register Status ---" ANSI_NONE "\n");
    for (int i = 0; i < 32; i++) {
        printf("%-3s = 0x%08x\t", regs[i], cpu_gpr[i]);
        if ((i + 1) % 4 == 0) printf("\n");
    }
}