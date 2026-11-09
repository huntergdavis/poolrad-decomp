#include "poolrad/recovered/resident_0709_time_0c3f_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xffffu)) & 0xfffffu; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t p = at(s,o); m[p] = (uint8_t)v; m[(p+1u)&0xfffffu] = (uint8_t)(v>>8); }
static int service(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after)
{
    if (*(int *)ctx) return 1;
    *after = *before; after->ip = 0x00a9; after->cx = 0x0d2a; after->dx = 0x0737;
    return 0;
}
static pr_r08ae_time_cpu initial(void)
{
    pr_r08ae_time_cpu c = {0x1234,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,0x8000,0x8800,0,0x5000,0x1111,0x2000,2};
    return c;
}
int main(void)
{
    uint8_t *a = malloc(PR_BUS_RAM_SIZE), *b = malloc(PR_BUS_RAM_SIZE), *snapshot = malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega = calloc(1, sizeof *ega);
    pr_bus ba = {a,NULL,NULL,NULL,NULL}, bb = {b,ega,NULL,NULL,NULL};
    pr_r08ae_time_cpu ca, cb, saved;
    pr_r08ae_time_cpu_diagnostic da, db;
    int refuse = 0;
    CHECK(a && b && snapshot && ega);
    memset(a, 0x5a, PR_BUS_RAM_SIZE);
    put(a, 0x8800, 0x001d, 0x6000); put(a, 0x8800, 0x002d, 0x6800);
    put(a, 0x8800, 0x0c5c, 0x6000); put(a, 0x8800, 0x0c6c, 0x6800);
    put(a, 0x2000, 0x8000, 0x7000); put(a, 0x2000, 0x8002, 0x9001);
    memcpy(b,a,PR_BUS_RAM_SIZE);
    ca = initial(); cb = ca; cb.ip = 0x0c3f;
    CHECK(pr_r08ae_time_0000_cpu(&ba, PR_BUS_RAM_SIZE, &ca, service, &refuse, &da) == PR_R08AE_TIME_CPU_OK);
    CHECK(pr_r0709_time_0c3f_cpu(&bb, PR_BUS_RAM_SIZE, &cb, service, &refuse, &db) == PR_R08AE_TIME_CPU_OK);
    CHECK(memcmp(&ca,&cb,sizeof ca)==0 && memcmp(&da,&db,sizeof da)==0);
    CHECK(cb.ax == 0x4493 && cb.dx == 0x0048 && cb.cs == 0x9001 && cb.ip == 0x7000);
    /* Different genuine return IPs remain in the stack. Normalize only these words. */
    put(b, 0x2000, 0x7fde, 0x001f); put(b, 0x2000, 0x7fee, 0x002f);
    CHECK(memcmp(a,b,PR_BUS_RAM_SIZE)==0);
    /* Refusal stops inside relocated GetTime, with true 0709 caller return. */
    memset(b,0x5a,PR_BUS_RAM_SIZE); put(b,0x8800,0x0c5c,0x6123);
    cb=initial(); cb.ip=0x0c3f; refuse=1;
    CHECK(pr_r0709_time_0c3f_cpu(&bb,PR_BUS_RAM_SIZE,&cb,service,&refuse,&db)==PR_R08AE_TIME_CPU_CALLEE);
    CHECK(cb.cs==0x6123 && cb.ip==0x00a7 && db.gettime_status==PR_DOS_GETTIME_SERVICE && db.longmul_status==-1);
    CHECK(b[at(0x2000,0x7fde)]==0x5e && b[at(0x2000,0x7fdf)]==0x0c);
    /* Any potential leaf stack overlap rejects before CPU, RAM or EGA mutation. */
    cb=initial(); cb.ip=0x0c3f; cb.ss=0xa000; saved=cb; memcpy(snapshot,b,PR_BUS_RAM_SIZE);
    CHECK(pr_r0709_time_0c3f_cpu(&bb,PR_BUS_RAM_SIZE,&cb,service,&refuse,&db)==PR_R08AE_TIME_CPU_ARGUMENT);
    CHECK(memcmp(&cb,&saved,sizeof cb)==0 && memcmp(snapshot,b,PR_BUS_RAM_SIZE)==0);
    cb.ss=0x9800; cb.sp=0x8000; saved=cb; /* first caller word at A0000 */
    CHECK(pr_r0709_time_0c3f_cpu(&bb,PR_BUS_RAM_SIZE,&cb,service,&refuse,&db)==PR_R08AE_TIME_CPU_ARGUMENT);
    CHECK(memcmp(&cb,&saved,sizeof cb)==0);
    /* Relocated call operands can themselves be in EGA; reads load latches in byte order. */
    pr_ega_reset(ega); refuse = 0; cb = initial(); cb.cs=0xa000; cb.ip=0x0c3f;
    put(b,0x2000,0x8000,0x7000); put(b,0x2000,0x8002,0x9001);
    ega->plane[0][0x0c5c]=0x23; ega->plane[0][0x0c5d]=0x61;
    ega->plane[0][0x0c6c]=0x45; ega->plane[0][0x0c6d]=0x68;
    ega->plane[1][0x0c6c]=0x39; ega->plane[1][0x0c6d]=0x79;
    CHECK(pr_r0709_time_0c3f_cpu(&bb,PR_BUS_RAM_SIZE,&cb,service,&refuse,&db)==PR_R08AE_TIME_CPU_OK);
    CHECK(ega->latch[0]==0x68 && ega->latch[1]==0x79);
    /* Live first segment selected by EGA is visible at the service refusal stop. */
    cb=initial(); cb.cs=0xa000; cb.ip=0x0c3f; refuse=1;
    CHECK(pr_r0709_time_0c3f_cpu(&bb,PR_BUS_RAM_SIZE,&cb,service,&refuse,&db)==PR_R08AE_TIME_CPU_CALLEE);
    CHECK(cb.cs==0x6123 && cb.ip==0x00a7 && ega->latch[0]==0x61);
    free(ega); free(snapshot); free(b); free(a);
    puts("0709 clock differential and guard checks passed");
    return 0;
}
