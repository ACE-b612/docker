#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include "capstone/capstone.h"


typedef size_t (*dyn_cs_disasm_t)(csh handle, const uint8_t *code, size_t code_size, 
                                  uint64_t address, size_t count, cs_insn **insn);
typedef void   (*dyn_cs_free_t)(cs_insn *insn, size_t count);
typedef cs_err (*dyn_cs_open_t)(cs_arch arch, cs_mode mode, csh *handle);

static dyn_cs_disasm_t cs_disasm_dl = NULL;
static dyn_cs_free_t   cs_free_dl   = NULL;
static dyn_cs_open_t   cs_open_dl   = NULL;
static csh handle;

extern "C" { 

void init_disasm(const char *triple) {
    const char *so_path = "/root/riscvlab/npc/tools/capstone/repo/libcapstone.so.5";
    
    void *dl_handle = dlopen(so_path, RTLD_LAZY);
    if (!dl_handle) {
        dl_handle = dlopen("libcapstone.so.5", RTLD_LAZY);
        if (!dl_handle) {
            fprintf(stderr, "[Error] Failed to load capstone: %s\n", dlerror());
            exit(1);
        }
    }

    cs_open_dl   = (dyn_cs_open_t)  dlsym(dl_handle, "cs_open");
    cs_disasm_dl = (dyn_cs_disasm_t)dlsym(dl_handle, "cs_disasm");
    cs_free_dl   = (dyn_cs_free_t)  dlsym(dl_handle, "cs_free");

    assert(cs_open_dl && cs_disasm_dl && cs_free_dl);
    if (cs_open_dl(CS_ARCH_RISCV, (cs_mode)(CS_MODE_RISCV32 | CS_MODE_RISCVC), &handle) != CS_ERR_OK) {
        fprintf(stderr, "[Error] cs_open failed.\n");
        exit(1);
    }
}

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
    if (!cs_disasm_dl) return;

    cs_insn *insn;
    size_t count = cs_disasm_dl(handle, code, nbyte, pc, 0, &insn);

    if (count > 0) {
        int ret = snprintf(str, size, "%s", insn[0].mnemonic);
        if (insn[0].op_str[0] != '\0') {
            snprintf(str + ret, size - ret, "\t%s", insn[0].op_str);
        }
        cs_free_dl(insn, count);
    } else {
        snprintf(str, size, "???");
    }
}

} // extern "C"