#include"../include/common.h"
#include<sys/time.h>
#include<assert.h>

uint8_t pmem[MEM_SIZE];
#define SERIAL_PORT 0x10000000
#define RTC_ADDR    0xa0000048

static uint64_t boot_time = 0;

static uint64_t get_time_internal() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

static uint64_t get_time() {
    if (boot_time == 0) boot_time = get_time_internal();
    return get_time_internal() - boot_time;
}

inline bool check_addr(uint32_t addr,uint32_t &offset){
    if(addr<MEM_BASE || addr>=MEM_BASE+MEM_SIZE) return false;
    offset = addr-MEM_BASE;
    return true;
}

extern "C" int pmem_read(int raddr){

    if(raddr == RTC_ADDR) return (uint32_t)get_time();
    if(raddr == RTC_ADDR + 4) return (uint32_t)(get_time()>>32);

    uint32_t offset;
    if(!check_addr(raddr,offset)) {
        return 0;
    }
    #ifdef CONFIG_MTRACE
        pread_display(raddr, 4);
    #endif
    return *(uint32_t*)(pmem + (offset & ~0x3u));

    //=================
if (raddr >= 0xa0000000) {
    printf("MMIO READ: 0x%08x\n", raddr);
}

}


extern "C" void pmem_write(int waddr, int wdata, char wmask){
//===================================================
    if (waddr >= 0xa0000000) {
    printf("MMIO WRITE: 0x%08x data=0x%08x\n", waddr, wdata);
}
//=========================================================
    if(waddr == SERIAL_PORT){
        putchar((char)wdata);
        return;
    }

    uint32_t offset;
    if(!check_addr(waddr, offset)) {
        return;
    }

    #ifdef CONFIG_MTRACE
        int len = 4;
        if (wmask == 0x1 || wmask == 0x2 || wmask == 0x4 || wmask == 0x8) len = 1;
        else if (wmask == 0x3 || wmask == 0xC) len = 2;
        
        pwrite_display(waddr, len);
    #endif

    uint32_t offset_aligned = offset & ~0x3u;
    for(int i=0; i<4; i++){
        if(wmask & (1<<i)){
            pmem[offset_aligned + i] = (wdata >> (i * 8)) & 0xFF;
        }
    }
}

long load_image(const char *img_file){
    if(img_file == NULL) return 0;
    FILE *fp = fopen(img_file,"rb");
    if(fp==NULL){
        printf("Can not open '%s'\n", img_file);
        return 0;
    }

    fseek(fp,0,SEEK_END);
    long size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    if(size > MEM_SIZE) size = MEM_SIZE;

    int ret = fread(pmem,size,1,fp);
    assert(ret==1);
    fclose(fp);
    return size;
}