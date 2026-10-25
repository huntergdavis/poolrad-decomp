#include "poolrad/recovered/resident_0709_printer_0768_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8); }
static unsigned word(const uint8_t *m,unsigned s,unsigned o) { size_t a=p(s,o);return m[a]|((unsigned)m[(a+1u)&1048575u]<<8); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip)
{ (void)ctx;(void)port;(void)value;(void)size;(void)cs;(void)ip;++outs;return 0; }
static void written(void *ctx,uint16_t offset,uint8_t value) { (void)ctx;(void)offset;(void)value;++writes; }
static void far_site(uint8_t *m,unsigned cs,unsigned at,unsigned target,unsigned seg)
{ m[p(cs,at)]=0x9A;put(m,cs,at+1u,target);put(m,cs,at+3u,seg); }
/* The window (x1, y1, x2, y2), the text at 4000:0100, the cursor; callee code sites as the 06A7 and 0DD3 tests plant them. */
static pr_r0709_printer_0768_regs setup(pr_bus *b,unsigned x1,unsigned y1,unsigned x2,unsigned y2,const char *text,unsigned clear)
{
    pr_r0709_printer_0768_regs c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x768,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,0x0100,0x4000,0xA500|clear,0xA505,0xA500|y2,0xA500|x2,0xA500|y1,0xA500|x1};
    const unsigned sites[]={0xE41,0xE63,0xF18,0xF35,0xF52,0xF6F,0xE8E,0xEA8};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<10u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(0x4000,0x100)]=(uint8_t)strlen(text);memcpy(b->ram+p(0x4000,0x101),text,strlen(text));
    b->ram[p(c.ds,0x0D16)]=0x02;b->ram[p(c.ds,0x0D17)]=0x70;b->ram[p(c.ds,0x0D18)]=0x00;b->ram[p(c.ds,0x0D19)]=0x8C;
    b->ram[p(c.ds,0x5E24)]=1;b->ram[p(c.ds,0x5E25)]=1;
    /* 06A7 and the glyph (as test_resident_0709_text_06a7_cpu.c) */
    b->ram[p(c.ds,0x4A1E)]=1;put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i)put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));
    for(i=0;i<2048u;++i)b->ram[p(0x7000,0x1000+i)]=(uint8_t)(0x81u^(i*13u));
    far_site(b->ram,c.cs,0x207,0x182A,0x6000);
    b->ram[p(c.cs,0x6E8)]=0xE8;put(b->ram,c.cs,0x6E9,0xFAF4);
    far_site(b->ram,c.cs,0x6FD,0x29C,0x7800);
    b->ram[p(c.ds,0x4A00)]=0;b->ram[p(c.ds,0x49E2)]=1;put(b->ram,c.ds,0x84FC,2);b->ram[0x400]=0x42;
    /* 0DD3 (as test_resident_0709_clear_0dd3_cpu.c) */
    b->ram[p(c.ds,0x4A1A)]=0;far_site(b->ram,c.cs,0x140,0x16A8,0x6000);
    for(i=0;i<8u;++i) {
        if(i<6)far_site(b->ram,c.cs,sites[i],0x16A8,0x6000);
        else {b->ram[p(c.cs,sites[i])]=0xE8;put(b->ram,c.cs,sites[i]+1u,(0xB7u-sites[i]-3u)&65535u);}
    }
    /* this body's own far sites */
    far_site(b->ram,c.cs,0x77E,0x0B25,0x6000);
    for(i=0;i<3u;++i) {
        const unsigned at[]={0x848,0x880,0x8E0};
        far_site(b->ram,c.cs,at[i],0x0D12,0x6000);far_site(b->ram,c.cs,at[i]+5u,0x0DC0,0x6000);
    }
    far_site(b->ram,c.cs,0x990,0x0B0B,0x6000);far_site(b->ram,c.cs,0x999,0x0104,0x6100);
    /* the literal at CS:074E (25 bytes in the game: it fits in [BP-11Fh,BP-105h], below the locals at [BP-104h]) */
    memcpy(b->ram+p(c.cs,0x74E),"\x19Press any key to continue",26);
    outs=writes=0;return c;
}
static const pr_r0709_printer_budget wide={65536,65536,65536,65536,100000000u,65536,65536,65536};
static int run(pr_bus *b,pr_r0709_printer_0768_regs *c,size_t words,size_t scans,pr_r0709_printer_diagnostic *d)
{
    pr_r0709_printer_budget k=wide;k.word_loopbacks=words;k.scan_loopbacks=scans;
    return pr_r0709_printer_0768_cpu(b,PR_BUS_RAM_SIZE,c,&k,d);
}
static unsigned cursor(const uint8_t *m) { return m[p(0x5000,0x5E24)]|(unsigned)m[p(0x5000,0x5E25)]<<8; }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE),*cycle=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_printer_0768_regs c,before,first;pr_r0709_printer_diagnostic d;
    CHECK(ram && copy && cycle && ega);
    /* Two words that fit: HELLO+space at columns 1..6, WORLD at 7..11; one glyph is 34 OUTs and 64 VRAM writes (06A7 test). */
    c=setup(&bus,1,1,20,5,"HELLO WORLD",0);
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_RETURNED);
    CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8014 && c.bp==0x7777 && d.site==0 && d.callee_status==-1);
    CHECK(cursor(ram)==(1u<<8|12u) && outs==11u*34u && writes==11u*64u);
    /* A cursor outside the window is homed first. */
    c=setup(&bus,3,2,20,5,"HI",0);ram[p(c.ds,0x5E24)]=30;
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_RETURNED && cursor(ram)==(2u<<8|5u) && outs==2u*34u);
    /* x1 > 27h: straight to the epilogue, nothing drawn, the cursor untouched. */
    c=setup(&bus,0x28,1,0x28,5,"HI",0);ram[p(c.ds,0x5E24)]=33;
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_RETURNED && c.sp==0x8014 && cursor(ram)==(1u<<8|33u) && outs==0);
    /* The word budget: the first JMP 081A (09C8) refused, after HELLO+space. */
    c=setup(&bus,1,1,20,5,"HELLO WORLD",0);
    CHECK(run(&bus,&c,0,65536,&d)==PR_R0709_PRINTER_LIMIT);
    CHECK(c.ip==0x09C8 && c.cs==0x8800 && c.bp==0x7FFE && c.sp==0x7ED8 && d.site==0x09C8 && d.callee_status==-1);
    CHECK(cursor(ram)==(1u<<8|7u) && ram[p(c.ss,0x7FFE - 0x103)]==7 && outs==6u*34u);
    /* The scan budget: the second JMP 0822 (0858) refused while skipping "!!!". */
    c=setup(&bus,1,1,20,5,"!!!AB",0);
    CHECK(run(&bus,&c,65536,1,&d)==PR_R0709_PRINTER_LIMIT);
    CHECK(c.ip==0x0858 && d.site==0x0858 && ram[p(c.ss,0x7FFE - 0x104)]==3 && outs==0);
    /* x1 > x2 passes the checks; the one-byte word never fits and the row advances once per pass, forever: 601 passes
     * leave row 1 + 601 = 90 (mod 256), and 256 more passes reproduce the whole state. */
    c=setup(&bus,10,1,5,5,"A",0);
    CHECK(run(&bus,&c,600,65536,&d)==PR_R0709_PRINTER_LIMIT && c.ip==0x09C8 && cursor(ram)==(90u<<8|10u) && outs==0 && writes==0);
    c=setup(&bus,10,1,5,5,"A",0);CHECK(run(&bus,&c,3,65536,&d)==PR_R0709_PRINTER_LIMIT);
    first=c;memcpy(cycle,ram,PR_BUS_RAM_SIZE);
    c=setup(&bus,10,1,5,5,"A",0);CHECK(run(&bus,&c,3u+256u,65536,&d)==PR_R0709_PRINTER_LIMIT);
    CHECK(memcmp(&c,&first,sizeof c)==0 && memcmp(ram,cycle,PR_BUS_RAM_SIZE)==0);
    /* The window fills: AAAA+space on row 1, BBBB wraps past y2 -> home, the literal, STOP_BF5 with PUSH CS; CALL done. */
    c=setup(&bus,1,1,6,1,"AAAA BBBB",0);
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_STOP_BF5);
    CHECK(c.ip==0x0BF5 && c.cs==0x8800 && d.site==0x0996 && word(ram,c.ss,c.sp)==0x0999 && word(ram,c.ss,c.sp+2u)==0x8800);
    CHECK(word(ram,c.ss,c.sp+4u)==0x7FFE - 0x11F && word(ram,c.ss,c.sp+6u)==0x2000 && ram[p(c.ss,c.sp+8u)]==0x0D);
    CHECK(cursor(ram)==(1u<<8|1u) && ram[p(c.ss,0x7FFE - 0x11F)]==ram[p(c.cs,0x74E)] && outs==5u*34u);
    /* 0BF5 returns (RETF 6) and the model resumes at 0999: idle, the clear, BBBB at columns 1..4. */
    c.ip=0x0999;c.sp=(uint16_t)(c.sp+10u);outs=writes=0;
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_RETURNED && c.sp==0x8014 && c.ip==0x7000);
    CHECK(cursor(ram)==(1u<<8|5u) && outs>4u*34u);
    /* The clear flag: 0DD3 runs, the cursor is homed, then HI is drawn. */
    c=setup(&bus,2,3,9,4,"HI",1);ram[p(c.ds,0x5E24)]=5;ram[p(c.ds,0x5E25)]=4;
    CHECK(run(&bus,&c,65536,65536,&d)==PR_R0709_PRINTER_RETURNED && cursor(ram)==(3u<<8|4u) && outs>2u*34u);
    /* Rejections change nothing. */
    c=setup(&bus,1,1,20,5,"HI",0);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);
    c.ip=0x0770;before.ip=0x0770;CHECK(run(&bus,&c,1,1,&d)==PR_R0709_PRINTER_ARGUMENT);
    c.ip=0x0768;before.ip=0x0768;c.sp=before.sp=0x02FE;CHECK(run(&bus,&c,1,1,&d)==PR_R0709_PRINTER_ARGUMENT);
    c.sp=before.sp=0x8000;CHECK(pr_r0709_printer_0768_cpu(&bus,PR_BUS_RAM_SIZE,&c,NULL,&d)==PR_R0709_PRINTER_ARGUMENT);
    CHECK(pr_r0709_printer_0768_cpu(&bus,PR_BUS_RAM_SIZE-1u,&c,&wide,&d)==PR_R0709_PRINTER_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(ram,copy,PR_BUS_RAM_SIZE)==0 && outs==0);
    printf("resident 0709:0768 printer: %u checks\n",checks);
    free(ram);free(copy);free(cycle);free(ega);
    return 0;
}
