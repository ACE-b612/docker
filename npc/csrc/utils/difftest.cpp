#include <dlfcn.h>
#include "../include/common.h"

typedef void (*difftest_memcpy_t)(uint32_t addr, void *buf, size_t n, bool direction);
typedef void (*difftest_regcpy_t)(void *dut, bool direction);
typedef void (*difftest_exec_t)(uint64_t n);
typedef void (*difftest_raise_intr_t)(uint64_t NO);
typedef void (*difftest_init_t)(int port);

static difftest_memcpy_t ref_difftest_memcpy = NULL;
static difftest_regcpy_t ref_difftest_regcpy = NULL;
static difftest_exec_t   ref_difftest_exec   = NULL;
static difftest_raise_intr_t ref_difftest_raise_intr = NULL;
static difftest_init_t   ref_difftest_init   = NULL;

typedef struct {
  uint32_t gpr[32];
  uint32_t pc;
} CPU_state;

extern uint32_t cpu_gpr[32];
extern NPCState npc_state; 
extern uint8_t pmem[]; 

static bool is_skip_ref = false; // 用于处理 skip 指令 (如外设访问时)

// ==========================================================
// 3. 初始化 DiffTest
// ==========================================================
void init_difftest(const char *ref_so_file, long img_size, int port) {
#ifdef CONFIG_DIFFTEST
    assert(ref_so_file != NULL);

    void *handle;
    handle = dlopen(ref_so_file, RTLD_LAZY);
    assert(handle);

    ref_difftest_memcpy = (difftest_memcpy_t)dlsym(handle, "difftest_memcpy");
    ref_difftest_regcpy = (difftest_regcpy_t)dlsym(handle, "difftest_regcpy");
    ref_difftest_exec   = (difftest_exec_t)  dlsym(handle, "difftest_exec");
    ref_difftest_raise_intr = (difftest_raise_intr_t)dlsym(handle, "difftest_raise_intr");
    ref_difftest_init   = (difftest_init_t)  dlsym(handle, "difftest_init");

    assert(ref_difftest_memcpy);
    assert(ref_difftest_regcpy);
    assert(ref_difftest_exec);
    assert(ref_difftest_raise_intr);
    assert(ref_difftest_init);

    printf("DiffTest: REF SO loaded from %s\n", ref_so_file);

    ref_difftest_init(port);
    
    // 把 NPC 的内存镜像拷贝给 NEMU
    // DIFFTEST_TO_REF (1) means DUT -> REF
    ref_difftest_memcpy(MEM_BASE, pmem, img_size, DIFFTEST_TO_REF);

    // 把 NPC 的寄存器状态拷贝给 NEMU
    CPU_state cpu_val = {0};
    for(int i = 0; i < 32; i++) cpu_val.gpr[i] = cpu_gpr[i];
    cpu_val.pc = 0x80000000; // 从 MEM_BASE 开始
    
    ref_difftest_regcpy(&cpu_val, DIFFTEST_TO_REF);
    
    printf("DiffTest: Init successful.\n");
#else
    printf("DiffTest: Disabled (Enable CONFIG_DIFFTEST in Makefile)\n");
#endif
}

void difftest_skip_ref() {
    is_skip_ref = true;
}

void difftest_step(uint32_t pc, uint32_t npc) {
#ifdef CONFIG_DIFFTEST
    CPU_state ref_r;

    if (is_skip_ref) {
        for(int i=0; i<32; i++) ref_r.gpr[i] = cpu_gpr[i];
        ref_r.pc = npc;
        ref_difftest_regcpy(&ref_r, DIFFTEST_TO_REF);
        
        is_skip_ref = false;
        return;
    }

    ref_difftest_exec(1);
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);

    bool mismatch = false;

    if (ref_r.pc != npc) {
        printf(ANSI_FG_RED "DiffTest Mismatch! PC expected 0x%08x but got 0x%08x" ANSI_NONE "\n", ref_r.pc, npc);
        mismatch = true;
    }

    for (int i = 0; i < 32; i++) {
        if (ref_r.gpr[i] != cpu_gpr[i]) {
            printf(ANSI_FG_RED "DiffTest Mismatch! Reg[%d] expected 0x%08x but got 0x%08x" ANSI_NONE "\n", i, ref_r.gpr[i], cpu_gpr[i]);
            mismatch = true;
        }
    }

    if (mismatch) {
        npc_state.state = NPC_ABORT;
        npc_state.halt_ret = 1;
    }
#endif
}