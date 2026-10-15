#include "poolrad/replay/slot_a_d8f_zero_boundary.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x100000u],before[0x100000u];
static pr_slot_a_directory_state initial(void)
{
 pr_slot_a_directory_state s={0};s.ram=ram;s.ram_size=sizeof ram;
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xD8Fu;s.cpu.ss=0x1D16u;
 s.cpu.sp=0x33ECu;s.cpu.bp=0x37A6u;s.cpu.ax=0xAB00u;s.cpu.flags=0x7AD7u;return s;
}
static void blocked(pr_slot_a_directory_state *s,pr_d8f_zero_status status)
{
 pr_slot_a_directory_cpu c=s->cpu;pr_slot_a_string_0356_report r={0};memcpy(before,ram,sizeof ram);
 assert(pr_slot_a_d8f_zero_step(s,&r)==status);
 assert(!memcmp(&c,&s->cpu,sizeof c) && !memcmp(before,ram,sizeof ram));
 if(status==PR_D8F_ZERO_BLOCKED_COPY)
  assert(r.dependency_valid && r.caller==0x6Fu && r.call_pc==0xD9Bu && r.callee_segment==0xAF8u && r.callee_pc==0xB0Bu && r.return_pc==0xDA0u && r.caller_is_overlay);
 else assert(!r.dependency_valid);
}
int main(void)
{
 pr_slot_a_directory_state s=initial();pr_slot_a_string_0356_report r={0};unsigned i;
 for(i=0;i<6u;++i)assert(pr_slot_a_d8f_zero_step(&s,&r)==PR_D8F_ZERO_STEPPED);
 assert(s.cpu.sp==0x33E4u && s.cpu.di==0x36A6u && s.cpu.ax==0xAB00u && s.cpu.flags==0x7AD7u);
 assert(!memcmp(ram+0x20544u,"\xA6\x36\x16\x1D\xED\x34\x16\x1D",8));
 blocked(&s,PR_D8F_ZERO_BLOCKED_COPY);
 s=initial();s.cpu.bp=0;s.cpu.sp=0;
 for(i=0;i<6u;++i)assert(pr_slot_a_d8f_zero_step(&s,&r)==PR_D8F_ZERO_STEPPED);
 assert(s.cpu.di==0xFF00u && s.cpu.sp==0xFFF8u);
 assert(!memcmp(ram+0x2D158u,"\0\xFF\x16\x1D\x47\xFD\x16\x1D",8));
 s=initial();s.cpu.ip=0xD93u;s.cpu.sp=1;blocked(&s,PR_D8F_ZERO_BLOCKED_MEMORY);
 s=initial();s.cpu.ip=0xD9Au;s.ram_size=0x2054Bu;blocked(&s,PR_D8F_ZERO_BLOCKED_MEMORY);
 s=initial();s.cpu.ip=0xD93u;s.cpu.ss=0xFFFFu;blocked(&s,PR_D8F_ZERO_BLOCKED_MEMORY);
 s=initial();s.ram=NULL;blocked(&s,PR_D8F_ZERO_BLOCKED_MEMORY);
 s=initial();s.ram_size=sizeof ram+1u;blocked(&s,PR_D8F_ZERO_BLOCKED_MEMORY);
 s=initial();s.cpu.cs=0x131Cu;blocked(&s,PR_D8F_ZERO_BLOCKED_PC);
 s=initial();s.cpu.ip=0xDA0u;blocked(&s,PR_D8F_ZERO_BLOCKED_PC);
 r.dependency_valid=9;assert(pr_slot_a_d8f_zero_step(NULL,&r)==PR_D8F_ZERO_ARGUMENT && r.dependency_valid==9u);
 assert(pr_slot_a_d8f_zero_step(&s,NULL)==PR_D8F_ZERO_ARGUMENT);
 puts("D8F argument order, wrap, typed B0B gate and atomic failures passed");return 0;
}
