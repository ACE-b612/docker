#include "../include/common.h"

extern "C" void npc_itrace(int pc, int next_pc, int inst) {
#ifdef CONFIG_ITRACE
    iringbuf_write(pc, inst);

    // char buf[128];
    // disassemble(buf, sizeof(buf), pc, (uint8_t *)&inst, 4);
    // printf("ITRACE: 0x%08x: %08x %s\n", pc, inst, buf);
#endif
}