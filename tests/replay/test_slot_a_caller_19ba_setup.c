#include "poolrad/replay/slot_a_caller_19ba_setup.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while (0)
static uint8_t ram[655360u], copy[655360u];
static int unchanged(pr_slot_a_directory_state *s, pr_slot_a_caller_19ba_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_caller_19ba_report r;
    memset(&r, 0xA5, sizeof r); memcpy(copy, ram, sizeof ram);
    CHECK(pr_slot_a_caller_19ba_step(s, &r) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0 && memcmp(copy, ram, sizeof ram) == 0);
    if (expected != PR_SLOT_A_CALLER_19BA_BLOCKED_LOADER) CHECK(!r.dependency_valid);
    return 0;
}
int main(void)
{
    pr_slot_a_directory_state s = {0};
    pr_slot_a_caller_19ba_report r, saved;
    const uint16_t stores[] = {0x19BAu, 0x19BEu};
    const uint16_t pushes[] = {0x19C6u,0x19C7u,0x19C8u,0x19CCu,0x19D2u,0x19D5u,0x19D6u};
    s.ram = ram; s.ram_size = sizeof ram; s.cpu.cs = 0x2A8Eu;
    s.cpu.ip = 0x19BAu; s.cpu.bp = 0x400u; s.cpu.ax = 0xABCDu; s.cpu.dx = 0x1234u;
    s.cpu.flags = 0x7ED7u; s.cpu.sp = 0x100u;
    for (unsigned i = 0; i < 12u; ++i) CHECK(pr_slot_a_caller_19ba_step(&s, &r) == PR_SLOT_A_CALLER_19BA_STEPPED);
    CHECK(s.cpu.ip == 0x19D7u && s.cpu.sp == 0xF2u && s.cpu.di == 0x20Au);
    CHECK(s.cpu.ax == 0xAB01u && s.cpu.flags == 0x7ED7u);
    CHECK(ram[0x233u] == 0xCDu && ram[0x234u] == 0xABu && ram[0x235u] == 0x34u && ram[0x236u] == 0x12u);
    CHECK(ram[0xF2u] == 0x8Eu && ram[0xF3u] == 0x2Au && ram[0xF4u] == 1u && ram[0xF5u] == 0xABu);
    CHECK(ram[0xF8u] == 0xCDu && ram[0xF9u] == 0xABu && ram[0xFAu] == 0x34u && ram[0xFBu] == 0x12u);
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_LOADER) == 0);
    CHECK(pr_slot_a_caller_19ba_step(&s, &r) == PR_SLOT_A_CALLER_19BA_BLOCKED_LOADER);
    CHECK(r.dependency_valid && r.caller_unit == 0x6Fu && r.callee_unit == 0x6Fu && r.call_is_near);
    CHECK(r.call_pc == 0x19D7u && r.callee_pc == 0xAECu && r.return_pc == 0x19DAu);
    /* Both PUSH source and destination must validate before any mutation. */
    for (unsigned i = 0; i < 7u; ++i) {
        s.cpu.ip = pushes[i]; s.cpu.sp = 1u; s.cpu.bp = 0x400u;
        CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
        s.cpu.sp = 0x100u; s.cpu.bp = 0x20Bu; s.ram_size = 0xFFu;
        CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
        s.ram_size = sizeof ram;
    }
    for (unsigned i = 0; i < 2u; ++i) {
        s.cpu.ip = stores[i]; s.cpu.bp = (uint16_t)(i == 0u ? 0x1CCu : 0x1CAu);
        CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
        s.cpu.bp = 0x400u; s.ram_size = (size_t)(i == 0u ? 0x234u : 0x236u);
        CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
        s.ram_size = sizeof ram;
    }
    s.cpu.ip = 0x19C8u; s.cpu.bp = 0x1CAu; s.cpu.sp = 0x100u;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x19CCu; s.cpu.bp = 0x1CCu;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
    s.cpu.ip = 0x19C8u; s.cpu.bp = 0x400u; s.cpu.sp = 0x237u;
    CHECK(pr_slot_a_caller_19ba_step(&s, &r) == PR_SLOT_A_CALLER_19BA_STEPPED);
    CHECK(ram[0x235u] == 0x34u && ram[0x236u] == 0x12u); /* exact source/dest alias */
    s.cpu.ip = 0x19D2u; s.cpu.sp = 0u;
    CHECK(pr_slot_a_caller_19ba_step(&s, &r) == PR_SLOT_A_CALLER_19BA_STEPPED && s.cpu.sp == 0xFFFEu);
    s.cpu.ip = 0x19C2u; s.cpu.bp = 0u;
    CHECK(pr_slot_a_caller_19ba_step(&s, &r) == PR_SLOT_A_CALLER_19BA_STEPPED && s.cpu.di == 0xFE0Au);
    s.cpu.ip = 0x19D2u; s.cpu.ss = 0xFFFFu; s.cpu.sp = 0x20u;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
    s.cpu.ss = 0u; s.cpu.ip = 0x19DAu;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_PC) == 0);
    s.cpu.ip = 0x19BAu; s.cpu.cs = 0x1026u;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_PC) == 0);
    s.cpu.cs = 0x2A8Eu; s.ram_size = 0x100001u;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.ram = NULL;
    CHECK(unchanged(&s, PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY) == 0);
    memset(&r, 0xA5, sizeof r); saved = r;
    CHECK(pr_slot_a_caller_19ba_step(NULL, &r) == PR_SLOT_A_CALLER_19BA_ARGUMENT);
    CHECK(memcmp(&r, &saved, sizeof r) == 0);
    CHECK(pr_slot_a_caller_19ba_step(&s, NULL) == PR_SLOT_A_CALLER_19BA_ARGUMENT);
    puts("strict caller stores, AH/flags, wrap/alias, atomic spans and immutable near-loader gate passed");
    return 0;
}
