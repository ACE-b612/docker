#include "../include/common.h"
#include <elf.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ITRACE: 指令轨迹跟踪 (Ring Buffer)
// ============================================================================
#define IRINGBUF_SIZE 16

typedef struct {
    word_t pc;
    uint32_t inst;
} ItraceNode;

static ItraceNode iringbuf[IRINGBUF_SIZE];
static int iringbuf_p = 0;

extern "C" {

void iringbuf_write(uint32_t pc, uint32_t inst){
    iringbuf[iringbuf_p].pc = pc;
    iringbuf[iringbuf_p].inst = inst;
    iringbuf_p = (iringbuf_p + 1) % IRINGBUF_SIZE;
}

void iringbuf_display(){
    #ifdef CONFIG_ITRACE
    printf("=== IRingBuf Output ===\n");

    if (iringbuf_p == 0 && iringbuf[0].pc == 0) {
        printf("  (Ring buffer is empty)\n");
        return;
    }

    int last_idx = (iringbuf_p - 1 + IRINGBUF_SIZE) % IRINGBUF_SIZE;

    for (int i = 0; i < IRINGBUF_SIZE; i++) {
        int idx = (iringbuf_p + i) % IRINGBUF_SIZE;
        
        if (iringbuf[idx].pc == 0 && iringbuf[idx].inst == 0) continue;

        char buf[128];
        disassemble(buf, sizeof(buf), iringbuf[idx].pc, (uint8_t *)&iringbuf[idx].inst, 4);
        
        if (idx == last_idx) {
            printf("--> 0x%08x: %s\n", iringbuf[idx].pc, buf);
        } else {
            printf("    0x%08x: %s\n", iringbuf[idx].pc, buf);
        }
    }
    printf("=======================\n");
    #endif
}

} // extern "C"


// ============================================================================
// MTRACE: 内存读写跟踪
// ============================================================================
extern "C" {

void pread_display(uint32_t addr, int len){
    #ifdef CONFIG_MTRACE
    printf(ANSI_FMT("READ AT PC= ", ANSI_FG_BLUE) FMT_PADDR " len=%d\n", addr, len);
    #endif
}

void pwrite_display(uint32_t addr, int len){
    #ifdef CONFIG_MTRACE
    printf(ANSI_FMT("WRITE AT PC= ", ANSI_FG_BLUE) FMT_PADDR " len=%d\n", addr, len);
    #endif
}

} // extern "C"


// ============================================================================
// FTRACE: 函数调用跟踪 (依赖 ELF 文件)
// ============================================================================

typedef struct {
    char name[64];
    word_t addr;
    uint32_t size;
} SymbolEntry;

static SymbolEntry *symbol_table = NULL;
static int symbol_count = 0;
static int ftrace_depth = 0;
static FILE *ftrace_fp = NULL;

static const char* cat_func_name(vaddr_t addr){
    for(int i=0; i<symbol_count; i++){
        if(addr >= symbol_table[i].addr && addr < (symbol_table[i].addr + symbol_table[i].size)){
            return symbol_table[i].name;
        }
    }
    return "???";
}

