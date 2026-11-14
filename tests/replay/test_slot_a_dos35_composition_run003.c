/* Conditional run003 DOS35 binding. PSP ownership is supplied, not proven. */
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_13b1_cpu.h"
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include "poolrad/recovered/resident_08ae_table_1742_cpu.h"
#include "poolrad/recovered/resident_08ae_table_17ae_cpu.h"
#include "poolrad/recovered/resident_08ae_delay_006c_cpu.h"
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 655360u
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static void word(uint8_t *r, unsigned p, uint16_t v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
static uint16_t get(const uint8_t *r, unsigned p) { return (uint16_t)(r[p] | ((unsigned)r[p+1]<<8)); }
static int load(const char *p, uint8_t *r) { FILE *f=fopen(p,"rb"); size_t n; int c; if(!f)return 0; n=fread(r,1,SIZE,f); c=fgetc(f); fclose(f); return n==SIZE && c==EOF; }
static int service(void *ctx, uint8_t *r, pr_r08ae_137c_cpu *c, uint16_t *flags) {
    unsigned stack, i; uint16_t reg[9];
    if (!ctx || *(const uint16_t *)ctx!=0x0814 || c->ax!=0x3508 || c->bx!=0x3ed4 || c->cx!=4 || c->dx!=15 || c->si!=0x3e8a || c->di!=0x4f || c->bp!=0x3db6 || c->ds!=0x10d2 || c->es!=0x3da1 || c->cs!=0x10d2 || c->ip!=0x1386 || c->ss!=0x1d16 || c->sp!=0x3da8 || c->flags!=0x7002 || r[0x8140]!=0xcd || r[0x8141]!=0x20) return 1;
    stack=((unsigned)c->ss<<4)+c->sp;
    if (stack<18 || stack+6>SIZE || get(r,stack)!=0x1388 || get(r,stack+2)!=c->cs || get(r,stack+4)!=0x7202) return 1;
    reg[0]=c->ax;reg[1]=c->bx;reg[2]=c->cx;reg[3]=c->dx;reg[4]=c->si;reg[5]=c->di;reg[6]=c->bp;reg[7]=c->ds;reg[8]=c->es;
    for(i=0;i<9;i++) word(r,stack-18+2*i,reg[i]);
    word(r,0x8140+0x2e,(uint16_t)(c->sp-18)); word(r,0x8140+0x30,c->ss);
    c->bx=get(r,0x20); c->es=get(r,0x22); *flags=get(r,stack+4); return 0;
}
/* Conditional AH=25 binding for the same capture and supplied PSP. */
static int service25(void *ctx, uint8_t *r, pr_r08ae_137c_cpu *c, uint16_t *flags) {
    unsigned stack, i; uint16_t reg[9];
    if (!ctx || *(const uint16_t *)ctx!=0x0814 || c->ax!=0x2508 || c->bx!=0xfea5 || c->cx!=4 || c->dx!=0x13ea || c->si!=0x3e8a || c->di!=0x4f || c->bp!=0x3db6 || c->ds!=0x10d2 || c->es!=0xf000 || c->cs!=0x10d2 || c->ip!=0x139e || c->ss!=0x1d16 || c->sp!=0x3da6 || c->flags!=0x7002 || r[0x8140]!=0xcd || r[0x8141]!=0x20) return 1;
    stack=((unsigned)c->ss<<4)+c->sp;
    if (stack<18 || stack+6>SIZE || get(r,stack)!=0x13a0 || get(r,stack+2)!=c->cs || get(r,stack+4)!=0x7202) return 1;
    reg[0]=c->ax;reg[1]=c->bx;reg[2]=c->cx;reg[3]=c->dx;reg[4]=c->si;reg[5]=c->di;reg[6]=c->bp;reg[7]=c->ds;reg[8]=c->es;
    for(i=0;i<9;i++) word(r,stack-18+2*i,reg[i]);
    word(r,0x8140+0x2e,(uint16_t)(c->sp-18)); word(r,0x8140+0x30,c->ss);
    word(r,0x20,c->dx); word(r,0x22,c->ds); *flags=get(r,stack+4); return 0;
}
typedef struct { unsigned count; uint16_t port; uint8_t value; } port_log;
static int observe_port(void *ctx, uint16_t port, uint8_t value) {
    port_log *log=(port_log *)ctx;
    if (!log || !((log->count==0 && port==0x43 && value==0xb6) || (log->count==1 && port==0x40 && value==0xb1) || (log->count==2 && port==0x40 && value==0x13))) return 1;
    log->count++; log->port=port; log->value=value; return 0;
}
static int refuse_gettime(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after) {
    (void)before; (void)after; ++*(unsigned *)ctx; return 1;
}
typedef struct { const uint16_t *values; unsigned count, next; } time_script;
/* Declared DOS service: only CX:DX is supplied; DOS makes no RAM writes. */
static int scripted_dos_time(void *ctx, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after) {
    time_script *script=(time_script *)ctx;
    if (!script || !before || !after || script->next>=script->count || before->ip!=0x00a7 || (before->ax & 0xff00u)!=0x2c00u) return 1;
    *after=*before; after->ip=0x00a9; after->cx=0; after->dx=script->values[script->next++]; return 0;
}
/* Commit only a complete time-model return; refusal leaves CPU/RAM intact. */
static int scripted_time(void *ctx, uint8_t *ram, pr_r08ae_006c_cpu *cpu) {
    time_script *script=(time_script *)ctx; pr_r08ae_time_cpu t; pr_r08ae_time_cpu_diagnostic diag;
    _Static_assert(sizeof(pr_r08ae_time_cpu)==sizeof(pr_r08ae_006c_cpu), "time CPU layout");
    uint8_t *copy; pr_bus bus; unsigned original_next; uint16_t sp,cs,ret;
    if (!script || script->next>=script->count || !ram || !cpu || cpu->ip!=0) return 1;
    copy=malloc(PR_BUS_RAM_SIZE); if (!copy) return 1; memcpy(copy,ram,PR_BUS_RAM_SIZE);
    bus=(pr_bus){copy,NULL,NULL,NULL,NULL}; sp=cpu->sp; cs=cpu->cs; ret=get(copy,((unsigned)cpu->ss<<4)+sp);
    original_next=script->next; memcpy(&t,cpu,sizeof t);
    if (pr_r08ae_time_0000_cpu(&bus,PR_BUS_RAM_SIZE,&t,scripted_dos_time,script,&diag)!=PR_R08AE_TIME_CPU_OK || t.sp!=(uint16_t)(sp+4) || t.cs!=cs || t.ip!=ret) { script->next=original_next; free(copy); return 1; }
    memcpy(ram,copy,PR_BUS_RAM_SIZE); free(copy); t.sp=sp; t.ip=0; memcpy(cpu,&t,sizeof t); return 0;
}
typedef struct { uint16_t flags; unsigned calls, outs, ins; uint8_t input61; } restore_binding;
/* Declared vector-table-only service; no DOS stack/PSP side effects. */
static int abstract_restore(void *ctx, uint8_t *ram, pr_r08ae_13b1_cpu *cpu, uint16_t *flags) {
    restore_binding *binding=(restore_binding *)ctx;
    if (!binding || !ram || !cpu || !flags || binding->calls || cpu->ax!=0x2508 || cpu->dx!=0xfea5 || cpu->ds!=0xf000 || cpu->ip!=0x13c6 || cpu->sp!=0x3da8 || cpu->flags!=0x7046 || get(ram,0x20)!=0x13ea || get(ram,0x22)!=0x10d2) return 1;
    word(ram,0x20,cpu->dx); word(ram,0x22,cpu->ds); *flags=binding->flags; binding->calls++; return 0;
}
static int restore_out(void *ctx, uint16_t port, uint8_t value) {
    static const uint16_t ports[]={0x40,0x40,0x61,0xc0,0xc0,0xc0,0xc0};
    static const uint8_t values[]={0xff,0xff,0x00,0x9f,0xbf,0xdf,0xff};
    restore_binding *binding=(restore_binding *)ctx;
    if (!binding || binding->calls!=1 || binding->outs>=7 || port!=ports[binding->outs] || value!=values[binding->outs] || (binding->outs>=2 && binding->ins!=1)) return 1;
    binding->outs++; return 0;
}
static int restore_in(void *ctx, uint16_t port, uint8_t *value) {
    restore_binding *binding=(restore_binding *)ctx;
    if (!binding || !value || port!=0x61 || binding->outs!=2 || binding->ins) return 1;
    *value=binding->input61; binding->ins++; return 0;
}
/* Acknowledge only the already-executed conditional dispatcher return. */
static int resident_returned(void *ctx, uint16_t argument) {
    const pr_r08ae_00dc_cpu *cpu=(const pr_r08ae_00dc_cpu *)ctx;
    return !cpu || argument!=10 || cpu->cs!=0x2e60 || cpu->ip!=0x0747 || cpu->sp!=0x3dbe;
}
int main(int argc,char **argv) {
    uint8_t *r,*expected; unsigned i,diffs=0; uint16_t psp=0x0815;
    pr_r08ae_137c_cpu initial={0x3508,0x3ed4,4,15,0x3e8a,0x4f,0x3db6,0x3dae,0x10d2,0x1386,0x10d2,0x3da1,0x1d16,0x7202}, c;
    const pr_r08ae_137c_cpu post={0x3508,0xfea5,4,15,0x3e8a,0x4f,0x3db6,0x3dae,0x10d2,0x1388,0x10d2,0xf000,0x1d16,0x7202};
    pr_r08ae_137c_env env={&psp,service,NULL};
    CHECK(argc==6); r=calloc(1,PR_R08AE_137C_RAM_SIZE); expected=malloc(SIZE); CHECK(r&&expected&&load(argv[1],r)&&load(argv[2],expected));
    c=initial; CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,&env)==PR_R08AE_137C_ENV_DOS); CHECK(c.ip==0x1386 && c.sp==0x3da8 && c.flags==0x7002);
    CHECK(load(argv[1],r)); c=initial; CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,NULL)==PR_R08AE_137C_ENV_DOS); CHECK(c.ip==0x1386 && c.sp==0x3da8);
    CHECK(load(argv[1],r)); c=initial; psp=0x0814;
    CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,&env)==PR_R08AE_137C_STOP_1388);
    for(i=0;i<SIZE;i++) if(r[i]!=expected[i]) { if(diffs<8) fprintf(stderr,"diff %05x\n",i); diffs++; }
    CHECK(diffs==0); CHECK(c.ax==post.ax && c.bx==post.bx && c.cx==post.cx && c.dx==post.dx && c.si==post.si && c.di==post.di && c.bp==post.bp && c.sp==post.sp && c.cs==post.cs && c.ip==post.ip && c.ds==post.ds && c.es==post.es && c.ss==post.ss && c.flags==post.flags);
    CHECK(load(argv[3],expected));
    CHECK(pr_r08ae_vector_137c_from_1388_to_139e(r,PR_R08AE_137C_RAM_SIZE,&c)==PR_R08AE_137C_STOP_139E);
    diffs=0; for(i=0;i<SIZE;i++) if(r[i]!=expected[i]) { if(diffs<8) fprintf(stderr,"139e diff %05x\n",i); diffs++; }
    CHECK(diffs==0 && c.ax==0x2508 && c.bx==0xfea5 && c.cx==4 && c.dx==0x13ea && c.si==0x3e8a && c.di==0x4f && c.bp==0x3db6 && c.sp==0x3dac && c.cs==0x10d2 && c.ip==0x139e && c.ds==0x10d2 && c.es==0xf000 && c.ss==0x1d16 && c.flags==0x7202);
    /* Preserve the composed state while checking explicit refusal. */
    { uint8_t *copy=malloc(PR_R08AE_137C_RAM_SIZE); pr_r08ae_137c_cpu refused=c;
      CHECK(copy); memcpy(copy,r,PR_R08AE_137C_RAM_SIZE);
      CHECK(pr_r08ae_vector_137c_from_139e(copy,PR_R08AE_137C_RAM_SIZE,&refused,NULL)==PR_R08AE_137C_ENV_DOS);
      CHECK(refused.ip==0x139e && refused.sp==0x3da6);
      memcpy(copy,r,PR_R08AE_137C_RAM_SIZE); refused=c; psp=0x0815; env.dos=service25;
      CHECK(pr_r08ae_vector_137c_from_139e(copy,PR_R08AE_137C_RAM_SIZE,&refused,&env)==PR_R08AE_137C_ENV_DOS);
      CHECK(refused.ip==0x139e && refused.sp==0x3da6); psp=0x0814; free(copy); }
    env.dos=service25;
    CHECK(pr_r08ae_vector_137c_from_139e(r,PR_R08AE_137C_RAM_SIZE,&c,&env)==PR_R08AE_137C_STOP_13A0);
    CHECK(load(argv[4],expected)); diffs=0;
    for(i=0;i<SIZE;i++) if(r[i]!=expected[i]) { if(diffs<8) fprintf(stderr,"13a0 diff %05x\n",i); diffs++; }
    CHECK(diffs==0 && c.ax==0x2508 && c.bx==0xfea5 && c.cx==4 && c.dx==0x13ea && c.si==0x3e8a && c.di==0x4f && c.bp==0x3db6 && c.sp==0x3dac && c.cs==0x10d2 && c.ip==0x13a0 && c.ds==0x10d2 && c.es==0xf000 && c.ss==0x1d16 && c.flags==0x7202);
    CHECK(pr_r08ae_vector_137c_from_13a0_to_13a3(r,PR_R08AE_137C_RAM_SIZE,&c)==PR_R08AE_137C_STOP_13A3);
    CHECK(load(argv[5],expected)); diffs=0;
    for(i=0;i<SIZE;i++) if(r[i]!=expected[i]) { if(diffs<8) fprintf(stderr,"13a3 diff %05x\n",i); diffs++; }
    CHECK(diffs==0 && c.ax==0x25b6 && c.bx==0xfea5 && c.cx==4 && c.dx==0x13ea && c.si==0x3e8a && c.di==0x4f && c.bp==0x3db6 && c.sp==0x3dae && c.cs==0x10d2 && c.ip==0x13a3 && c.ds==0x10d2 && c.es==0xf000 && c.ss==0x1d16 && c.flags==0x7202);
    /* Abstract observation: expected 13A3 RAM is only used for comparison. */
    { pr_r08ae_137c_cpu before=c, refused=c; port_log log={0,0,0};
      pr_r08ae_137c_env port_env={&log,NULL,observe_port};
      CHECK(pr_r08ae_vector_137c_first_port(r,PR_R08AE_137C_RAM_SIZE,&refused,NULL)==PR_R08AE_137C_ENV_PORT);
      CHECK(refused.ip==0x13a3 && !memcmp(&refused,&before,sizeof before) && !memcmp(r,expected,SIZE));
      CHECK(pr_r08ae_vector_137c_first_port(r,PR_R08AE_137C_RAM_SIZE,&c,&port_env)==PR_R08AE_137C_STOP_13A5);
      CHECK(log.count==1 && log.port==0x43 && log.value==0xb6);
      before.ip=0x13a5; CHECK(!memcmp(&c,&before,sizeof before) && !memcmp(r,expected,SIZE));
      puts("abstract first OUT: one 43h<-B6h; CPU advances to 13A5, RAM unchanged; no PIT-state claim");
      refused=c; CHECK(pr_r08ae_vector_137c_second_port(r,PR_R08AE_137C_RAM_SIZE,&refused,NULL)==PR_R08AE_137C_ENV_PORT);
      before.ax=0x13b1; before.ip=0x13a8; CHECK(!memcmp(&refused,&before,sizeof before) && !memcmp(r,expected,SIZE));
      CHECK(pr_r08ae_vector_137c_second_port(r,PR_R08AE_137C_RAM_SIZE,&c,&port_env)==PR_R08AE_137C_STOP_13AA);
      CHECK(log.count==2 && log.port==0x40 && log.value==0xb1);
      before.ip=0x13aa; CHECK(!memcmp(&c,&before,sizeof before) && !memcmp(r,expected,SIZE));
      puts("abstract second OUT: ordered 40h<-B1h after 43h<-B6h; CPU at 13AA, RAM unchanged; no PIT-state claim");
      refused=c; CHECK(pr_r08ae_vector_137c_third_port(r,PR_R08AE_137C_RAM_SIZE,&refused,NULL)==PR_R08AE_137C_ENV_PORT);
      before.ax=0x1313; before.ip=0x13ac; CHECK(!memcmp(&refused,&before,sizeof before) && !memcmp(r,expected,SIZE));
      CHECK(pr_r08ae_vector_137c_third_port(r,PR_R08AE_137C_RAM_SIZE,&c,&port_env)==PR_R08AE_137C_STOP_13AE);
      CHECK(log.count==3 && log.port==0x40 && log.value==0x13);
      before.ip=0x13ae; CHECK(!memcmp(&c,&before,sizeof before) && !memcmp(r,expected,SIZE));
      puts("abstract third OUT: ordered 40h<-13h after 43h<-B6h,40h<-B1h; CPU at 13AE; no PIT-state claim");
      CHECK(pr_r08ae_vector_137c_return(r,PR_R08AE_137C_RAM_SIZE,&c)==PR_R08AE_137C_OK);
      before.es=0x3da1; before.ds=0x14a0; before.sp=0x3db6; before.ip=0x00f4; before.cs=0x10d2;
      CHECK(!memcmp(&c,&before,sizeof before) && !memcmp(r,expected,SIZE));
      puts("abstract resident return: restored ES/DS and RETF to 10D2:00F4; RAM unchanged under logger");
      { pr_r08ae_00dc_cpu d; unsigned stack=((unsigned)c.ss<<4);
        CHECK(sizeof d==sizeof c); memcpy(&d,&c,sizeof d);
        CHECK(pr_r08ae_dispatch_00dc_cpu(r,PR_R08AE_137C_RAM_SIZE,&d)==PR_R08AE_00DC_CALL_1742);
        CHECK(d.ax==9 && d.bx==0xfea5 && d.cx==4 && d.dx==0x13ea && d.si==0x3e8a && d.di==0x4f && d.bp==0x3db6 && d.sp==0x3db0 && d.cs==0x10d2 && d.ip==0x1742 && d.ds==0x14a0 && d.es==0x3da1 && d.ss==0x1d16 && d.flags==0x7206);
        word(expected,stack+0x3db0,0x0147); word(expected,stack+0x3db2,0x10d2); word(expected,stack+0x3db4,9);
        CHECK(!memcmp(r,expected,SIZE));
        puts("abstract dispatcher continuation: call 10D2:1742 with argument 9; stack and CPU match modeled effects");
        { pr_r08ae_1742_cpu t; unsigned j;
          static const struct { unsigned address; uint8_t old_value, new_value; } changes[] = {
            {0x10f1f,0x00,0x09},{0x10f29,0x00,0x09},{0x10f2a,0x00,0x09},{0x10f2b,0x00,0x09},{0x10f2c,0x00,0x09},
            {0x10f4f,0x2b,0xdb},{0x10f50,0x0d,0x0b},{0x20f04,0x00,0xa0},{0x20f05,0xf0,0x14},{0x20f06,0xa0,0x8a},
            {0x20f07,0x13,0x3e},{0x20f08,0xd2,0x4f},{0x20f09,0x10,0x00},{0x20f0a,0x02,0xa5},{0x20f0b,0x72,0xfe},
            {0x20f0c,0xd2,0x09},{0x20f0d,0x10,0x00},{0x20f0e,0xa1,0xb6} };
          CHECK(sizeof t==sizeof d); memcpy(&t,&d,sizeof t);
          CHECK(pr_r08ae_table_1742_cpu(r,PR_R08AE_1742_RAM_SIZE,&t)==PR_R08AE_1742_OK);
          CHECK(t.ax==9 && t.bx==0xfea5 && t.cx==4 && t.dx==0x13ea && t.si==0x3e8a && t.di==0x4f && t.bp==0x3db6 && t.sp==0x3db6 && t.cs==0x10d2 && t.ip==0x0147 && t.ds==0x14a0 && t.es==0x3da1 && t.ss==0x1d16 && t.flags==0x7246);
          for(j=0;j<sizeof changes/sizeof changes[0];j++) { CHECK(expected[changes[j].address]==changes[j].old_value); expected[changes[j].address]=changes[j].new_value; }
          CHECK(!memcmp(r,expected,SIZE));
          puts("conditional 1742 model: returned 10D2:0147; CPU14 and 18 model-pinned RAM byte changes match; no native post-port parity");
          memcpy(&d,&t,sizeof d);
          CHECK(pr_r08ae_dispatch_00dc_cpu(r,PR_R08AE_00DC_RAM_SIZE,&d)==PR_R08AE_00DC_CALL_006C);
          CHECK(d.ax==10 && d.bx==0xfea5 && d.cx==4 && d.dx==0x13ea && d.si==0x3e8a && d.di==0x4f && d.bp==0x3db6 && d.sp==0x3db0 && d.cs==0x10d2 && d.ip==0x006c && d.ds==0x14a0 && d.es==0x3da1 && d.ss==0x1d16 && d.flags==0x7202);
          word(expected,stack+0x3db0,0x0165); word(expected,stack+0x3db2,0x10d2); word(expected,stack+0x3db4,10);
          CHECK(!memcmp(r,expected,SIZE));
          puts("conditional dispatcher continuation: call 10D2:006C with argument 10; CPU14 and stack effects match modeled expectations");
          { static const uint16_t values[]={0x0000,0x000a}; time_script script={values,2,0};
            uint8_t *copy=malloc(PR_BUS_RAM_SIZE); pr_r08ae_006c_cpu scripted; unsigned j;
            static const struct { unsigned address; uint8_t old_value,new_value; } changed[]={
              {0x20ede,0x31,0xa0},{0x20edf,0x35,0x3d},{0x20ee0,0x2c,0x1f},{0x20ee1,0x34,0},{0x20ee2,0x20,0xd2},{0x20ee3,0x57,0x10},
              {0x20ee4,0x20,0x94},{0x20ee5,0x31,0x3d},{0x20ee6,0x30,0x16},{0x20ee7,0x3a,0x1d},{0x20ee8,0x35,0x96},{0x20ee9,0x30,0x3d},
              {0x20eea,1,0x16},{0x20eeb,0,0x1d},{0x20eec,0x53,0},{0x20eee,1,0},{0x20eef,0xa0,0},{0x20ef0,0x0b,0x2f},{0x20ef1,0xa0,0},
              {0x20ef2,9,0xd2},{0x20ef3,0xa0,0x10},{0x20ef4,8,0x0a},{0x20ef5,0x25,0},{0x20ef6,0xa5,0},{0x20ef7,0xfe,0},{0x20ef8,4,0},
              {0x20efa,0xea,0},{0x20efb,0x13,0},{0x20efc,0x8a,0x0a},{0x20efd,0x3e,0},{0x20efe,0x4f,0},{0x20f00,0xb6,0xae},
              {0x20f02,0xd2,0x8d},{0x20f03,0x10,0},{0x20f04,0xa0,0xd2},{0x20f05,0x14,0x10},{0x20f06,0x8a,0x0a},{0x20f07,0x3e,0},
              {0x20f08,0x4f,0},{0x20f0a,0xa5,0x0a},{0x20f0b,0xfe,0},{0x20f0c,9,0}};
            CHECK(copy && sizeof scripted==sizeof d); memcpy(copy,r,PR_BUS_RAM_SIZE); memcpy(&scripted,&d,sizeof scripted);
            { time_script short_script={values,1,0}; pr_r08ae_006c_cpu short_cpu=scripted; uint8_t *short_ram=malloc(PR_BUS_RAM_SIZE);
              CHECK(short_ram); memcpy(short_ram,r,PR_BUS_RAM_SIZE);
              CHECK(pr_r08ae_delay_006c_cpu(short_ram,PR_BUS_RAM_SIZE,&short_cpu,scripted_time,&short_script,128)==PR_R08AE_006C_TIME);
              CHECK(short_script.next==1 && short_cpu.ip==0 && get(short_ram,((unsigned)short_cpu.ss<<4)+short_cpu.sp)==0x008d);
              { pr_r08ae_006c_cpu saved=short_cpu; uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE); CHECK(snapshot); memcpy(snapshot,short_ram,PR_BUS_RAM_SIZE);
                CHECK(scripted_time(&short_script,short_ram,&short_cpu)!=0 && short_script.next==1 && !memcmp(&saved,&short_cpu,sizeof saved) && !memcmp(snapshot,short_ram,PR_BUS_RAM_SIZE)); free(snapshot); }
              free(short_ram); }
            CHECK(pr_r08ae_delay_006c_cpu(copy,PR_BUS_RAM_SIZE,&scripted,scripted_time,&script,128)==PR_R08AE_006C_OK);
            CHECK(script.next==2 && scripted.ax==10 && scripted.bx==0 && scripted.cx==10 && scripted.dx==0 && scripted.si==0 && scripted.di==0 && scripted.bp==0x3db6 && scripted.sp==0x3db6 && scripted.cs==0x10d2 && scripted.ip==0x0165 && scripted.ds==0x14a0 && scripted.es==0x1d16 && scripted.ss==0x1d16 && scripted.flags==0x7246);
            { uint8_t *reference=malloc(SIZE); CHECK(reference); memcpy(reference,r,SIZE);
              for(j=0;j<sizeof changed/sizeof changed[0];j++) { CHECK(reference[changed[j].address]==changed[j].old_value); reference[changed[j].address]=changed[j].new_value; }
              CHECK(!memcmp(copy,reference,SIZE));
              { pr_r08ae_00dc_cpu next; CHECK(sizeof next==sizeof scripted); memcpy(&next,&scripted,sizeof next);
                CHECK(pr_r08ae_dispatch_00dc_cpu(copy,PR_R08AE_00DC_RAM_SIZE,&next)==PR_R08AE_00DC_CALL_17AE);
                CHECK(next.ax==0 && next.bx==0 && next.cx==10 && next.dx==0 && next.si==0 && next.di==0 && next.bp==0x3db6 && next.sp==0x3db0 && next.cs==0x10d2 && next.ip==0x17ae && next.ds==0x14a0 && next.es==0x1d16 && next.ss==0x1d16 && next.flags==0x7246);
                word(reference,stack+0x3db0,0x016c); word(reference,stack+0x3db2,0x10d2); word(reference,stack+0x3db4,0);
                CHECK(!memcmp(copy,reference,SIZE));
                puts("abstract dispatcher 0165: call 10D2:17AE with argument 0; CPU14 and RAM match modeled frame");
                { pr_r08ae_17ae_cpu table; unsigned k;
                  static const struct { unsigned address; uint8_t old_value,new_value; } changes[]={
                    {0x10f1f,9,0},{0x10f29,9,0},{0x10f2a,9,0},{0x10f2b,9,0},{0x10f2c,9,0},
                    {0x20f04,0xd2,0xa0},{0x20f05,0x10,0x14},{0x20f06,0x0a,0},{0x20f0a,0x0a,0}};
                  CHECK(sizeof table==sizeof next); memcpy(&table,&next,sizeof table);
                  CHECK(pr_r08ae_table_17ae_cpu(copy,PR_R08AE_17AE_RAM_SIZE,&table)==PR_R08AE_17AE_OK);
                  CHECK(table.ax==0 && table.bx==0 && table.cx==10 && table.dx==0 && table.si==0 && table.di==0 && table.bp==0x3db6 && table.sp==0x3db6 && table.cs==0x10d2 && table.ip==0x016c && table.ds==0x14a0 && table.es==0x1d16 && table.ss==0x1d16 && table.flags==0x7246);
                  for(k=0;k<sizeof changes/sizeof changes[0];k++) { CHECK(reference[changes[k].address]==changes[k].old_value); reference[changes[k].address]=changes[k].new_value; }
                  CHECK(!memcmp(copy,reference,SIZE));
                  puts("conditional 17AE model: returned 10D2:016C; CPU14 and nine model-pinned RAM changes match; no native post-port parity");
                  memcpy(&next,&table,sizeof next);
                  CHECK(pr_r08ae_dispatch_00dc_cpu(copy,PR_R08AE_00DC_RAM_SIZE,&next)==PR_R08AE_00DC_CALL_1742);
                  CHECK(next.ax==0 && next.bx==0 && next.cx==10 && next.dx==0 && next.si==0 && next.di==0 && next.bp==0x3db6 && next.sp==0x3db0 && next.cs==0x10d2 && next.ip==0x1742 && next.ds==0x14a0 && next.es==0x1d16 && next.ss==0x1d16 && next.flags==0x7246);
                  word(reference,stack+0x3db0,0x0173); word(reference,stack+0x3db2,0x10d2); word(reference,stack+0x3db4,0);
                  CHECK(!memcmp(copy,reference,SIZE));
                  puts("conditional dispatcher 016C: call 10D2:1742 with argument 0; CPU14 and RAM match modeled frame");
                  { pr_r08ae_1742_cpu again; CHECK(sizeof again==sizeof next); memcpy(&again,&next,sizeof again);
                    CHECK(pr_r08ae_table_1742_cpu(copy,PR_R08AE_1742_RAM_SIZE,&again)==PR_R08AE_1742_OK);
                    CHECK(again.ax==0 && again.bx==0 && again.cx==10 && again.dx==0 && again.si==0 && again.di==0 && again.bp==0x3db6 && again.sp==0x3db6 && again.cs==0x10d2 && again.ip==0x0173 && again.ds==0x14a0 && again.es==0x1d16 && again.ss==0x1d16 && again.flags==0x7246);
                    CHECK(reference[0x10f4f]==0xdb && reference[0x10f50]==0x0b); reference[0x10f4f]=0x2b; reference[0x10f50]=0x0d;
                    CHECK(!memcmp(copy,reference,SIZE));
                    puts("conditional second 1742 model: returned 10D2:0173; CPU14 and two model-pinned RAM changes match; no native post-port parity");
                    memcpy(&next,&again,sizeof next);
                    CHECK(pr_r08ae_dispatch_00dc_cpu(copy,PR_R08AE_00DC_RAM_SIZE,&next)==PR_R08AE_00DC_CALL_13B1);
                    CHECK(next.ax==0 && next.bx==0 && next.cx==10 && next.dx==0 && next.si==0 && next.di==0 && next.bp==0x3db6 && next.sp==0x3db2 && next.cs==0x10d2 && next.ip==0x13b1 && next.ds==0x14a0 && next.es==0x1d16 && next.ss==0x1d16 && next.flags==0x7246);
                    word(reference,stack+0x3db2,0x0177); word(reference,stack+0x3db4,0x10d2);
                    CHECK(!memcmp(copy,reference,SIZE));
                    puts("conditional dispatcher 0173: call 10D2:13B1; CPU14 and RAM match modeled frame");
                    { pr_r08ae_13b1_cpu restore; uint8_t *bound=malloc(PR_R08AE_13B1_RAM_SIZE), *full=malloc(PR_R08AE_13B1_RAM_SIZE);
                      restore_binding binding={.flags=0x7246,.input61=3}; pr_r08ae_13b1_env env_restore={&binding,abstract_restore,NULL,NULL};
                      CHECK(bound && full && sizeof restore==sizeof next); memcpy(bound,copy,PR_R08AE_13B1_RAM_SIZE); memcpy(full,copy,PR_R08AE_13B1_RAM_SIZE); memcpy(&restore,&next,sizeof restore);
                      CHECK(pr_r08ae_vector_13b1_cpu(copy,PR_R08AE_13B1_RAM_SIZE,&restore,NULL)==PR_R08AE_13B1_ENV_DOS);
                      CHECK(restore.ax==0x2508 && restore.bx==0 && restore.cx==10 && restore.dx==0xfea5 && restore.si==0 && restore.di==0 && restore.bp==0x3db6 && restore.sp==0x3da8 && restore.cs==0x10d2 && restore.ip==0x13c6 && restore.ds==0xf000 && restore.es==0x1d16 && restore.ss==0x1d16 && restore.flags==0x7046);
                      word(reference,stack+0x3db0,0x14a0); word(reference,stack+0x3dae,0x10d2); word(reference,stack+0x3dac,0x7246); word(reference,stack+0x3daa,0x10d2); word(reference,stack+0x3da8,0x13c8);
                      CHECK(!memcmp(copy,reference,SIZE));
                      puts("conditional 13B1: DOS vector restore refused at 13C6; CPU14 and full RAM match modeled frame");
                      memcpy(&restore,&next,sizeof restore);
                      CHECK(pr_r08ae_vector_13b1_cpu(bound,PR_R08AE_13B1_RAM_SIZE,&restore,&env_restore)==PR_R08AE_13B1_ENV_PORT);
                      CHECK(binding.calls==1 && restore.ax==0xffff && restore.bx==0 && restore.cx==10 && restore.dx==0xfea5 && restore.si==0 && restore.di==0 && restore.bp==0x3db6 && restore.sp==0x3db0 && restore.cs==0x10d2 && restore.ip==0x13cc && restore.ds==0x10d2 && restore.es==0x1d16 && restore.ss==0x1d16 && restore.flags==0x7246);
                      word(reference,0x20,0xfea5); word(reference,0x22,0xf000); CHECK(!memcmp(bound,reference,SIZE));
                      puts("abstract vector-table-only restore: IVT08=F000:FEA5; next OUT refused at 13CC; no DOS/PIT side-effect claim");
                      { uint8_t *missing=malloc(PR_R08AE_13B1_RAM_SIZE); pr_r08ae_13b1_cpu stopped;
                        CHECK(missing); memcpy(missing,full,PR_R08AE_13B1_RAM_SIZE); memcpy(&stopped,&next,sizeof stopped);
                        binding=(restore_binding){.flags=0x7246,.input61=3}; env_restore.port_out=restore_out; env_restore.port_in=NULL;
                        CHECK(pr_r08ae_vector_13b1_cpu(missing,PR_R08AE_13B1_RAM_SIZE,&stopped,&env_restore)==PR_R08AE_13B1_ENV_PORT);
                        CHECK(binding.calls==1 && binding.outs==2 && binding.ins==0 && stopped.ip==0x13d2 && !memcmp(missing,reference,SIZE)); free(missing); }
                      binding=(restore_binding){.flags=0x7246,.input61=3}; env_restore.port_out=restore_out; env_restore.port_in=restore_in; memcpy(&restore,&next,sizeof restore);
                      CHECK(pr_r08ae_vector_13b1_cpu(full,PR_R08AE_13B1_RAM_SIZE,&restore,&env_restore)==PR_R08AE_13B1_OK);
                      CHECK(binding.calls==1 && binding.outs==7 && binding.ins==1);
                      CHECK(restore.ax==0xffff && restore.bx==0 && restore.cx==10 && restore.dx==0xfea5 && restore.si==0 && restore.di==0 && restore.bp==0x3db6 && restore.sp==0x3db6 && restore.cs==0x10d2 && restore.ip==0x0177 && restore.ds==0x14a0 && restore.es==0x1d16 && restore.ss==0x1d16 && restore.flags==0x7246 && !memcmp(full,reference,SIZE));
                      memcpy(&next,&restore,sizeof next);
                      CHECK(pr_r08ae_dispatch_00dc_cpu(full,PR_R08AE_00DC_RAM_SIZE,&next)==PR_R08AE_00DC_RETURN);
                      CHECK(next.ax==0xffff && next.bx==0 && next.cx==10 && next.dx==0xfea5 && next.si==0 && next.di==0 && next.bp==0x3dbe && next.sp==0x3dbe && next.cs==0x2e60 && next.ip==0x0747 && next.ds==0x14a0 && next.es==0x1d16 && next.ss==0x1d16 && next.flags==0x7246 && !memcmp(full,reference,SIZE));
                      puts("abstract restore/dispatcher: IN61=03, seven ordered OUTs; returned to coordinate writer 2E60:0747; no device/native return claim");
                      { unsigned ds=(unsigned)next.ds<<4; pr_u68_step_report step_report;
                        pr_u68_step_hooks hooks={resident_returned,NULL,&next};
                        pr_u68_step_state step={.party_x=full[ds+0x6aad],.party_y=full[ds+0x6aae],.facing=full[ds+0x6aaf],
                          .word_2628=get(full,ds+0x2628),.direction_table=full+ds+0x2736,.direction_table_size=20,
                          .map_id=full[ds+0x84dc],.buffer=NULL,.hooks=&hooks};
                        CHECK(step.party_x==15 && step.party_y==4 && step.facing==6);
                        { pr_u68_step_state absent=step; pr_u68_step_report absent_report; absent.hooks=NULL;
                          CHECK(pr_u68_step_coordinate_writer(&absent,&absent_report)==PR_U68_STEP_HOOK);
                          CHECK(absent_report.stop_offset==0x0742 && absent.party_x==15 && absent.party_y==4); }
                        CHECK(pr_u68_step_coordinate_writer(&step,&step_report)==PR_U68_STEP_LOOKUP);
                        CHECK(step_report.stop_offset==0x07a1 && step_report.dx==-1 && step_report.dy==0 && step.party_x==14 && step.party_y==4);
                        full[ds+0x6aad]=step.party_x; full[ds+0x6aae]=step.party_y; reference[ds+0x6aad]=14;
                        CHECK(!memcmp(full,reference,SIZE) && next.ip==0x0747);
                        puts("conditional writer field projection: x 15->14, y 4; lookup buffer withheld; raw CPU remains at 0747"); }
                      free(full); free(bound); } } } }
              free(reference); }
            puts("abstract caller-supplied time: readings 00:00:00.00 then .10, delay returned 0165; exhausted input refuses; 42 model-pinned RAM changes");
            free(copy); }
          { pr_r08ae_006c_cpu delay; CHECK(sizeof delay==sizeof d); memcpy(&delay,&d,sizeof delay);
            CHECK(pr_r08ae_delay_006c_cpu(r,PR_R08AE_006C_RAM_SIZE,&delay,NULL,NULL,32)==PR_R08AE_006C_TIME);
            CHECK(delay.ax==10 && delay.bx==0xfea5 && delay.cx==4 && delay.dx==0x13ea && delay.si==0x3e8a && delay.di==0x4f && delay.bp==0x3dae && delay.sp==0x3da2 && delay.cs==0x10d2 && delay.ip==0 && delay.ds==0x14a0 && delay.es==0x3da1 && delay.ss==0x1d16 && delay.flags==0x7206);
            word(expected,stack+0x3dae,0x3db6); word(expected,stack+0x3da4,0x10d2); word(expected,stack+0x3da2,0x0076);
            CHECK(!memcmp(r,expected,SIZE));
            puts("conditional 006C: time service refused at 10D2:0000 with modeled CPU14 and full RAM frame");
            { pr_r08ae_time_cpu time_cpu; pr_r08ae_time_cpu_diagnostic diag; pr_bus bus={r,NULL,NULL,NULL,NULL}; unsigned calls=0,j;
              static const struct { unsigned address; uint8_t old_value,new_value; } changes[]={
                {0x20ede,0x31,0xa0},{0x20edf,0x35,0x3d},{0x20ee0,0x2c,0x1f},{0x20ee1,0x34,0x00},{0x20ee2,0x20,0xd2},{0x20ee3,0x57,0x10},
                {0x20ee4,0x20,0x94},{0x20ee5,0x31,0x3d},{0x20ee6,0x30,0x16},{0x20ee7,0x3a,0x1d},{0x20ee8,0x35,0x96},{0x20ee9,0x30,0x3d},
                {0x20eea,0x01,0x16},{0x20eeb,0x00,0x1d},{0x20eec,0x53,0x98},{0x20eed,0x00,0x3d},{0x20eee,0x01,0x16},{0x20eef,0xa0,0x1d},
                {0x20ef0,0x0b,0x9a},{0x20ef1,0xa0,0x3d},{0x20ef2,0x09,0x16},{0x20ef3,0xa0,0x1d},{0x20f00,0xb6,0xae}};
              CHECK(sizeof time_cpu==sizeof delay); memcpy(&time_cpu,&delay,sizeof time_cpu);
              CHECK(pr_r08ae_time_0000_cpu(&bus,PR_BUS_RAM_SIZE,&time_cpu,refuse_gettime,&calls,&diag)==PR_R08AE_TIME_CPU_CALLEE);
              CHECK(calls==1 && diag.gettime_status==PR_DOS_GETTIME_SERVICE && diag.longmul_status==-1);
              CHECK(time_cpu.ax==0x2c0a && time_cpu.bx==0xfea5 && time_cpu.cx==4 && time_cpu.dx==0x13ea && time_cpu.si==0x3e8a && time_cpu.di==0x3d94 && time_cpu.bp==0x3d7e && time_cpu.sp==0x3d7e && time_cpu.cs==0x1256 && time_cpu.ip==0x00a7 && time_cpu.ds==0x14a0 && time_cpu.es==0x3da1 && time_cpu.ss==0x1d16 && time_cpu.flags==0x7212);
              for(j=0;j<sizeof changes/sizeof changes[0];j++) { CHECK(expected[changes[j].address]==changes[j].old_value); expected[changes[j].address]=changes[j].new_value; }
              CHECK(!memcmp(r,expected,SIZE));
              puts("conditional staged time: GetTime refused at 1256:00A7; CPU14 and 23 model-pinned RAM changes match; no clock supplied");
            } } } }
    }
    puts("run003 1386->13a3: CPU14 and 655360 RAM bytes match; first port output not executed");
    puts("run003 1386->13a0: conditional DOS25 binding matches CPU14 and 655360 RAM bytes; missing/unsupported binding refuses");
    puts("run003 1386->139e: CPU14 and 655360 RAM bytes match; second DOS interrupt was not executed at this stop");
    puts("run003 1386->1388: conditional PSP=0814 binding matches CPU14 and 655360 RAM bytes; missing/unsupported binding refuses");
    free(r); free(expected); return 0;
}
