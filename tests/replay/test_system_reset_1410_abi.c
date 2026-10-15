#include "poolrad/replay/system_reset_1410_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;} } while(0)
static uint8_t ram[655360u],saved_ram[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_system_reset_status status)
{
    pr_slot_a_directory_cpu before=s->cpu;pr_system_reset_report r;
    memcpy(saved_ram,ram,sizeof ram);memset(&r,0xA5,sizeof r);
    CHECK(pr_system_reset_1410_step(s,&r)==status);
    CHECK(!memcmp(&s->cpu,&before,sizeof before) && !memcmp(ram,saved_ram,sizeof ram));
    if(status!=PR_SYSTEM_RESET_BLOCKED_DOS && status!=PR_SYSTEM_RESET_BLOCKED_CLOSE)CHECK(!r.valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s={0};pr_system_reset_report r,save;
    s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x1026u;s.cpu.ip=0x31Fu;s.cpu.sp=0x100u;
    CHECK(pr_system_reset_1410_step(&s,&r)==PR_SYSTEM_RESET_STEPPED);
    CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x1410u && s.cpu.sp==0xFCu);
    CHECK(ram[0xFCu]==0x24u && ram[0xFDu]==3u && ram[0xFEu]==0x26u && ram[0xFFu]==0x10u);
    s.cpu.ip=0x1456u;s.cpu.ax=0x3D02u;s.cpu.ds=0x1D16u;s.cpu.dx=0x3300u;
    CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_DOS)==0);
    CHECK(pr_system_reset_1410_step(&s,&r)==PR_SYSTEM_RESET_BLOCKED_DOS);
    CHECK(r.valid && r.original_segment==0xAF8u && r.call_pc==0x1456u && r.return_pc==0x1458u);
    CHECK(r.ax==0x3D02u && r.ds==0x1D16u && r.dx==0x3300u);
    s.cpu.ip=0x1442u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_CLOSE)==0);
    CHECK(pr_system_reset_1410_step(&s,&r)==PR_SYSTEM_RESET_BLOCKED_CLOSE);
    CHECK(r.valid && r.callee_pc==0x1491u && r.return_pc==0x1445u);
    s.cpu.cs=0x1026u;s.cpu.ip=0x324u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_CALLER)==0);
    s.cpu.cs=0x131Cu;s.cpu.ip=0x1419u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_PC)==0);
    s.cpu.ip=0x1422u;s.cpu.bp=0xFFF6u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x1425u;s.cpu.es=0u;s.cpu.di=0xFFFDu;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x1472u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x1435u;s.cpu.ds=0u;s.ram_size=0x4427u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.cpu.ip=0x1461u;s.cpu.di=0x100u;s.cpu.flags=0xFFFFu;
    CHECK(pr_system_reset_1410_step(&s,&r)==PR_SYSTEM_RESET_STEPPED);
    CHECK(ram[0x102u]==0xB3u && ram[0x103u]==0xD7u && s.cpu.flags==0xFFFFu);
    s.cpu.ip=0x1472u;s.cpu.sp=0x100u;ram[0x100u]=0x24u;ram[0x101u]=3u;ram[0x102u]=0x26u;ram[0x103u]=0x10u;
    CHECK(pr_system_reset_1410_step(&s,&r)==PR_SYSTEM_RESET_STEPPED);
    CHECK(s.cpu.cs==0x1026u && s.cpu.ip==0x324u && s.cpu.sp==0x10Au);
    s.ram_size=0x100001u;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SYSTEM_RESET_BLOCKED_MEMORY)==0);
    memset(&r,0xA5,sizeof r);save=r;
    CHECK(pr_system_reset_1410_step(NULL,&r)==PR_SYSTEM_RESET_ARGUMENT && !memcmp(&r,&save,sizeof r));
    CHECK(pr_system_reset_1410_step(&s,NULL)==PR_SYSTEM_RESET_ARGUMENT);
    puts("Reset strict gates, spans, real far-call/RETF6 and unsupported Create passed");return 0;
}