extern "C" {

void init_ftrace(const char *elf_file){
    #ifdef CONFIG_FTRACE
    if(elf_file == NULL) return;

    ftrace_fp = fopen("ftrace.log", "w");
    if(ftrace_fp == NULL){
        Log("Error: Can not create 'ftrace.log'");
        return;
    }

    FILE *fp = fopen(elf_file, "r");
    if(fp == NULL){
        Log("Error: Can not read elf_file: %s", elf_file);
        return;
    }

    Elf32_Ehdr elf_header;
    if(fread(&elf_header, sizeof(Elf32_Ehdr), 1, fp) <= 0){
        fclose(fp); return;
    }
    if(memcmp(elf_header.e_ident, ELFMAG, 4) != 0){
        Log("Error: Not a valid ELF file");
        fclose(fp); return;
    }

    fseek(fp, elf_header.e_shoff, SEEK_SET);
    Elf32_Shdr *shdr = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr) * elf_header.e_shnum);
    if(fread(shdr, sizeof(Elf32_Shdr), elf_header.e_shnum, fp) != elf_header.e_shnum){
        Log("ERROR: Failed to read Section Headers");
        free(shdr); fclose(fp); return;
    }

    int symtab_idx = -1;
    int strtab_idx = -1;
    for(int i=0; i<elf_header.e_shnum; i++){
        if(shdr[i].sh_type == SHT_SYMTAB){
            symtab_idx = i;
            strtab_idx = shdr[i].sh_link;
            break;
        }
    }

    if(symtab_idx != -1 && strtab_idx != -1){
        // 读取字符串表
        char *strtab_buff = (char *)malloc(shdr[strtab_idx].sh_size);
        fseek(fp, shdr[strtab_idx].sh_offset, SEEK_SET);
        if (fread(strtab_buff, shdr[strtab_idx].sh_size, 1, fp) != 1) {
            Log("ERROR: Failed to read String Table");
            // 这里为了简单没有做完备的资源释放
        }

        // 读取符号表
        int entries = shdr[symtab_idx].sh_size / sizeof(Elf32_Sym);
        Elf32_Sym *symtab_buff = (Elf32_Sym *)malloc(shdr[symtab_idx].sh_size);
        fseek(fp, shdr[symtab_idx].sh_offset, SEEK_SET);
        if(fread(symtab_buff, shdr[symtab_idx].sh_size, 1, fp) != 1){
            Log("ERROR: Failed to read Symbol Table");
        }

        // 提取函数符号
        symbol_table = (SymbolEntry *)malloc(sizeof(SymbolEntry) * entries);
        for(int i=0; i<entries; i++){
            if(ELF32_ST_TYPE(symtab_buff[i].st_info) == STT_FUNC){
                char *fname = strtab_buff + symtab_buff[i].st_name;
                strncpy(symbol_table[symbol_count].name, fname, 63);
                symbol_table[symbol_count].name[63] = '\0';
                symbol_table[symbol_count].addr = symtab_buff[i].st_value;
                symbol_table[symbol_count].size = symtab_buff[i].st_size;
                symbol_count++;
            }
        }
        Log("FTRACE: Successfully loaded %d function symbols.", symbol_count);
        
        free(strtab_buff);
        free(symtab_buff);
    }
    free(shdr);
    fclose(fp);
    #endif
}

void ftrace_write(int type, uint32_t pc, uint32_t target){
    #ifdef CONFIG_FTRACE
    if(ftrace_fp == NULL) return;

    // type: 1=Call, 0=Ret
    vaddr_t final_pc = (type == 1) ? target : pc;
    const char *funct_name = cat_func_name(final_pc);
    
    fprintf(ftrace_fp, "0x%08x: ", pc);
    
    if(type){ // Call
        for(int i=0; i<ftrace_depth; i++) fprintf(ftrace_fp, "  ");
        fprintf(ftrace_fp, "call [%s@0x%08x]\n", funct_name, target);
        ftrace_depth++;
    } else { // Ret
        if(ftrace_depth > 0) ftrace_depth--;
        for(int i=0; i<ftrace_depth; i++) fprintf(ftrace_fp, "  ");
        fprintf(ftrace_fp, "ret  [%s]\n", funct_name);
    }
    fflush(ftrace_fp);
    #endif
}

} // extern "C"


// ============================================================================
// DTRACE: 设备访问跟踪
// ============================================================================
extern "C" {

void dtrace_display(const char *name, int type, uint32_t addr, int len, uint32_t data) {
    #ifdef CONFIG_DTRACE
    printf("DTRACE: %s Device=[%-8s] Addr=" FMT_PADDR " Len=%d Data=" FMT_WORD "\n",
           (type == 0) ? ANSI_FMT("READ ", ANSI_FG_YELLOW) : ANSI_FMT("WRITE", ANSI_FG_CYAN),
           name ? name : "unknown",
           addr, 
           len, 
           data
    );
    #endif
}

} // extern "C"