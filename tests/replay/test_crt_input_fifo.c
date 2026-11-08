#include "poolrad/replay/crt_input_fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_CRT_KEYBOARD_RAM_SIZE
#define DS 0x5000u
#define SS 0x2000u
static uint8_t ram[RAM], saved[RAM];
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + o) & (RAM - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { ram[p(s,o)] = (uint8_t)v; ram[p(s,o+1u)] = (uint8_t)(v >> 8u); }
static pr_crt_cpu cpu(unsigned ip)
{
    pr_crt_cpu c = {0x1234,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,0x8000,0x6000,(uint16_t)ip,DS,0x1111,SS,0x0a83};
    return c;
}
static void frame(void) { put(SS,0x8000,0x7000); put(SS,0x8002,0x9001); }
int main(void)
{
    pr_crt_input_fifo q, copy;
    pr_crt_cpu b, a, expected, c;
    size_t i;
    memset(&q, 0, sizeof q); pr_crt_input_fifo_init(&q);
    memset(ram, 0x5a, sizeof ram); memcpy(saved, ram, sizeof ram);
    b = cpu(0x0306); b.ax = 0x0100; a = cpu(0x9999);
    expected = b; expected.ip += 2u; expected.flags |= 0x0040u;
    CHECK(pr_crt_input_fifo_service(&q, &b, &a) == 0 && memcmp(&a,&expected,sizeof a)==0);
    CHECK(memcmp(ram,saved,sizeof ram)==0);
    CHECK(pr_crt_input_fifo_push(&q, 0x4800) == PR_CRT_INPUT_OK);
    CHECK(pr_crt_input_fifo_push(&q, 0x1e61) == PR_CRT_INPUT_OK);
    b.flags = 0xffff; expected = b; expected.ip += 2u; expected.ax = 0x4800; expected.flags &= (uint16_t)~0x0040u;
    CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && memcmp(&a,&expected,sizeof a)==0 && q.count==2);
    CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && a.ax==0x4800 && q.count==2);
    b.ax = 0x0000; b.ip = 0x031d; expected = b; expected.ip += 2u; expected.ax = 0x4800;
    CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && memcmp(&a,&expected,sizeof a)==0 && q.count==1);
    CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && a.ax==0x1e61 && q.count==0);
    a = expected; copy = q;
    CHECK(pr_crt_input_fifo_service(&q,&b,&a)!=0 && memcmp(&q,&copy,sizeof q)==0 && memcmp(&a,&expected,sizeof a)==0);
    b.ax = 0x0200; CHECK(pr_crt_input_fifo_service(&q,&b,&a)!=0 && memcmp(&q,&copy,sizeof q)==0);
    CHECK(pr_crt_input_fifo_service(NULL,&b,&a)!=0 && pr_crt_input_fifo_push(NULL,1)==PR_CRT_INPUT_ARGUMENT);
    for (i=0;i<PR_CRT_INPUT_FIFO_CAPACITY;i++) CHECK(pr_crt_input_fifo_push(&q,(uint16_t)i)==PR_CRT_INPUT_OK);
    copy=q; CHECK(pr_crt_input_fifo_push(&q,99)==PR_CRT_INPUT_FULL && memcmp(&q,&copy,sizeof q)==0);
    b.ax=0; for(i=0;i<8;i++) CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && a.ax==i);
    for(i=0;i<8;i++) CHECK(pr_crt_input_fifo_push(&q,(uint16_t)(100+i))==PR_CRT_INPUT_OK);
    for(i=8;i<PR_CRT_INPUT_FIFO_CAPACITY;i++) CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && a.ax==i);
    for(i=0;i<8;i++) CHECK(pr_crt_input_fifo_service(&q,&b,&a)==0 && a.ax==100+i);
    CHECK(memcmp(ram,saved,sizeof ram)==0);
    /* Actual CRT: poll sees queued Up, then first ReadKey buffers scan; second returns it without another BIOS read. */
    memset(ram,0,sizeof ram); frame(); CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    c=cpu(0x02fd); CHECK(pr_crt_keypressed_02fd(ram,RAM,&c,pr_crt_input_fifo_service,&q)==PR_CRT_RETURNED && (c.ax & 0xff)==1 && q.count==1);
    c=cpu(0x030f); CHECK(pr_crt_readkey_030f(ram,RAM,&c,pr_crt_input_fifo_service,&q)==PR_CRT_RETURNED && (c.ax & 0xff)==0 && ram[p(DS,0x8501)]==0x48 && q.count==0);
    c=cpu(0x030f); CHECK(pr_crt_readkey_030f(ram,RAM,&c,pr_crt_input_fifo_service,&q)==PR_CRT_RETURNED && (c.ax & 0xff)==0x48 && ram[p(DS,0x8501)]==0);
    c=cpu(0x02fd); CHECK(pr_crt_keypressed_02fd(ram,RAM,&c,pr_crt_input_fifo_service,&q)==PR_CRT_RETURNED && (c.ax & 0xff)==0);
    c=cpu(0x030f); CHECK(pr_crt_readkey_030f(ram,RAM,&c,pr_crt_input_fifo_service,&q)==PR_CRT_SERVICE && c.ip==0x031d && q.count==0);
    puts("CRT input FIFO checks passed"); return 0;
}
