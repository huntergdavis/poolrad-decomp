#include "poolrad/replay/u3b0_poll_0566.h"
#include "poolrad/replay/u3b0_return_06cf.h"
#include "poolrad/replay/u3b0_character_059c.h"
#include "poolrad/replay/crt_input_fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
enum { RAM=1<<20, BODY=0x1899, DRV=0x18ae, DOS=0x1a32, SYS=0x1af8, CRT=0x1a40, DS=0x1c7c, SS=0x7000, CS=0x6000, PSP=0x6800 };
static uint8_t m[RAM], expected_ram[RAM];
static size_t p(unsigned s,unsigned o) { return (((size_t)s<<4)+o)&(RAM-1u); }
static void w(unsigned s,unsigned o,unsigned v) { size_t a=p(s,o); m[a]=(uint8_t)v; m[(a+1u)&(RAM-1u)]=(uint8_t)(v>>8); }
static int vector(void *x,uint8_t *r,pr_r08ae_driver_cpu *c,uint16_t *f) { (void)x;(void)r;(void)c;(void)f;return 1; }
static int out(void *x,uint16_t port,uint8_t v) { (void)x;(void)port;(void)v;return 1; }
static int in(void *x,uint16_t port,uint8_t *v) { (void)x;(void)port;(void)v;return 1; }
static int timefn(void *x,const pr_dos_gettime_cpu *b,pr_dos_gettime_cpu *a) { (void)x;(void)b;(void)a;return 1; }
static int video(void *x,uint8_t *r,const pr_dos_intr_cpu *b,pr_dos_intr_cpu *a) { (void)x;(void)r;(void)b;(void)a;return 1; }
static void farop(unsigned pc,unsigned off,unsigned seg) { m[p(CS,pc)]=0x9a;w(CS,pc+1,off);w(CS,pc+3,seg); }
static pr_r0899_cpu setup(pr_crt_input_fifo *q) {
    static const uint16_t sites[8][2]={{0x63,CRT},{0x82,DRV},{0x97,SYS},{0xa4,DRV},{0xb3,SYS},{0xbd,SYS},{0xe7,CRT},{0xf0,CRT}};
    pr_r0899_cpu c={0x1234,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x9000,0xf000,CS,0x0566,DS,0x1111,SS,0x0202};
    size_t i; memset(m,0,sizeof m); pr_crt_input_fifo_init(q);
    farop(0x566,0x02fd,CRT);farop(0x572,0x0059,BODY);farop(0x580,0x0059,BODY);
    for(i=0;i<8;i++) w(BODY,sites[i][0],sites[i][1]);
    w(BODY,0x10,DRV);w(BODY,0x28,DOS);w(BODY,0x2f,SYS);m[p(BODY,0x33)]=3;
    memcpy(m+p(BODY,0x53),"\x05" "HELLO",6);
    w(DS,0x4422,PSP);m[p(PSP,0x80)]=0;m[p(DS,0x5372)]=1;w(DS,0x2612,1);
    w(SS,0x9006,0x8000);w(SS,0x9008,SS);m[p(SS,0x900c)]=1;
    return c;
}
int main(void) {
    pr_crt_input_fifo q; pr_r0899_cpu c,expected; pr_u3b0_poll_diagnostic d;
    pr_r0899_key_env ke={{{NULL,vector,out,in,timefn,1000u},video,NULL},pr_crt_input_fifo_service,&q,8u};
    pr_u3b0_poll_env e={CRT,BODY,&ke}; pr_u3b0_return_target t={0x2e60,0x09d5};
    c=setup(&q);w(SS,0x9000,0x7777);w(SS,0x9002,t.ip);w(SS,0x9004,t.cs);
    w(DS,0x49d2,0x1000);w(DS,0x49d4,0x5000);w(0x5000,0x13fe,0xabcd);
    CHECK(pr_crt_input_fifo_push(&q,0x011b)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_NON_EXTENDED && c.ip==0x059c);
    memcpy(expected_ram,m,sizeof m);expected_ram[p(SS,0x8f9d)]=1;expected_ram[p(SS,0x8fa0)]=0;
    expected=c;expected.ax=(uint16_t)((expected.ax&0xff00u)|0x1b);expected.ip=0x06cf;
    expected.flags=(uint16_t)((expected.flags&~0x08d5u)|0x44u);
    CHECK(pr_u3b0_character_059c(m,RAM,&c)==PR_U3B0_CHARACTER_ESCAPE);
    CHECK(memcmp(m,expected_ram,sizeof m)==0 && memcmp(&c,&expected,sizeof c)==0);
    CHECK(pr_u3b0_return_06cf(m,RAM,&c,&t)==PR_U3B0_RETURN_DONE && c.ip==t.ip && (c.ax&0xff)==0);
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x1c0d)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_NON_EXTENDED);
    m[p(SS,0x8f71)]=0;memcpy(expected_ram,m,sizeof m);
    CHECK(pr_u3b0_character_059c(m,RAM,&c)==PR_U3B0_CHARACTER_ENTER_DISABLED && c.ip==0x06cf && memcmp(m,expected_ram,sizeof m)==0);
    CHECK(pr_u3b0_return_06cf(m,RAM,&c,&t)==PR_U3B0_RETURN_LOOP && c.ip==0x048e);
    c=setup(&q);w(SS,0x9000,0x7777);w(SS,0x9002,t.ip);w(SS,0x9004,t.cs);
    w(DS,0x49d2,0x1000);w(DS,0x49d4,0x5000);m[p(SS,0x8f71)]=1;
    CHECK(pr_crt_input_fifo_push(&q,0x1c0d)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_NON_EXTENDED);
    CHECK(pr_u3b0_character_059c(m,RAM,&c)==PR_U3B0_CHARACTER_ENTER_ENABLED && m[p(SS,0x8f9d)]==1);
    CHECK(pr_u3b0_return_06cf(m,RAM,&c,&t)==PR_U3B0_RETURN_DONE && (c.ax&0xff)==0x0d);
    c=setup(&q);c.ip=0x059c;m[p(SS,0x8fa0)]=1;memcpy(expected_ram,m,sizeof m);
    CHECK(pr_u3b0_character_059c(m,RAM,&c)==PR_U3B0_CHARACTER_OTHER && c.ip==0x05c4 && (c.ax&0xff)==1 && (c.flags&0x08d5u)==0x91u && memcmp(m,expected_ram,sizeof m)==0);
    c=setup(&q);c.ip=0x059c;c.bp=0x8e;expected=c;memcpy(expected_ram,m,sizeof m);
    CHECK(pr_u3b0_character_059c(m,RAM,&c)==PR_U3B0_CHARACTER_ARGUMENT && memcmp(&c,&expected,sizeof c)==0 && memcmp(m,expected_ram,sizeof m)==0);
    puts("3B0 character branch composition passed");return 0;
}
