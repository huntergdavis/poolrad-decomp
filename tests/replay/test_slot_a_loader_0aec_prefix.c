#include "poolrad/replay/slot_a_loader_0aec_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], copy[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_loader_0aec_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_loader_0aec_report r;
    memset(&r,0xA5,sizeof r); memcpy(copy,ram,sizeof ram);
    CHECK(pr_slot_a_loader_0aec_step(s,&r) == expected);
    CHECK(memcmp(&before,&s->cpu,sizeof before) == 0 && memcmp(copy,ram,sizeof ram) == 0);
    if (expected != PR_SLOT_A_LOADER_0AEC_BLOCKED_FILE_OPEN) CHECK(!r.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0}; pr_slot_a_loader_0aec_report r, saved;
    const uint16_t sp[] = {0x37A6u,0u,0x8000u,0x3BAu};
    const uint16_t result[] = {0x33ECu,0xFC46u,0x7C46u,0u};
    const uint16_t flags[] = {0x7212u,0x7293u,0x7A12u,0x7246u};
    s.ram=ram; s.ram_size=sizeof ram; s.cpu.cs=0x2A8Eu;
    s.cpu.ip=0x19D7u; s.cpu.sp=0x100u; s.cpu.bp=0x400u; s.cpu.flags=0x7202u;
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
    CHECK(s.cpu.ip == 0xAECu && s.cpu.cs == 0x2A8Eu && s.cpu.sp == 0xFEu);
    CHECK(ram[0xFEu] == 0xDAu && ram[0xFFu] == 0x19u && s.cpu.flags == 0x7202u);
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
    CHECK(ram[0xFCu] == 0u && ram[0xFDu] == 4u && s.cpu.sp == 0xFCu);
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED && s.cpu.bp == 0xFCu);
    for (unsigned i=0;i<4u;++i) {
        s.cpu.ip=0xAEFu; s.cpu.sp=sp[i]; s.cpu.flags=0x7202u;
        CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
        CHECK(s.cpu.sp == result[i] && s.cpu.flags == flags[i]);
    }
    s.cpu.ip=0xAF3u; s.cpu.bp=0x100u;
    ram[0x10Eu]=0x34u; ram[0x10Fu]=0x12u; ram[0x110u]=0x78u; ram[0x111u]=0x56u;
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
    CHECK(s.cpu.di == 0x1234u && s.cpu.es == 0x5678u);
    s.cpu.ip=0xAF3u; s.cpu.bp=0xFFF1u;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.cpu.bp=0x100u; s.ram_size=0x111u;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.ram_size=sizeof ram; s.cpu.ip=0x19D7u; s.cpu.sp=1u;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.cpu.sp=0u;
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED && s.cpu.sp == 0xFFFEu);
    s.cpu.ip=0xB29u; s.cpu.sp=2u;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.cpu.sp=0x100u;
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
    CHECK(s.cpu.cs == 0x1026u && s.cpu.ip == 0x425u && s.cpu.sp == 0xFCu);
    CHECK(ram[0xFCu] == 0x2Eu && ram[0xFDu] == 0xBu && ram[0xFEu] == 0x8Eu && ram[0xFFu] == 0x2Au);
    s.cpu.cs=0x2A8Eu; s.cpu.ip=0xB58u; s.cpu.ds=0xFFFFu;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.cpu.ds=0u; s.cpu.ip=0xB07u; s.cpu.bp=0x100u; s.cpu.flags=0x7ED7u;
    ram[0x106u]=0u;
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED);
    CHECK((s.cpu.flags & 0x8D5u) == 0x44u && (s.cpu.flags & 0x400u));
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_STEPPED && s.cpu.ip == 0xB81u);
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_PATH) == 0);
    s.cpu.ip=0xBE7u; CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_PATH) == 0);
    s.cpu.ip=0xC26u;
    CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_FILE_OPEN) == 0);
    CHECK(pr_slot_a_loader_0aec_step(&s,&r) == PR_SLOT_A_LOADER_0AEC_BLOCKED_FILE_OPEN);
    CHECK(r.dependency_valid && r.caller == 0x6Fu && r.call_pc == 0xC26u && r.callee_segment == 0x802u);
    CHECK(r.callee_pc == 0x25Du && r.return_pc == 0xC2Bu && r.caller_is_overlay);
    s.cpu.ip=0xC2Bu; CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_PC) == 0);
    s.cpu.cs=0x9999u; s.cpu.ip=0xAECu; CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_PC) == 0);
    s.ram_size=0x100001u; CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    s.ram_size=sizeof ram; s.ram=NULL; CHECK(unchanged(&s,PR_SLOT_A_LOADER_0AEC_BLOCKED_MEMORY) == 0);
    memset(&r,0xA5,sizeof r); saved=r;
    CHECK(pr_slot_a_loader_0aec_step(NULL,&r) == PR_SLOT_A_LOADER_0AEC_ARGUMENT);
    CHECK(memcmp(&r,&saved,sizeof r) == 0);
    CHECK(pr_slot_a_loader_0aec_step(&s,NULL) == PR_SLOT_A_LOADER_0AEC_ARGUMENT);
    puts("strict loader entry/far frame, SUB flags, atomic spans, file and zero-path gates passed");
    return 0;
}
