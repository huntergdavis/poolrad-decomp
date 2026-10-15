#include "poolrad/replay/slot_a_loader_sidecar_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved_ram[655360u];
typedef pr_slot_a_sidecar_status (*step_fn)(pr_slot_a_directory_state *,pr_slot_a_sidecar_report *);
static int blocked(step_fn fn,pr_slot_a_directory_state *s,pr_slot_a_sidecar_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_slot_a_sidecar_report r;
 memcpy(saved_ram,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(fn(s,&r)==status);
 CHECK(!memcmp(&s->cpu,&before,sizeof before) && !memcmp(ram,saved_ram,sizeof ram));
 if(status!=PR_SLOT_A_SIDECAR_BLOCKED_ASSIGN && status!=PR_SLOT_A_SIDECAR_BLOCKED_ALLOCATOR && status!=PR_SLOT_A_SIDECAR_BLOCKED_PRINTER)CHECK(!r.dependency_valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_slot_a_sidecar_report r,save;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC9Bu;s.cpu.sp=0x100u;
 CHECK(pr_slot_a_loader_sidecar_prefix_step(&s,&r)==PR_SLOT_A_SIDECAR_STEPPED);
 CHECK(s.cpu.cs==0x131Cu && s.cpu.ip==0xB0Bu && s.cpu.sp==0xFCu);
 CHECK(ram[0xFCu]==0xA0u && ram[0xFDu]==0xCu && ram[0xFEu]==0x8Eu && ram[0xFFu]==0x2Au);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xDAAu;s.cpu.sp=0x100u;
 CHECK(pr_slot_a_loader_sidecar_prefix_step(&s,&r)==PR_SLOT_A_SIDECAR_STEPPED);
 CHECK(s.cpu.cs==0x1026u && s.cpu.ip==0x2EBu && ram[0xFCu]==0xAFu && ram[0xFDu]==0xDu);
 s.cpu.ip=0x310u;CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_ASSIGN)==0);
 CHECK(pr_slot_a_loader_sidecar_prefix_step(&s,&r)==PR_SLOT_A_SIDECAR_BLOCKED_ASSIGN);
 CHECK(r.dependency_valid && r.caller==0x802u && r.callee_segment==0xAF8u && r.callee_pc==0x13E2u && r.return_pc==0x315u);
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xCAAu;s.cpu.sp=2u;
 CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_MEMORY)==0);
 s.cpu.ip=0xCAFu;s.cpu.ax=0xAB00u;s.cpu.flags=0xFFFFu;
 CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_PC)==0);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,&r)==PR_SLOT_A_SIDECAR_STEPPED);
 CHECK(s.cpu.ax==0xAB00u && (s.cpu.flags&0x8D5u)==0x44u && s.cpu.ip==0xCB1u);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,&r)==PR_SLOT_A_SIDECAR_STEPPED && s.cpu.ip==0xCB3u);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,&r)==PR_SLOT_A_SIDECAR_STEPPED && s.cpu.ip==0xD8Fu);
 CHECK(blocked(pr_slot_a_loader_sidecar_return_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_NEXT_CONTEXT)==0);
 s.cpu.ip=0xCB9u;s.cpu.sp=1u;CHECK(blocked(pr_slot_a_loader_sidecar_return_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_MEMORY)==0);
 s.cpu.ip=0xCBAu;CHECK(blocked(pr_slot_a_loader_sidecar_return_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_ALLOCATOR)==0);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,&r)==PR_SLOT_A_SIDECAR_BLOCKED_ALLOCATOR);
 CHECK(r.dependency_valid && r.caller==0x6Fu && r.call_pc==0xCBAu && r.callee_segment==0xAF8u && r.callee_pc==0x23Fu && r.return_pc==0xCBFu && r.caller_is_overlay);
 s.cpu.ip=0xE8Du;CHECK(blocked(pr_slot_a_loader_sidecar_return_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_PRINTER)==0);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,&r)==PR_SLOT_A_SIDECAR_BLOCKED_PRINTER);
 CHECK(r.callee_segment==0x899u && r.callee_pc==0x104u && r.return_pc==0xE92u);
 s.cpu.cs=0x131Cu;s.cpu.ip=0xB1Eu;s.cpu.flags=0x400u;
 CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_PC)==0);
 s.cpu.ip=0xB10u;s.cpu.bx=0xFFF6u;CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_MEMORY)==0);
 s.ram_size=0x100001u;CHECK(blocked(pr_slot_a_loader_sidecar_return_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(pr_slot_a_loader_sidecar_prefix_step,&s,PR_SLOT_A_SIDECAR_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);save=r;
 CHECK(pr_slot_a_loader_sidecar_prefix_step(NULL,&r)==PR_SLOT_A_SIDECAR_ARGUMENT && !memcmp(&r,&save,sizeof r));
 CHECK(pr_slot_a_loader_sidecar_return_step(NULL,&r)==PR_SLOT_A_SIDECAR_ARGUMENT && !memcmp(&r,&save,sizeof r));
 CHECK(pr_slot_a_loader_sidecar_prefix_step(&s,NULL)==PR_SLOT_A_SIDECAR_ARGUMENT);
 CHECK(pr_slot_a_loader_sidecar_return_step(&s,NULL)==PR_SLOT_A_SIDECAR_ARGUMENT);
 puts("Loader sidecar strict frames, flags, separate fragments, bounds and immutable dependencies passed");return 0;
}
