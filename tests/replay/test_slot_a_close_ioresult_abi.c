#include "poolrad/replay/slot_a_close_ioresult_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved_ram[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_slot_a_close_ioresult_status status)
{
    pr_slot_a_directory_cpu before=s->cpu;pr_slot_a_close_ioresult_report r;
    memcpy(saved_ram,ram,sizeof ram);memset(&r,0xA5,sizeof r);
    CHECK(pr_slot_a_close_ioresult_step(s,&r)==status);
    CHECK(!memcmp(&s->cpu,&before,sizeof before) && !memcmp(ram,saved_ram,sizeof ram));
    if(status!=PR_SLOT_A_CLOSE_IORESULT_BLOCKED_DOS)CHECK(!r.valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s={0};pr_slot_a_close_ioresult_report r,save;pr_slot_a_string_0356_report old;
    s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x1026u;s.cpu.ip=0x32Au;s.cpu.sp=0x100u;
    CHECK(pr_slot_a_file_probe_02eb_step(&s,&old)==PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME);
    CHECK(old.callee_pc==0x1491u && old.return_pc==0x32Fu && s.cpu.sp==0x100u);
    CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_STEPPED);
    CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x1491u && s.cpu.sp==0xFCu);
    CHECK(ram[0xFCu]==0x2Fu && ram[0xFDu]==3u && ram[0xFEu]==0x26u && ram[0xFFu]==0x10u);
    s.cpu.ip=0x1497u;CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_STEPPED);
    CHECK(s.cpu.sp==0xFAu && s.cpu.ip==0x14B6u && ram[0xFAu]==0x9Au && ram[0xFBu]==0x14u);
    s.cpu.ip=0x14A6u;s.cpu.ax=0x3E80u;s.cpu.bx=6u;
    CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_DOS)==0);
    CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_BLOCKED_DOS);
    CHECK(r.valid && r.original_segment==0xAF8u && r.call_pc==0x14A6u && r.return_pc==0x14A8u && r.ax==0x3E80u && r.bx==6u);
    s.cpu.ip=0x1493u;s.cpu.bx=0xFFFAu;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x14B6u;s.cpu.di=0xFFFDu;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x14B3u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x4A4u;s.cpu.ds=0u;s.cpu.ax=0xABCDu;s.ram_size=0x4427u;
    CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;ram[0x4426u]=0x34u;ram[0x4427u]=0x12u;s.cpu.flags=0xFFFFu;
    CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_STEPPED);
    CHECK(s.cpu.ax==0x1234u && ram[0x4426u]==0xCDu && ram[0x4427u]==0xABu && s.cpu.flags==0xFFFFu);
    s.cpu.cs=0x1026u;s.cpu.ip=0x32Fu;s.cpu.sp=0x100u;
    CHECK(pr_slot_a_file_probe_02eb_step(&s,&old)==PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME && old.callee_pc==0x4A2u);
    CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_STEPPED);
    CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0x4A2u && s.cpu.sp==0xFCu && ram[0xFCu]==0x34u);
    s.cpu.ip=0x4A8u;CHECK(pr_slot_a_close_ioresult_step(&s,&r)==PR_SLOT_A_CLOSE_IORESULT_STEPPED);
    CHECK(s.cpu.cs==0x1026u && s.cpu.ip==0x334u && s.cpu.sp==0x100u);
    s.cpu.ip=0x292u;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_CALLER)==0);
    s.cpu.ip=0x31Fu;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_PC)==0);
    s.cpu.cs=0x131Cu;s.cpu.ip=0x14C5u;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_PC)==0);
    s.ram_size=0x100001u;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SLOT_A_CLOSE_IORESULT_BLOCKED_MEMORY)==0);
    memset(&r,0xA5,sizeof r);save=r;
    CHECK(pr_slot_a_close_ioresult_step(NULL,&r)==PR_SLOT_A_CLOSE_IORESULT_ARGUMENT && !memcmp(&r,&save,sizeof r));
    CHECK(pr_slot_a_close_ioresult_step(&s,NULL)==PR_SLOT_A_CLOSE_IORESULT_ARGUMENT);
    puts("Close/IOResult strict frames, unchanged old gates, DOS stop, exchange and bounds passed");return 0;
}
