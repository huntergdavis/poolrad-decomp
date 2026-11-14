#include "poolrad/replay/slot_a_dispatch_save_select.h"
#include "poolrad/replay/slot_a_dispatch_first_branch.h"
#include "poolrad/replay/slot_a_dispatch_prologue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), code[0x47]={0}, first[4]={0}, prologue[6]={0};
    pr_u1c_3171_cpu c, before; unsigned i;
    CHECK(r && saved); memcpy(saved,r,0x100000u);
    for(i=0;i<256;i++) if(i!=9) {
        memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f3e; c.ax=(uint16_t)(0xab00u|i); c.flags=0xffff; before=c;
        CHECK(pr_slot_a_dispatch_save_select(r,0x100000u,code,sizeof code,&c));
        CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    }
    memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f3e; c.ax=0xab09; c.flags=0xffff; before=c;
    CHECK(pr_slot_a_dispatch_save_select(r,0x100000u,code,sizeof code-1,&c)); CHECK(!memcmp(&c,&before,sizeof c));
    CHECK(!pr_slot_a_dispatch_save_select(r,0x100000u,code,sizeof code,&c));
    CHECK(c.ip==0x2f85 && c.flags==(uint16_t)((0xffffu & ~0x08d5u)|0x44u) && c.ax==0xab09 && !memcmp(r,saved,0x100000u));
    memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f2d; c.ss=0x3000; c.ds=0x2000; c.sp=0x100; r[0x26f7f]=9;
    CHECK(!pr_slot_a_dispatch_prologue(r,0x100000u,prologue,sizeof prologue,&c));
    CHECK(!pr_slot_a_dispatch_first_branch(r,0x100000u,first,sizeof first,&c));
    CHECK(!pr_slot_a_dispatch_save_select(r,0x100000u,code,sizeof code,&c)); CHECK(c.ip==0x2f85);
    free(saved); free(r); puts("dispatcher SAVE selection passed"); return 0;
}
