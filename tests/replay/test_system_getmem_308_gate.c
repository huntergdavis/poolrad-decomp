#include "poolrad/replay/system_getmem_308_gate.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], saved[655360u];
static int blocked(pr_slot_a_directory_state *s, pr_getmem_core_status status)
{
    pr_slot_a_directory_cpu cpu = s->cpu;
    pr_system_getmem_report report;
    memcpy(saved, ram, sizeof ram);
    CHECK(pr_system_getmem_308_step(s, PR_GETMEM_FLAGS_DOSBOX_X_59915C1, &report) == status);
    CHECK(memcmp(&cpu, &s->cpu, sizeof cpu) == 0 && memcmp(saved, ram, sizeof ram) == 0);
    if (status != PR_GETMEM_CORE_BLOCKED_HOOK) CHECK(!report.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_system_getmem_report report, before;
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x131Cu;
    for (unsigned af = 0; af <= 0x10u; af += 0x10u) {
        s.cpu.ip = 0x498u; s.cpu.dx = 0x92u; s.cpu.flags = (uint16_t)(0x7206u | af);
        CHECK(pr_system_getmem_308_step(&s, PR_GETMEM_FLAGS_DOSBOX_X_59915C1, &report) == 0);
        CHECK(s.cpu.dx == 0x49u && s.cpu.flags == 0x7212u);
        s.cpu.ip = 0x498u; s.cpu.dx = 0x92u; s.cpu.flags = (uint16_t)(0x7206u | af);
        CHECK(pr_system_getmem_308_step(&s, PR_GETMEM_FLAGS_UNICORN_2_1_4, &report) == 0);
        CHECK(s.cpu.dx == 0x49u && s.cpu.flags == 0x7202u);
    }
    s.cpu.ip = 0x3B7u; s.cpu.sp = 0xFFFFu;
    ram[0x4414] = 0x34u; ram[0x4415] = 0x12u; ram[0x4416] = 0x78u; ram[0x4417] = 0x56u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_HOOK) == 0);
    CHECK(pr_system_getmem_308_step(&s, PR_GETMEM_FLAGS_UNICORN_2_1_4, &report) == PR_GETMEM_CORE_BLOCKED_HOOK);
    CHECK(report.dependency_valid && !report.call_is_near && report.call_pc == 0x3B7u);
    CHECK(report.callee_segment == 0x5678u && report.callee_pc == 0x1234u && report.return_pc == 0x3BBu);
    s.cpu.ip = 0x3C3u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_HOOK) == 0);
    CHECK(pr_system_getmem_308_step(&s, PR_GETMEM_FLAGS_UNICORN_2_1_4, &report) == PR_GETMEM_CORE_BLOCKED_HOOK);
    CHECK(report.return_pc == 0x3C7u);
    s.ram_size = 0x4417u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ip = 0x31Du; s.cpu.di = 0xFFFDu;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x30Fu; s.cpu.sp = 1u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x3BFu; s.cpu.sp = 0x100u; s.ram_size = 0x8755u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.ip = 0x3ADu; s.cpu.di = 0xFFFFu;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x4A1u; s.cpu.sp = 0xFFFFu;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x363u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_TAIL) == 0);
    s.cpu.ip = 0x376u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_TAIL) == 0);
    s.cpu.ip = 0x248u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_RETURN) == 0);
    s.cpu.ip = 0x3C7u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_PC) == 0);
    s.ram_size = 0x100001u;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(blocked(&s, PR_GETMEM_CORE_BLOCKED_MEMORY) == 0);
    memset(&report, 0xA5, sizeof report); before = report;
    CHECK(pr_system_getmem_308_step(NULL, PR_GETMEM_FLAGS_DOSBOX_X_59915C1, &report) == PR_GETMEM_CORE_ARGUMENT);
    CHECK(memcmp(&report, &before, sizeof report) == 0);
    CHECK(pr_system_getmem_308_step(&s, PR_GETMEM_FLAGS_DOSBOX_X_59915C1, NULL) == PR_GETMEM_CORE_ARGUMENT);
    CHECK(pr_system_getmem_308_step(&s, (pr_getmem_flags_profile)0, &report) == PR_GETMEM_CORE_ARGUMENT);
    puts("strict allocator flags profiles, atomic bounds and explicit tail/hook gates passed");
    return 0;
}
