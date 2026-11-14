#include "poolrad/replay/slot_a_runner_opcode_fetch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[10]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); c.cs=0x3582; c.ip=0x319a; c.ds=0x2000; c.es=0x6300; c.di=0x1000; c.ax=0x991a; c.flags=0x7202; r[0x6401a]=3;
    before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_runner_opcode_fetch(r,0x100000u,code,9,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_runner_opcode_fetch(r,0x100000u,code,10,&c)); CHECK(c.ip==0x31a4 && c.di==0xa91a && c.ax==0x9903 && (c.flags&0x08d5u)==0x80 && r[0x26f7f]==3); saved[0x26f7f]=3; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x3582; c.ip=0x319a; c.ds=0x2000; c.es=0x3000; c.di=0x7fff; c.ax=1; CHECK(!pr_slot_a_runner_opcode_fetch(r,0x100000u,code,10,&c)); CHECK(c.di==0x8000 && (c.flags&0x08d5u)==0x894u);
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x3582; c.ip=0x319a; c.ds=0x2000; c.es=0x3000; c.di=0xffff; c.ax=1; CHECK(!pr_slot_a_runner_opcode_fetch(r,0x100000u,code,10,&c)); CHECK(c.di==0 && (c.flags&0x08d5u)==0x55u);
    free(saved); free(r); puts("runner opcode fetch passed"); return 0;
}
