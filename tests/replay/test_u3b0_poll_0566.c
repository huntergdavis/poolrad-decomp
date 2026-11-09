#include "poolrad/replay/u3b0_poll_0566.h"
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
static int decline_read(void *q, const pr_crt_cpu *before, pr_crt_cpu *after) {
    if ((before->ax >> 8u) == 0u) return 1;
    return pr_crt_input_fifo_service(q, before, after);
}
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
    pr_crt_input_fifo q; pr_r0899_cpu c; pr_u3b0_poll_diagnostic d;
    pr_r0899_key_env ke={{{NULL,vector,out,in,timefn,1000u},video,NULL},pr_crt_input_fifo_service,&q,8u};
    pr_u3b0_poll_env e={CRT,BODY,&ke};
    c=setup(&q);m[p(SS,0x8f9d)]=7;
    memcpy(expected_ram,m,sizeof m);
    expected_ram[p(SS,0xeffe)]=(uint8_t)CS; expected_ram[p(SS,0xefff)]=(uint8_t)(CS>>8);
    expected_ram[p(SS,0xeffc)]=0x6b; expected_ram[p(SS,0xeffd)]=0x05;
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_NO_EVENT);
    CHECK(c.cs==CS && c.ip==0x06cf && c.sp==0xf000 && m[p(SS,0x8f9d)]==7 && q.count==0);
    CHECK(memcmp(m,expected_ram,sizeof m)==0);
    CHECK(c.ax==0x0100 && c.flags==0x0246 && c.bx==0x3456 && c.bp==0x9000 && c.ds==DS && c.es==0x1111);
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_EXTENDED);
    CHECK(c.cs==CS && c.ip==0x06cf && c.sp==0xf000 && q.count==0);
    CHECK(m[p(SS,0x8fa0)]==0x48 && m[p(SS,0x8f9d)]==1 && m[p(SS,0x8000)]==1 && c.es==SS && c.di==0x8000);
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);CHECK(pr_crt_input_fifo_push(&q,0x1e61)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_EXTENDED);
    CHECK(m[p(SS,0x8fa0)]==0x61 && q.count==0); /* second wrapper drains the later key */
    c=setup(&q);m[p(SS,0x900c)]=0;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_EXTENDED_DISABLED && m[p(SS,0x8000)]==0 && m[p(SS,0x8f9d)]==0);
    c=setup(&q);w(SS,0x9006,0x8fa0);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_EXTENDED && m[p(SS,0x8fa0)]==1 && m[p(SS,0x8f9d)]==1);
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x1e61)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_NON_EXTENDED && c.ip==0x059c && m[p(SS,0x8fa0)]==0x61);
    c=setup(&q);w(CS,0x567,0x1234);CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_CALL_TARGET && c.ip==0x0566);
    c=setup(&q);m[p(DS,0x5372)]=2;m[p(DS,0x5373)]=1;CHECK(pr_crt_input_fifo_push(&q,0x1f13)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_KEY_STOP && d.key_status==PR_R0899_KEY_DRIVER && q.count==0 && m[p(DS,0x5372)]==1);
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    ke.keyboard=decline_read;
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_KEY_STOP && d.key_status==PR_R0899_KEY_CRT && d.key.status==PR_CRT_SERVICE);
    CHECK(d.site==0x0572 && c.ip==0x031d && q.count==1 && m[p(SS,0xeffc)]==0x77);
    ke.keyboard=pr_crt_input_fifo_service;
    c=setup(&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);w(CS,0x581,0x1234);
    CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_CALL_TARGET && c.ip==0x0580 && q.count==0 && m[p(DS,0x8501)]==0x48);
    c=setup(&q);c.sp=0x0115;CHECK(pr_u3b0_poll_0566(m,RAM,&c,&e,&d)==PR_U3B0_POLL_ARGUMENT);
    puts("3B0 bounded input checks passed");return 0;
}
