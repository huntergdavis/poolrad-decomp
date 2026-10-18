#include "poolrad/recovered/overlay_u7f_resource_subtract_0a01.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while (0)
static size_t physical(unsigned seg,unsigned off)
{
    return ((size_t)seg*16u+(off&65535u))&1048575u;
}
static void put(uint8_t *ram,unsigned seg,unsigned off,unsigned word)
{
    size_t at=physical(seg,off);ram[at]=(uint8_t)word;ram[(at+1u)&1048575u]=(uint8_t)(word/256u);
}
static unsigned get(const uint8_t *ram,unsigned seg,unsigned off)
{
    size_t at=physical(seg,off);return ram[at]+256u*ram[(at+1u)&1048575u];
}
static pr_u7f_0a01_cpu setup(uint8_t *ram,unsigned sp,unsigned index,unsigned amount,
                           unsigned record,unsigned segment,unsigned mode)
{
    pr_u7f_0a01_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                      0x7777,(uint16_t)sp,0x3000,0x0A01,0x1000,0x1111,0x2000,0x7603};
    put(ram,c.ss,sp,0x7000);put(ram,c.ss,sp+2u,c.cs);
    put(ram,c.ss,sp+4u,index|0xAB00u);put(ram,c.ss,sp+6u,amount);
    put(ram,c.ss,sp+8u,record);put(ram,c.ss,sp+10u,segment);
    ram[physical(c.ds,0x49F3)]=(uint8_t)mode;return c;
}
static unsigned parity(unsigned value)
{
    unsigned count=0,i;for(i=0;i<8u;++i)count+=(value>>i)&1u;
    return count%2u==0u ? 4u : 0u;
}
static unsigned final_flags(unsigned index,unsigned mode)
{
    unsigned flags=0x7602u;
    if(mode==1u || mode==6u) {
        flags|=0x10u; /* Native-pinned undefined SHL AF, checked without a mask. */
        unsigned result=(index*4u)&65535u;
        if(index>=128u)result=(result+64512u)&65535u;
        flags|=parity(result);
        if(index>=128u)flags|=0x81u;
        if(index==0u)flags|=0x40u;
    } else {
        unsigned result=(mode-1u)&255u;
        int signed_mode=mode<128u ? (int)mode : (int)mode-256;
        flags|=parity(result);
        if(mode==0u)flags|=1u;
        if(mode%16u==0u)flags|=0x10u;
        if(result>=128u)flags|=0x80u;
        if(signed_mode-1 < -128)flags|=0x800u;
    }
    return flags;
}
int main(void)
{
    uint8_t *ram=malloc(PR_U7F_0020_RAM_SIZE);unsigned index,mode,sp;
    CHECK(ram!=NULL);memset(ram,0xA5,PR_U7F_0020_RAM_SIZE);
    for(index=0;index<256u;++index)for(mode=0;mode<256u;++mode) {
        int signed_index=index<128u ? (int)index : (int)index-256;
        unsigned off=(unsigned)(0x9088+2*signed_index),acc=(unsigned)(0x67F4+4*signed_index);
        unsigned amount=index*256u+mode,first=0xA55Au^amount,base=0x8000u^index;
        unsigned expected_first,expected_base;uint32_t total=UINT32_C(0xDEADFFFF)+amount;
        pr_u7f_0a01_cpu c=setup(ram,0x8000,index,amount,0x9000,0x4000,mode);
        put(ram,0x4000,off,first);put(ram,0x4000,0x9102,base);
        put(ram,c.ds,acc,0xFFFF);put(ram,c.ds,acc+2u,0xDEAD);
        expected_first=((index==61u ? base : first)-amount)&65535u;
        expected_base=((index==61u ? expected_first : base)-amount)&65535u;
        CHECK(pr_u7f_resource_subtract_0a01(ram,PR_U7F_0020_RAM_SIZE,&c)==PR_U7F_0A01_OK);
        CHECK(get(ram,0x4000,off)==(index==61u ? expected_base : expected_first));
        CHECK(get(ram,0x4000,0x9102)==expected_base);
        CHECK(c.bp==0x7777 && c.sp==0x800C && c.cs==0x3000 && c.ip==0x7000);
        CHECK(c.si==0x9ABC && c.ds==0x1000 && c.ss==0x2000 && c.es==0x4000);
        CHECK(c.flags==final_flags(index,mode));
        if(mode==1u || mode==6u) {
            CHECK(get(ram,c.ds,acc)==(total&65535u) && get(ram,c.ds,acc+2u)==total/65536u);
            CHECK(c.ax==(uint16_t)signed_index && c.di==(uint16_t)(signed_index*4));
            CHECK(c.cx==(total&65535u) && c.bx==total/65536u && c.dx==total/65536u);
        } else {
            CHECK(get(ram,c.ds,acc)==0xFFFF && get(ram,c.ds,acc+2u)==0xDEAD);
            CHECK(c.ax==expected_base && c.di==0x9000 && c.dx==expected_first);
            CHECK(c.bx==0x3456 && c.cx==0x5678);
        }
    }
    for(sp=0xFFFD;sp<=0xFFFFu;++sp) {
        pr_u7f_0a01_cpu c=setup(ram,sp,0,1,0x9000,0x4000,0);
        put(ram,0x4000,0x9088,0x8000);put(ram,0x4000,0x9102,0x8000);
        if(sp>=0xFFFEu)put(ram,0x3000,sp+2u,0x4444); /* linear CS decoy */
        CHECK(pr_u7f_resource_subtract_0a01(ram,PR_U7F_0020_RAM_SIZE,&c)==PR_U7F_0A01_OK);
        CHECK(c.cs==0x3000 && c.ip==0x7000 && c.sp==(uint16_t)(sp+12u));
        CHECK(c.ax==0x7FFF && c.dx==0x7FFF && c.flags==final_flags(0,0));
    }
    {
        unsigned record=(0x8000u-12u-0x102u)&65535u;
        pr_u7f_0a01_cpu c=setup(ram,0x8000,0,0xFFBD,record,0x2000,6);
        put(ram,0x2000,record+0x88u,0x8000);put(ram,c.ds,0x67F4,0x1234);
        CHECK(pr_u7f_resource_subtract_0a01(ram,PR_U7F_0020_RAM_SIZE,&c)==PR_U7F_0A01_OK);
        CHECK(c.ip==0x7000 && c.ax==0x0A79 && c.cx==0x5678 && c.bx==0x3456);
        CHECK(get(ram,c.ds,0x67F4)==0x1234); /* overwritten return skips natural accumulator */
        c=setup(ram,0x8000,0,0x20,record,0x2000,6);
        CHECK(pr_u7f_resource_subtract_0a01(ram,PR_U7F_0020_RAM_SIZE,&c)==PR_U7F_0A01_CONTROL_EXIT);
        CHECK(c.ip==0x0A16 && c.cs==0x3000 && c.sp==0x7FFE);
    }
    {
        /* The callee's record+102 write aliases DS:49F3. Mode 6 becomes 5,
         * so the later accumulator branch must observe the changed byte. */
        pr_u7f_0a01_cpu c=setup(ram,0x8000,0,1,0x48F1,0x1000,6);
        put(ram,c.ds,0x4979,0x1000);put(ram,c.ds,0x49F3,0x8006);
        put(ram,c.ds,0x67F4,0xFFFF);put(ram,c.ds,0x67F6,0x1234);
        CHECK(pr_u7f_resource_subtract_0a01(ram,PR_U7F_0020_RAM_SIZE,&c)==PR_U7F_0A01_OK);
        CHECK(get(ram,c.ds,0x49F3)==0x8005 && get(ram,c.ds,0x4979)==0x0FFF);
        CHECK(get(ram,c.ds,0x67F4)==0xFFFF && get(ram,c.ds,0x67F6)==0x1234);
        CHECK(c.ax==0x8005 && c.dx==0x0FFF && c.flags==final_flags(0,5));
        CHECK(c.bx==0x3456 && c.cx==0x5678 && c.ip==0x7000 && c.sp==0x800C);
    }
    free(ram);puts("u7f +0A01: 65536 independent index/mode cases, natural effects, wrap and nested return aliases passed");
    return 0;
}
