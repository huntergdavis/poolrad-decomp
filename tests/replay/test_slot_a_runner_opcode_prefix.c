#include "poolrad/replay/slot_a_runner_opcode_prefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
static void setup(uint8_t *r) { static const uint8_t code[]={0xa0,0x7f,0x6f,0xa2,0x7e,0x6f,0xa1,0xed,0x49,0xc4,0x3e,0xde,0x49}; memcpy(r+0x389ad,code,sizeof code); r[0x26f7f]=9; r[0x249ed]=0x1a; r[0x249ee]=0x99; r[0x249df]=0x10; r[0x249e1]=0x63; }
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u); pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); setup(r); c.cs=0x3582; c.ip=0x318d; c.ds=0x2000; c.ax=0xab00; c.flags=0x7246; before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_runner_opcode_prefix(r,0x100000u,&c)==0x319a); CHECK(c.ip==0x319a && c.ax==0x991a && c.di==0x1000 && c.es==0x6300 && r[0x26f7e]==9);
    memcpy(r,saved,0x100000u); c=before; r[0x389b3]^=1; CHECK(pr_slot_a_runner_opcode_prefix(r,0x100000u,&c)==0x3193); CHECK(c.ip==0x3193 && (c.ax&0xff)==9 && r[0x26f7e]==9);
    memcpy(r,saved,0x100000u); c=before; r[0x389ad]^=1; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_runner_opcode_prefix(r,0x100000u,&c)==0x318d); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved); free(r); puts("runner opcode prefix passed"); return 0;
}
