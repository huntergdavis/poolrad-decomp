#include "poolrad/replay/slot_a_save_runner_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[14]={0}; pr_u1c_3171_cpu c,before; unsigned v,j,k;
    CHECK(r&&saved);
    for(k=0;k<2;k++) for(v=0;v<256;v++) { unsigned ones=0; uint16_t expected; for(j=0;j<8;j++) ones+=(v>>j)&1u;
        memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x317f; c.ds=0x2000; c.flags=0xffff; r[0x2442e]=k?(uint8_t)v:0; r[0x249ff]=k?0xa5:(uint8_t)v; before=c; memcpy(saved,r,0x100000u);
        CHECK(pr_slot_a_save_runner_loop(r,0x100000u,code,13,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
        CHECK(!pr_slot_a_save_runner_loop(r,0x100000u,code,14,&c));
        if(k && !v) { unsigned o=4; expected=(uint16_t)((0xffffu&~0x08d5u)|(o%2u==0?4u:0u)|0x80u); }
        else expected=(uint16_t)((0xffffu&~0x08d5u)|(ones%2u==0?4u:0u)|(v==0?0x40u:0u)|(v&0x80u));
        CHECK(c.flags==expected && c.ip==((v || k)?0x31aa:0x318d) && !memcmp(r,saved,0x100000u));
    }
    free(saved); free(r); puts("SAVE runner loop tests passed"); return 0;
}
