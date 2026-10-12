#include "poolrad/recovered/slot_a_u6f_031f_return.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 0x30000u
#define SS 0x1d16u
#define ADDR(off) (((size_t)SS << 4) + (off))

static uint8_t ram[RAM_SIZE], original[RAM_SIZE];

static void put_word(size_t linear, uint16_t value)
{
    ram[linear] = (uint8_t)value;
    ram[linear + 1u] = (uint8_t)(value >> 8);
}

static pr_slot_a_directory_state initial(void)
{
    pr_slot_a_directory_state s;
    memset(&s, 0, sizeof s);
    memset(ram, 0, sizeof ram);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x031fu;
    s.cpu.ds = 0x14a0u; s.cpu.es = SS; s.cpu.ss = SS;
    s.cpu.sp = 0x35b0u; s.cpu.bp = 0x37b2u;
    s.cpu.ax = 5u; s.cpu.flags = 0x7246u;
    put_word(ADDR(0x37b0u), 5u);
    put_word(ADDR(0x37b2u), 0x3dcau);
    put_word(ADDR(0x37b4u), 0x19eau);
    put_word(ADDR(0x37b6u), 0x2a8eu);
    return s;
}

static void test_saved_return(void)
{
    pr_slot_a_directory_state s = initial();
    const uint16_t stops[] = {0x0323u, 0x0325u, 0x0329u, 0x032bu,
                              0x032du, 0x032eu, 0x19eau};
    const uint16_t flags[] = {0x7206u, 0x7206u, 0x7246u, 0x7246u,
                              0x7246u, 0x7246u, 0x7246u};
    size_t i;
    memcpy(original, ram, sizeof ram);
    for (i = 0; i < 7u; ++i) {
        assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_OK);
        assert(s.cpu.ip == stops[i] && s.cpu.flags == flags[i]);
        assert(memcmp(ram, original, sizeof ram) == 0);
        assert(s.cpu.ax == 5u && s.cpu.cs == 0x2a8eu);
    }
    assert(s.cpu.bp == 0x3dcau && s.cpu.sp == 0x37b8u);
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
}

static void test_other_results_stop(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_cpu before = s.cpu;
    put_word(ADDR(0x37b0u), 0u);
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    put_word(ADDR(0x37b0u), 7u);
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);

    s = initial(); s.cpu.ip = 0x0323u; s.cpu.flags |= 0x0040u;
    before = s.cpu;
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    s.cpu.ip = 0x0329u; s.cpu.flags &= (uint16_t)~0x0040u;
    before = s.cpu;
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
}

static void test_stack_and_bounds(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_cpu before;
    size_t base = (size_t)0x1000u << 4;

    /* POP BP uses SS:FFFF, then SS:0000 for the second byte. */
    s.cpu.ip = 0x032du; s.cpu.ss = 0x1000u; s.cpu.sp = 0xffffu;
    ram[base + 0xffffu] = 0xcau; ram[base] = 0x3du;
    put_word(base + 1u, 0x19eau);
    put_word(base + 3u, 0x2a8eu);
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_OK);
    assert(s.cpu.bp == 0x3dcau && s.cpu.sp == 1u && s.cpu.ip == 0x032eu);
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_OK);
    assert(s.cpu.ip == 0x19eau && s.cpu.sp == 5u);

    s = initial(); s.cpu.ip = 0x032du; s.cpu.ss = 0x3000u;
    before = s.cpu;
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);

    s = initial(); s.cpu.ip = 0x032eu; s.cpu.sp = 0x37b4u;
    put_word(ADDR(0x37b6u), 0x1111u);
    before = s.cpu;
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    s.cpu.cs = 0x131cu;
    assert(pr_slot_a_u6f_031f_step(&s) == PR_SLOT_A_U6F_031F_BOUNDARY);
    assert(pr_slot_a_u6f_031f_step(NULL) == PR_SLOT_A_U6F_031F_BOUNDARY);
}

int main(void)
{
    test_saved_return();
    test_other_results_stop();
    test_stack_and_bounds();
    return 0;
}
