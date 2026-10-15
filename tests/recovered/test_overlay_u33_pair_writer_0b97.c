#include "poolrad/recovered/overlay_u33_pair_writer_0b97.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void put(uint8_t *ram,size_t at,uint16_t value)
{
    ram[at]=(uint8_t)value;ram[at+1u]=(uint8_t)(value >> 8u);
}
static uint16_t expected_flags(uint8_t before,int carry)
{
    unsigned i,ones=0;
    uint8_t after=(uint8_t)(before+1u);
    uint16_t value=0x7602u;
    for (i=0;i<8u;++i) ones+=(after >> i)&1u;
    if (carry) value|=1u;
    if (ones%2u==0u) value|=4u;
    if (before%16u==15u) value|=0x10u;
    if (after==0u) value|=0x40u;
    if (after>=128u) value|=0x80u;
    if (before==127u) value|=0x800u;
    return value;
}
int main(void)
{
    uint8_t *ram=malloc(PR_U33_0B97_RAM_SIZE);
    unsigned x,y,seen[256]={0},carry_seen[2]={0},cases=0;
    CHECK(ram!=NULL);
    memset(ram,0xA5,PR_U33_0B97_RAM_SIZE);
    put(ram,0x16716,0x100);put(ram,0x16718,0x4000);
    for (x=0;x<256u;++x) for (y=0;y<256u;++y) {
        int column=x<128u?(int)x:(int)x-256;
        int row=y<128u?(int)y:(int)y-256;
        uint16_t first=(uint16_t)(0x100+50*row+column+7);
        uint16_t second=(uint16_t)(first+1u);
        uint16_t row_low=(uint16_t)(50*row),column_next=(uint16_t)(column+1);
        uint32_t product=(uint32_t)(uint16_t)row*50u;
        int carry=(uint32_t)(uint16_t)(0x100u+row_low)+column_next>0xFFFFu;
        uint8_t before=(uint8_t)(x+y);
        pr_u33_0b97_cpu cpu={0x1234,0x3456,0x5678,0x789A,0x9ABC,
            0xBCDE,0x7777,0x8000,0x3000,0x0B97,0x1000,0x1111,0x2000,0x7603};
        ram[0x40000u+first]=0xA5;ram[0x40000u+second]=0xA5;
        ram[0x29000]=before;
        put(ram,0x28000,0x7000);put(ram,0x28002,0x3000);
        put(ram,0x28004,0x9002);put(ram,0x28006,(uint16_t)(0xA500u|x));
        put(ram,0x28008,(uint16_t)(0x5A00u|y));
        CHECK(pr_u33_pair_writer_0b97(ram,PR_U33_0B97_RAM_SIZE,&cpu)==PR_U33_0B97_OK);
        CHECK(ram[0x40000u+first]==0x34 && ram[0x40000u+second]==0x35);
        CHECK(ram[0x29000]==(uint8_t)(before+1u));
        CHECK(cpu.ax==row_low && cpu.dx==(uint16_t)(product >> 16u));
        CHECK(cpu.cx==column_next && cpu.di==0x9002 && cpu.es==0x4000);
        CHECK(cpu.flags==expected_flags(before,carry));
        CHECK(cpu.bx==0x3456 && cpu.si==0x9ABC && cpu.ds==0x1000 && cpu.ss==0x2000);
        CHECK(cpu.bp==0x7777 && cpu.sp==0x800A && cpu.cs==0x3000 && cpu.ip==0x7000);
        seen[before]=1u;carry_seen[carry]=1u;++cases;
    }
    CHECK(carry_seen[0] && carry_seen[1]);
    for (x=0;x<256u;++x) CHECK(seen[x]);
    {
        pr_u33_0b97_cpu cpu={0},before=cpu;
        CHECK(pr_u33_pair_writer_0b97(NULL,PR_U33_0B97_RAM_SIZE,&cpu)==PR_U33_0B97_ARGUMENT);
        CHECK(pr_u33_pair_writer_0b97(ram,0u,&cpu)==PR_U33_0B97_ARGUMENT);
        CHECK(pr_u33_pair_writer_0b97(ram,PR_U33_0B97_RAM_SIZE,NULL)==PR_U33_0B97_ARGUMENT);
        CHECK(memcmp(&before,&cpu,sizeof cpu)==0);
    }
    free(ram);
    printf("u33 +0B97: %u coordinate pairs, all counter bytes and both carries passed\n",cases);
    return 0;
}
