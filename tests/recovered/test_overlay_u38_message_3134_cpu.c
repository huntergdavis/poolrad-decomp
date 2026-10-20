#include "poolrad/recovered/overlay_u38_message_3134_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Synthetic semantic checks of the composition seams. The full 1693 closure
 * is exercised against the original bytes by the Unicorn differential; here
 * 1693's first nested CALL site is left unbound, which stops it with CODE after
 * its StrStore, so every byte this body and 0B0B wrote is observable. */
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define SIZE PR_BUS_RAM_SIZE
static unsigned char ram[SIZE];
static pr_ega ega;
static size_t addr(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(SIZE-1u); }
static void put(unsigned s,unsigned o,unsigned v) {
    size_t a=addr(s,o);ram[a]=(unsigned char)v;ram[(a+1u)&(SIZE-1u)]=(unsigned char)(v>>8);
}
static unsigned get(unsigned s,unsigned o) { return ram[addr(s,o)]|((unsigned)ram[(addr(s,o)+1u)&(SIZE-1u)]<<8); }
static const pr_u38_3134_limits limits={100,100,100,100,100};
static pr_u38_3134_cpu fixture(unsigned gate,unsigned b_byte,unsigned a_seg,unsigned a_off) {
    pr_u38_3134_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x3000,0x3134,0x5000,0x1111,0x2000,0x0202};
    unsigned i;
    memset(ram,0xA5,sizeof ram);pr_ega_reset(&ega);
    put(c.ss,c.sp,0x7000);put(c.ss,c.sp+2,0x9001);put(c.ss,c.sp+4,gate|0xA500u);
    put(c.ss,c.sp+6,a_off);put(c.ss,c.sp+8,a_seg);put(c.ss,c.sp+10,0x4000);put(c.ss,c.sp+12,0x1000);
    ram[addr(0x1000,0x4010)]=(unsigned char)b_byte;
    ram[addr(c.cs,0x311B)]=24;
    for(i=0;i<24u;++i)ram[addr(c.cs,0x311C+i)]=(unsigned char)('a'+i);
    ram[addr(c.cs,0x316A)]=0x9A;put(c.cs,0x316B,0x0B0B);put(c.cs,0x316D,0x6000);
    ram[addr(c.cs,0x316F)]=0x9A;put(c.cs,0x3170,0x1693);put(c.cs,0x3172,0x8200);
    ram[addr(0x8200,0x16A7)]=0x9A;put(0x8200,0x16A8,0x0B25);put(0x8200,0x16AA,0x6000);
    ram[addr(0x1000,0x0200+0x34)]=0x77;
    return c;
}
static void same_ram_except(const unsigned char *copy,size_t lo,size_t hi,size_t skip_lo,size_t skip_hi) {
    size_t i;
    for(i=lo;i<hi;++i)if(i<skip_lo||i>=skip_hi)CHECK(copy[i]==ram[i]);
}
int main(void) {
    pr_bus bus={ram,&ega,NULL,NULL,NULL};
    pr_u38_3134_diagnostic d;pr_u38_3134_cpu c,before;
    unsigned char *copy=malloc(SIZE);
    unsigned gate,i,bad;
    CHECK(copy!=NULL);
    /* Gate paths: nonzero low byte, and B byte at or above 13h, return without writing A. */
    for(gate=1;gate<256u;gate+=51u) {
        c=fixture(gate,0x12,0x1000,0x0200);memcpy(copy,ram,SIZE);
        CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800E && c.bp==0x7777);
        CHECK(c.ax==0x0200 && get(0x2000,0x7FFA)==0x0200 && get(0x2000,0x7FFC)==0x1000);
        CHECK(c.dx==0x1000 && c.es==0x1111 && c.di==0xBCDE && d.callee==-1);
        CHECK(ram[addr(0x1000,0x0234)]==0x77);
        CHECK((c.flags&0x0041u)==0u && (c.flags&0x0001u)==0u);  /* CMP gate,0 of a nonzero byte */
        CHECK(memcmp(copy,ram,SIZE)!=0);                       /* the frame words below SP were written */
        same_ram_except(copy,0,SIZE,addr(0x2000,0x7FDE),addr(0x2000,0x8000));
    }
    c=fixture(0,0x13,0x1000,0x0200);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_OK);
    CHECK(c.es==0x1000 && c.di==0x4000 && (c.flags&0x0041u)==0x0040u && ram[addr(0x1000,0x0234)]==0x77);
    c=fixture(0,0xFF,0x1000,0x0200);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_OK);
    CHECK((c.flags&0x0041u)==0u && ram[addr(0x1000,0x0234)]==0x77);
    /* Message path: A byte cleared, literal copied to SS:BP-1Dh, 1693 stops at its unbound call. */
    c=fixture(0,0x12,0x1000,0x0200);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CALLEE);
    CHECK(d.callee==PR_U38_3134_DISPLAY && d.status==PR_R00BA_1693_CODE && d.display.callee==0);
    CHECK(ram[addr(0x1000,0x0234)]==0);
    CHECK(ram[addr(0x2000,0x7FE1)]==24);
    for(i=0;i<24u;++i)CHECK(ram[addr(0x2000,0x7FE1+1u+i)]==(unsigned char)('a'+i));
    CHECK(c.cs==0x8200 && c.ip==0x16B8);
    /* A aliases the literal's length byte: an empty string is copied. */
    c=fixture(0,0x12,0x3000,(0x311Bu-0x34u)&65535u);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CALLEE);
    CHECK(ram[addr(0x3000,0x311B)]==0 && ram[addr(0x2000,0x7FE1)]==0);
    /* A offset wraps inside its segment. */
    c=fixture(0,0x12,0x1000,0xFFFF);ram[addr(0x1000,0x33)]=0x55;
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CALLEE && ram[addr(0x1000,0x33)]==0);
    /* A in the EGA aperture is written through the bus, not flat RAM. */
    c=fixture(0,0x12,0xA000,0x0100);memset(ega.plane,0x5A,sizeof ega.plane);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CALLEE);
    CHECK(ega.plane[0][0x134]==0 && ega.plane[3][0x134]==0 && ega.plane[0][0x135]==0x5A);
    CHECK(ram[addr(0xA000,0x0134)]==0xA5);
    /* B in the EGA aperture is read through the bus. */
    c=fixture(0,0x12,0x1000,0x0200);put(c.ss,c.sp+10,0x0100);put(c.ss,c.sp+12,0xA000);
    memset(ega.plane,0x05,sizeof ega.plane);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CALLEE && d.callee==PR_U38_3134_DISPLAY);
    c=fixture(0,0x12,0x1000,0x0200);put(c.ss,c.sp+10,0x0100);put(c.ss,c.sp+12,0xA000);
    memset(ega.plane,0x30,sizeof ega.plane);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_OK && d.callee==-1);
    /* Code guards precede the pushes of their call frame. */
    for(bad=0;bad<4u;++bad) {
        c=fixture(0,0x12,0x1000,0x0200);
        if(bad==0)ram[addr(c.cs,0x316A)]=0x90;
        if(bad==1)put(c.cs,0x316B,0);
        if(bad==2)ram[addr(c.cs,0x316F)]=0x90;
        if(bad==3)put(c.cs,0x3170,0);
        CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_CODE);
        CHECK(c.ip==(bad<2u?0x316Au:0x316Fu) && c.cs==0x3000 && c.sp==(bad<2u?0x7FD8u:0x7FDCu));
        CHECK(bad<2u?(d.callee==-1 && d.status==-1):(d.callee==0 && d.status==0));
    }
    /* Host rejections are atomic. */
    for(bad=0;bad<7u;++bad) {
        c=fixture(0,0x12,0x1000,0x0200);
        if(bad==0)c.ip=0x3135;
        if(bad==1)c.sp=0x191;
        if(bad==2)c.sp=0xFFF3;
        if(bad==3)c.cs=0xA000;
        if(bad==4)c.ds=0xA800;
        if(bad==5)c.ss=0xAFFF;
        if(bad==6)c.ss=0xA000;
        before=c;memcpy(copy,ram,SIZE);
        CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,&d)==PR_U38_3134_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(copy,ram,SIZE)==0 && d.callee==-1 && d.status==-1);
    }
    c=fixture(0,0x12,0x1000,0x0200);
    CHECK(pr_u38_message_3134_cpu(NULL,SIZE,&c,&limits,&d)==PR_U38_3134_ARGUMENT);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE-1u,&c,&limits,&d)==PR_U38_3134_ARGUMENT);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,NULL,&limits,&d)==PR_U38_3134_ARGUMENT);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,NULL,&d)==PR_U38_3134_ARGUMENT);
    c=fixture(0,0x12,0x1000,0x0200);
    CHECK(pr_u38_message_3134_cpu(&bus,SIZE,&c,&limits,NULL)==PR_U38_3134_CALLEE);
    free(copy);printf("%u semantic checks passed\n",checks);return 0;
}
