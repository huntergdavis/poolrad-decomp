#include "poolrad/replay/slot_a_loader_blockread_abi.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved_ram[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_slot_a_blockread_status expected)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_slot_a_blockread_report r;
 memcpy(saved_ram,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_slot_a_loader_blockread_step(s,&r)==expected);
 CHECK(!memcmp(&s->cpu,&before,sizeof before) && !memcmp(ram,saved_ram,sizeof ram));
 if(expected!=PR_SLOT_A_BLOCKREAD_BLOCKED_READ && expected!=PR_SLOT_A_BLOCKREAD_BLOCKED_CLOSE)CHECK(!r.dependency_valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_slot_a_blockread_report r,save;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x2A8Eu;s.cpu.ip=0xC44u;s.cpu.sp=0x100u;
 CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_READ)==0);
 CHECK(pr_slot_a_loader_blockread_step(&s,&r)==PR_SLOT_A_BLOCKREAD_BLOCKED_READ);
 CHECK(r.dependency_valid && r.caller==0x6Fu && r.call_pc==0xC44u && r.callee_segment==0xAF8u && r.callee_pc==0x14FBu && r.return_pc==0xC49u && r.caller_is_overlay);
 s.cpu.ip=0xC4Fu;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_CLOSE)==0);
 CHECK(pr_slot_a_loader_blockread_step(&s,&r)==PR_SLOT_A_BLOCKREAD_BLOCKED_CLOSE && r.callee_pc==0x1491u && r.return_pc==0xC54u);
 s.cpu.ip=0xC8Fu;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_SIDECAR)==0);
 s.cpu.ip=0xC35u;s.cpu.bp=0xFFF4u;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY)==0);
 s.cpu.ip=0xC33u;s.cpu.sp=1u;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY)==0);
 s.cpu.ip=0xC59u;s.cpu.es=0u;s.cpu.di=0xFEFBu;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY)==0);
 s.cpu.di=0x100u;s.cpu.ax=0xABCDu;s.cpu.flags=0xFFFFu;
 CHECK(pr_slot_a_loader_blockread_step(&s,&r)==PR_SLOT_A_BLOCKREAD_STEPPED);
 CHECK(ram[0x204u]==0xCDu && ram[0x205u]==0xABu && s.cpu.flags==0xFFFFu && s.cpu.ip==0xC5Eu);
 s.cpu.ip=0xC57u;CHECK(pr_slot_a_loader_blockread_step(&s,&r)==PR_SLOT_A_BLOCKREAD_STEPPED && s.cpu.ax==0u && (s.cpu.flags&0x8D5u)==0x44u);
 s.cpu.ip=0xC54u;s.cpu.bp=0x100u;ram[0x10Au]=0x34u;ram[0x10Bu]=0x12u;ram[0x10Cu]=0x78u;ram[0x10Du]=0x56u;
 CHECK(pr_slot_a_loader_blockread_step(&s,&r)==PR_SLOT_A_BLOCKREAD_STEPPED && s.cpu.di==0x1234u && s.cpu.es==0x5678u);
 s.cpu.ip=0xC26u;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_PC)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SLOT_A_BLOCKREAD_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);save=r;
 CHECK(pr_slot_a_loader_blockread_step(NULL,&r)==PR_SLOT_A_BLOCKREAD_ARGUMENT && !memcmp(&r,&save,sizeof r));
 CHECK(pr_slot_a_loader_blockread_step(&s,NULL)==PR_SLOT_A_BLOCKREAD_ARGUMENT);
 puts("Loader read/Close immutable gates, spans, full-word stores, flags and pointer reload passed");return 0;
}
