#include "poolrad/replay/slot_a_save_word_helper.h"
#include "poolrad/replay/slot_a_save_word_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u), call[5]={0x9a,0x4d,0,0x34,0x12},stub[5]={0xea,0xaf,6,0,0x30},body[36]={0},args[8]={0};
    pr_u1c_3171_cpu c={0},before; pr_u25_0000_status st=PR_U25_0000_TRANSFER;
    CHECK(r&&saved); memcpy(r+0x102a8,call,5); memcpy(r+0x1238d,stub,5); c.cs=0x1000; c.ip=0x02a0; c.ds=0x2000; c.ss=0x4000; c.sp=0x8000; c.ax=0xab00; r[0x27087]=0x34; r[0x27047]=0x12;
    CHECK(!pr_slot_a_save_word_args(r,0x100000u,args,8,&c)); before=c; memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_save_word_helper(r,0x100000u,call,5,stub,5,body,35,&c,&st)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u)&&st==PR_U25_0000_TRANSFER);
    CHECK(!pr_slot_a_save_word_helper(r,0x100000u,call,5,stub,5,body,36,&c,&st));
    CHECK(st==PR_U25_0000_OK && c.cs==0x1000 && c.ip==0x02ad && c.sp==0x8000 && c.ax==0x1234);
    free(saved); free(r); puts("SAVE operand-word helper passed"); return 0;
}
