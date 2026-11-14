#include "poolrad/replay/slot_a_save_tag_branch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),code[10]={0}; pr_u1c_3171_cpu c,before; unsigned i,j;
    CHECK(r&&saved);
    for(i=0;i<256;i++) { unsigned ones=0,res=(i-128u)&255u; uint16_t expected;
        for(j=0;j<8;j++) ones+=(res>>j)&1u;
        memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x02ad; c.ds=0x2000; c.ss=0x3000; c.bp=0x100; c.ax=0x1234; c.flags=0xffff; r[0x27006]=(uint8_t)i;
        before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_tag_branch(r,0x100000u,code,9,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
        CHECK(!pr_slot_a_save_tag_branch(r,0x100000u,code,10,&c));
        expected=(uint16_t)((0xffffu&~0x08d5u)|(i<128u?0x801u:0u)|((ones%2u)==0u?4u:0u)|(res==0?0x40u:0u)|(res&0x80u));
        CHECK(c.flags==expected && c.ip==(i<128u?0x02b7u:0x02cfu)); saved[0x300fc]=0x34; saved[0x300fd]=0x12; CHECK(!memcmp(r,saved,0x100000u));
    }
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x02ad; c.ds=c.ss=0x2000; c.bp=0x700a; c.ax=0x1280;
    CHECK(!pr_slot_a_save_tag_branch(r,0x100000u,code,10,&c)); CHECK(c.ip==0x02cf && r[0x27006]==0x80);
    free(saved); free(r); puts("SAVE tag branch passed"); return 0;
}
