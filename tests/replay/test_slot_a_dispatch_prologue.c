#include "poolrad/replay/slot_a_dispatch_prologue.h"
#include "poolrad/replay/slot_a_runner_first_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), code[6]={0};
    pr_u1c_3171_cpu c={0}, prior;
    CHECK(r && saved); c.cs=0x1000; c.ip=0x2f2d; c.ds=0x2000; c.ss=0x3000;
    c.sp=0x100; c.bp=0x1234; c.ax=0xabcd; c.flags=0x72d7; r[0x26f7f]=9;
    prior=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_dispatch_prologue(r,0x100000u,code,5,&c));
    CHECK(!memcmp(&c,&prior,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_dispatch_prologue(r,0x100000u,code,6,&c));
    CHECK(c.ip==0x2f33 && c.sp==0xfe && c.bp==0xfe && c.ax==0xab09 && c.flags==0x72d7);
    saved[0x300fe]=0x34; saved[0x300ff]=0x12; CHECK(!memcmp(r,saved,0x100000u));
    /* The pushed BP aliases the latch: the load sees the just-written byte. */
    memset(r,0,0x100000u); c.cs=0x1000; c.ip=0x2f2d; c.ds=0x2000; c.ss=0x2000;
    c.sp=0x6f81; c.bp=0x2244; c.ax=0xab00;
    CHECK(!pr_slot_a_dispatch_prologue(r,0x100000u,code,6,&c)); CHECK(c.ax==0xab44);
    {
        uint8_t runner_code[66]={0};
        memset(r,0,0x100000u); memset(&c,0,sizeof c);
        c.cs=0x1000; c.ip=0x3171; c.ds=0x2000; c.ss=0x3000; c.sp=0x100; c.bp=0x9876;
        r[0x30104]=0x14; r[0x30105]=0x99; r[0x249e1]=0x40; r[0x40014]=9;
        CHECK(pr_slot_a_runner_first_dispatch(r,0x100000u,runner_code,sizeof runner_code,&c)==PR_SLOT_A_RUNNER_FIRST_DISPATCH);
        CHECK(!pr_slot_a_dispatch_prologue(r,0x100000u,code,sizeof code,&c));
        CHECK(c.ip==0x2f33 && c.sp==0xf8 && c.bp==0xf8 && (c.ax&0xff)==9);
    }
    free(saved); free(r); puts("dispatcher prologue passed"); return 0;
}
