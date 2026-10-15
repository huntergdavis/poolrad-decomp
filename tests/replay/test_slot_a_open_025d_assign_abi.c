#include "poolrad/replay/slot_a_open_025d_assign_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}} while (0)
static uint8_t ram[655360u],copy[655360u];
static int unchanged(pr_slot_a_directory_state *s,pr_slot_a_open_025d_status expected)
{
    pr_slot_a_directory_cpu before=s->cpu;pr_slot_a_open_025d_report r;
    memset(&r,0xA5,sizeof r);memcpy(copy,ram,sizeof ram);
    CHECK(pr_slot_a_open_025d_step(s,&r)==expected);
    CHECK(memcmp(&before,&s->cpu,sizeof before)==0 && memcmp(copy,ram,sizeof ram)==0);
    if (expected!=PR_SLOT_A_OPEN_025D_BLOCKED_RESET) CHECK(!r.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s={0};pr_slot_a_open_025d_report r,saved;
    s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC26u;s.cpu.sp=0x100u;
    CHECK(pr_slot_a_open_025d_step(&s,&r)==PR_SLOT_A_OPEN_025D_STEPPED);
    CHECK(s.cpu.cs==0x1026u && s.cpu.ip==0x25Du && s.cpu.sp==0xFCu);
    CHECK(ram[0xFCu]==0x2Bu && ram[0xFDu]==0xCu && ram[0xFEu]==0x8Eu && ram[0xFFu]==0x2Au);
    s.cpu.ip=0x263u;s.cpu.bp=0xFFEEu;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.bp=0x100u;s.ram_size=0x113u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.cpu.ip=0x310u;
    memcpy(copy,ram,sizeof ram);pr_slot_a_directory_cpu before=s.cpu;
    CHECK(pr_slot_a_file_probe_02eb_step(&s,&r)==PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME);
    CHECK(r.dependency_valid && r.callee_pc==0x13E2u);
    CHECK(memcmp(copy,ram,sizeof ram)==0 && memcmp(&before,&s.cpu,sizeof before)==0);
    CHECK(pr_slot_a_open_025d_step(&s,&r)==PR_SLOT_A_OPEN_025D_STEPPED);
    CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x13E2u && s.cpu.sp==0xF8u);
    CHECK(ram[0xF8u]==0x15u && ram[0xF9u]==3u && ram[0xFAu]==0x26u && ram[0xFBu]==0x10u);
    s.cpu.ip=0x13E5u;s.cpu.bx=0xFFF6u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x13F0u;s.cpu.es=0u;s.cpu.di=0xFFFFu;s.cpu.flags=0x7202u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x13FAu;s.cpu.di=0xFFFEu;s.cpu.cx=2u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.di=0x100u;s.cpu.cx=2u;s.ram_size=0x103u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.cpu.di=0x100u;s.cpu.ax=0xABCDu;
    CHECK(pr_slot_a_open_025d_step(&s,&r)==PR_SLOT_A_OPEN_025D_STEPPED);
    CHECK(ram[0x100u]==0xCDu && ram[0x101u]==0xABu && ram[0x102u]==0xCDu && ram[0x103u]==0xABu);
    CHECK(s.cpu.di==0x104u && s.cpu.cx==0u && s.cpu.flags==0x7202u);
    s.cpu.ip=0x1407u;s.cpu.ds=0u;s.cpu.es=0u;s.cpu.si=0x200u;s.cpu.di=0xFFFFu;s.cpu.cx=2u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.di=0x300u;s.cpu.si=0xFFFFu;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.si=0x200u;s.cpu.flags|=0x400u;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_PC)==0);
    s.cpu.flags=0x7202u;s.cpu.ip=0x140Du;s.cpu.sp=0xFFFEu;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.cpu.cs=0x1026u;s.cpu.ip=0x31Fu;
    CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_RESET)==0);
    CHECK(pr_slot_a_open_025d_step(&s,&r)==PR_SLOT_A_OPEN_025D_BLOCKED_RESET);
    CHECK(r.dependency_valid && r.caller==0x802u && r.call_pc==0x31Fu && r.callee_segment==0xAF8u);
    CHECK(r.callee_pc==0x1410u && r.return_pc==0x324u && !r.caller_is_overlay);
    s.cpu.ip=0x292u;CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_PC)==0);
    s.cpu.cs=0x9999u;CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_PC)==0);
    s.ram_size=0x100001u;CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.ram=NULL;CHECK(unchanged(&s,PR_SLOT_A_OPEN_025D_BLOCKED_MEMORY)==0);
    memset(&r,0xA5,sizeof r);saved=r;
    CHECK(pr_slot_a_open_025d_step(NULL,&r)==PR_SLOT_A_OPEN_025D_ARGUMENT && memcmp(&r,&saved,sizeof r)==0);
    CHECK(pr_slot_a_open_025d_step(&s,NULL)==PR_SLOT_A_OPEN_025D_ARGUMENT);
    puts("strict025D far frames, unchanged probe gate, Assign spans/REP and immutable Reset passed");return 0;
}
