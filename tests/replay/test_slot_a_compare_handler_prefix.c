#include "poolrad/replay/slot_a_compare_handler_prefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    uint8_t *r=calloc(1,0x100000u),*saved=malloc(0x100000u),call[4]={0},body[10]={0}; pr_u1c_3171_cpu c={0},before;
    CHECK(r&&saved); c.cs=0x1000;c.ip=0x2f58;c.ss=0x3000;c.sp=0x8000;c.bp=0x1234;c.ax=0xab03;c.flags=0x7202;before=c;memcpy(saved,r,0x100000u);
    CHECK(pr_slot_a_compare_handler_prefix(r,0x100000u,call,3,body,10,&c)); CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    CHECK(!pr_slot_a_compare_handler_prefix(r,0x100000u,call,4,body,10,&c)); CHECK(c.ip==0x0130 && c.sp==0x7df4 && c.bp==0x7ffa && c.ax==0xab02 && (c.flags&0x08d5u)==4u);
    saved[0x37ffe]=0;saved[0x37fff]=0x10;saved[0x37ffc]=0x5c;saved[0x37ffd]=0x2f;saved[0x37ffa]=0x34;saved[0x37ffb]=0x12;saved[0x37df4]=2;saved[0x37df5]=0xab;CHECK(!memcmp(r,saved,0x100000u));
    memset(r,0,0x100000u); c=before;c.cs=c.ss=0x1000;c.sp=0x2f5a;before=c;memcpy(saved,r,0x100000u);CHECK(pr_slot_a_compare_handler_prefix(r,0x100000u,call,4,body,10,&c));CHECK(!memcmp(&c,&before,sizeof c)&&!memcmp(r,saved,0x100000u));
    free(saved);free(r);puts("COMPARE handler prefix passed");return 0;
}
