#include "poolrad/replay/slot_a_save_handler_prefix.h"
#include "poolrad/replay/slot_a_save_call_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u), *saved=malloc(0x100000u), code[9]={0}, call[4]={0};
    pr_u1c_3171_cpu c={0}, before;
    CHECK(r && saved); c.cs=0x1000; c.ip=0x0292; c.ss=0x3000; c.sp=0x100; c.bp=0x1234; c.ax=0xab09; c.flags=0x7202;
    before=c; memcpy(saved,r,0x100000u); CHECK(pr_slot_a_save_handler_prefix(r,0x100000u,code,8,&c)); CHECK(!memcmp(&c,&before,sizeof c) && !memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_save_handler_prefix(r,0x100000u,code,9,&c));
    CHECK(c.ip==0x029b && c.bp==0xfe && c.sp==0xf8 && c.ax==0xab02 && (c.flags&0x08d5u)==0x0004u);
    saved[0x300fe]=0x34; saved[0x300ff]=0x12; saved[0x300f8]=2; saved[0x300f9]=0xab; CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); memset(&c,0,sizeof c); c.cs=0x1000; c.ip=0x2f85; c.ss=0x3000; c.sp=0x100; c.ax=9;
    CHECK(!pr_slot_a_save_call_frame(r,0x100000u,call,4,&c)); CHECK(!pr_slot_a_save_handler_prefix(r,0x100000u,code,9,&c)); CHECK(c.ip==0x029b && c.sp==0xf4);
    free(saved); free(r); puts("SAVE handler prefix passed"); return 0;
}
