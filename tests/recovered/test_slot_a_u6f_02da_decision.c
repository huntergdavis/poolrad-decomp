#include "poolrad/recovered/slot_a_u6f_02da_decision.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 0x30000u
#define SS 0x1d16u
#define CELL ((((size_t)SS << 4) + 0x37b0u) & 0xfffffu)

static uint8_t ram[RAM_SIZE];

static pr_slot_a_directory_state initial(uint16_t result)
{
    pr_slot_a_directory_state s;
    memset(&s, 0, sizeof s);
    memset(ram, 0, sizeof ram);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x02dau;
    s.cpu.ss = SS; s.cpu.bp = 0x37b2u;
    s.cpu.ax = result; s.cpu.flags = 0x7246u;
    ram[CELL] = 0xe6u; ram[CELL + 1u] = 0x19u;
    return s;
}

static void test_error_five(void)
{
    pr_slot_a_directory_state s = initial(5u);
    pr_slot_a_directory_cpu before = s.cpu;
    const uint16_t stops[] = {0x02ddu, 0x02e1u, 0x02e3u,
                              0x02e7u, 0x031fu};
    const uint16_t flags[] = {0x7246u, 0x7206u, 0x7206u,
                              0x7246u, 0x7246u};
    size_t i;
    for (i = 0; i < 5u; ++i) {
        assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
        assert(s.cpu.ip == stops[i] && s.cpu.flags == flags[i]);
        assert(ram[CELL] == 5u && ram[CELL + 1u] == 0u);
        assert(s.cpu.ax == before.ax && s.cpu.bp == before.bp &&
               s.cpu.ss == before.ss && s.cpu.sp == before.sp);
    }
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_BOUNDARY);
    assert(s.cpu.ip == 0x031fu);
}

static void test_zero_and_other(void)
{
    pr_slot_a_directory_state s = initial(0u);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.ip == 0x02e1u && s.cpu.flags == 0x7246u);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.ip == 0x031fu);

    s = initial(7u);
    while (s.cpu.ip != 0x02e9u)
        assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.flags == 0x7202u); /* 7-5=2: positive, odd parity. */
    assert(ram[CELL] == 7u && ram[CELL + 1u] == 0u);

    /* The CMP reads the stored word, not AX, if memory changes between
     * instructions. This is a synthetic interruption boundary. */
    s = initial(5u);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    ram[CELL] = 0u;
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.flags == 0x7246u);
}

static void test_flags_and_wrap(void)
{
    pr_slot_a_directory_state s = initial(0u);
    size_t base = (size_t)0x1000u << 4;
    s.cpu.ss = 0x1000u; s.cpu.bp = 1u; s.cpu.ax = 0x8000u;
    ram[base + 0xffffu] = 0x77u; ram[base] = 0x88u;
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(ram[base + 0xffffu] == 0u && ram[base] == 0x80u);
    assert(ram[base + 0x10000u] == 0u);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.flags == 0x7286u); /* SF set, low byte has even parity. */
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_OK);
    assert(s.cpu.flags == 0x7a12u); /* 0x8000-5: AF and OF set. */
}

static void test_boundary(void)
{
    pr_slot_a_directory_state s = initial(5u);
    pr_slot_a_directory_cpu before = s.cpu;
    uint8_t low = ram[CELL], high = ram[CELL + 1u];
    s.ram_size = CELL + 1u;
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[CELL] == low && ram[CELL + 1u] == high);
    s.ram_size = sizeof ram; s.cpu.cs = 0x2a8fu;
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_BOUNDARY);
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x031fu;
    assert(pr_slot_a_u6f_02da_step(&s) == PR_SLOT_A_U6F_02DA_BOUNDARY);
    assert(pr_slot_a_u6f_02da_step(NULL) == PR_SLOT_A_U6F_02DA_BOUNDARY);
}

int main(void)
{
    test_error_five();
    test_zero_and_other();
    test_flags_and_wrap();
    test_boundary();
    return 0;
}
