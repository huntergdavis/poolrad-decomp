#include "poolrad/recovered/overlay_u7f_record_subtract_0020.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void put(uint8_t *ram,size_t at,uint16_t value)
{
    ram[at]=(uint8_t)value;ram[at+1u]=(uint8_t)(value >> 8u);
}
static int signed_word(unsigned value)
{
    return value<32768u ? (int)value : (int)value-65536;
}
static uint16_t expected_flags(unsigned left,unsigned right)
{
    unsigned i,ones=0,result=(left-right)&65535u;
    int difference=signed_word(left)-signed_word(right);
    uint16_t flags=0x7602u;
    for (i=0;i<8u;++i) ones+=(result >> i)&1u;
    if (left<right) flags|=1u;
    if (ones%2u==0u) flags|=4u;
    if (left%16u<right%16u) flags|=0x10u;
    if (result==0u) flags|=0x40u;
    if (result>=32768u) flags|=0x80u;
    if (difference<-32768 || difference>32767) flags|=0x800u;
    return flags;
}
int main(void)
{
    static const unsigned amounts[7]={0,1,15,16,32767,32768,65535};
    static const uint16_t bits[6]={1,4,0x10,0x40,0x80,0x800};
    uint8_t *ram=malloc(PR_U7F_0020_RAM_SIZE);
    unsigned value,k,cases=0,seen[6][2]={{0}};
    CHECK(ram!=NULL);memset(ram,0xA5,PR_U7F_0020_RAM_SIZE);
    for (value=0;value<65536u;++value) for (k=0;k<8u;++k) {
        unsigned amount=k<7u ? amounts[k] : value^0xA55Au;
        unsigned result=(value-amount)&65535u,i;
        pr_u7f_0020_cpu cpu={0x1234,0x3456,0x5678,0x789A,0x9ABC,
            0xBCDE,0x7777,0x8000,0x3000,0x0020,0x1000,0x1111,0x2000,0x7603};
        put(ram,0x28000,0x7000);put(ram,0x28002,0x3000);
        put(ram,0x28004,(uint16_t)amount);put(ram,0x28006,0x100);
        put(ram,0x28008,0x4000);put(ram,0x40202,(uint16_t)value);
        CHECK(pr_u7f_record_subtract_0020(ram,PR_U7F_0020_RAM_SIZE,&cpu)==PR_U7F_0020_OK);
        CHECK(ram[0x40202]==(result&255u) && ram[0x40203]==result/256u);
        CHECK(ram[0x40201]==0xA5 && ram[0x40204]==0xA5);
        CHECK(ram[0x27FFE]==0x77 && ram[0x27FFF]==0x77);
        CHECK(cpu.ax==result && cpu.di==0x100 && cpu.es==0x4000);
        CHECK(cpu.flags==expected_flags(value,amount));
        CHECK(cpu.bx==0x3456 && cpu.cx==0x5678 && cpu.dx==0x789A && cpu.si==0x9ABC);
        CHECK(cpu.ds==0x1000 && cpu.ss==0x2000 && cpu.bp==0x7777);
        CHECK(cpu.sp==0x800A && cpu.cs==0x3000 && cpu.ip==0x7000);
        for (i=0;i<6u;++i) seen[i][(cpu.flags&bits[i])!=0u]=1u;
        ++cases;
    }
    for (k=0;k<6u;++k) CHECK(seen[k][0] && seen[k][1]);
    /* Native-backed regression: RETF's second word pop wraps SP. */
    for (k=0xFFFDu;k<=0xFFFFu;++k) {
        pr_u7f_0020_cpu cpu={0x1234,0x3456,0x5678,0x789A,0x9ABC,
            0xBCDE,0x7777,(uint16_t)k,0x3000,0x0020,0x1000,0x1111,0x2000,0x7603};
        memset(ram,0xA5,PR_U7F_0020_RAM_SIZE);
        put(ram,0x20000u+k,0x7000);
        put(ram,0x20000u+(uint16_t)(k+2u),0x3000);
        put(ram,0x20000u+(uint16_t)(k+4u),1);
        put(ram,0x20000u+(uint16_t)(k+6u),0x100);
        put(ram,0x20000u+(uint16_t)(k+8u),0x4000);
        if (k>=0xFFFEu) put(ram,0x20000u+k+2u,0x4444);
        put(ram,0x40202,0x8000);
        CHECK(pr_u7f_record_subtract_0020(ram,PR_U7F_0020_RAM_SIZE,&cpu)==PR_U7F_0020_OK);
        CHECK(cpu.cs==0x3000 && cpu.ip==0x7000 && cpu.sp==(uint16_t)(k+10u));
        CHECK(cpu.ax==0x7FFF && cpu.flags==expected_flags(0x8000,1));
        CHECK(cpu.bp==0x7777 && cpu.di==0x100 && cpu.es==0x4000);
        CHECK(cpu.bx==0x3456 && cpu.cx==0x5678 && cpu.dx==0x789A && cpu.si==0x9ABC);
        CHECK(cpu.ds==0x1000 && cpu.ss==0x2000);
    }
    {
        pr_u7f_0020_cpu cpu={0},before=cpu;
        CHECK(pr_u7f_record_subtract_0020(NULL,PR_U7F_0020_RAM_SIZE,&cpu)==PR_U7F_0020_ARGUMENT);
        CHECK(pr_u7f_record_subtract_0020(ram,PR_U7F_0020_RAM_SIZE-1u,&cpu)==PR_U7F_0020_ARGUMENT);
        CHECK(pr_u7f_record_subtract_0020(ram,PR_U7F_0020_RAM_SIZE,NULL)==PR_U7F_0020_ARGUMENT);
        CHECK(memcmp(&before,&cpu,sizeof cpu)==0 && ram[0]==0xA5);
    }
    free(ram);
    printf("u7f +0020: %u independent cases, all input words and both states of all SUB flags passed\n",cases);
    return 0;
}
