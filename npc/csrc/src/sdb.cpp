#include"../include/common.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>

static bool is_batch_mode_ended = false;

static int cmd_c(char *args){
    if (is_batch_mode_ended) {
        return 0;
    }
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args){
    npc_state.state = NPC_QUIT;
    return -1;
}

static int cmd_si(char *args){
    int steps = 1;
    if(args != NULL) steps = atoi(args);
    cpu_exec(steps);
    return 0;
}

static int cmd_info(char *args){
    if (args && strcmp(args, "r") == 0) {
        isa_reg_display();
    }
    return 0;
}

static int cmd_x(char *args) {
    if (args == NULL) {
        printf("Usage: x N ADDR\n");
        return 0;
    }
    char *arg_n = strtok(NULL, " ");
    char *arg_addr = strtok(NULL, " ");
    if (!arg_n || !arg_addr) return 0;

    int n = atoi(arg_n);
    uint32_t addr;
    sscanf(arg_addr, "%x", &addr);

    printf("Memory at 0x%08x:\n", addr);
    for(int i=0; i<n; i++) {
        printf("0x%08x: 0x%08x\n", addr + i*4, pmem_read(addr + i*4));
    }
    return 0;
}

void sdb_mainloop(){
    char *str;
    while ((str = readline(ANSI_FG_GREEN "(npc) " ANSI_NONE)) != NULL) {
        if (strlen(str) == 0) { free(str); continue; }
        add_history(str);

        char *cmd = strtok(str, " ");
        if (cmd == NULL) { free(str); continue; }

        if (strcmp(cmd, "c") == 0) cmd_c(NULL);
        else if (strcmp(cmd, "q") == 0) { free(str); break; }
        else if (strcmp(cmd, "si") == 0) cmd_si(strtok(NULL, " "));
        else if (strcmp(cmd, "info") == 0) cmd_info(strtok(NULL, " "));
        else if (strcmp(cmd, "x") == 0) cmd_x(str);
        else printf("Unknown command '%s'\n", cmd);

        free(str);

        if (npc_state.state == NPC_QUIT) break;
    }
}