#include "poolrad/recovered/slot_a_u6f_1937_precall.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 0x30000u
#define SS 0x1d16u
#define ADDR(off) (((size_t)SS << 4) + (off))

static uint8_t ram[RAM_SIZE], original[RAM_SIZE];

static uint16_t word_at(uint16_t off)
{
    size_t p = ADDR(off);
    return (uint16_t)(ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static pr_slot_a_directory_state initial(void)
{
    pr_slot_a_directory_state s;
    memset(&s, 0, sizeof s);
    memset(ram, 0, sizeof ram);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x1937u;
    s.cpu.ds = 0x14a0u; s.cpu.es = SS; s.cpu.ss = SS;
    s.cpu.sp = 0x37b8u; s.cpu.bp = 0x3dcau;
    s.cpu.ax = 1u; s.cpu.bx = 0x35a0u;
    s.cpu.dx = 0x3526u; s.cpu.si = 0x36bbu; s.cpu.di = 0x3531u;
    s.cpu.flags = 0x7293u;
    ram[ADDR(0x3c01u)] = 1u;
    return s;
}

static void test_saved_pre_call(void)
{
    pr_slot_a_directory_state s = initial();
    const uint16_t expected_words[] = {
        0x15e7u, 0x2a8eu, 0x3bb6u, 0x1d16u, 0x3cabu,
        0x1d16u, 0x3ab6u, 0x1d16u, 0x39b6u, 0x1d16u
    };
    size_t i;
    memcpy(original, ram, sizeof ram);
    for (i = 0; s.cpu.ip != 0x1965u; ++i) {
        assert(i < 21u);
        assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_OK);
        if (s.cpu.ip == 0x193bu)
            assert(s.cpu.flags == 0x7203u && ram[ADDR(0x3c01u)] == 2u);
        if (s.cpu.ip == 0x1947u)
            assert(s.cpu.sp == 0x37b0u && s.cpu.di == 0x3ab6u);
        if (s.cpu.ip == 0x194du)
            assert(s.cpu.ax == 2u && s.cpu.flags == 0x7246u);
        if (s.cpu.ip == 0x1952u)
            assert(s.cpu.ax == 0x52u && s.cpu.dx == 0u &&
                   s.cpu.flags == 0x7202u);
        if (s.cpu.ip == 0x1958u)
            assert(s.cpu.di == 0x3cabu);
    }
    assert(i == 21u);
    assert(s.cpu.ax == 0x52u && s.cpu.dx == 0u &&
           s.cpu.di == 0x15e7u && s.cpu.sp == 0x37a4u &&
           s.cpu.flags == 0x7202u);
    for (i = 0; i < 10u; ++i)
        assert(word_at((uint16_t)(0x37a4u + i * 2u)) == expected_words[i]);
    for (i = 0; i < sizeof ram; ++i)
        if (i != ADDR(0x3c01u) && (i < ADDR(0x37a4u) ||
                                   i >= ADDR(0x37b8u)))
            assert(ram[i] == original[i]);
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_BOUNDARY);
}

static void test_arithmetic_and_boundaries(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_cpu before;
    ram[ADDR(0x3c01u)] = 0x7fu;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_OK);
    assert(ram[ADDR(0x3c01u)] == 0x80u && s.cpu.flags == 0x7a93u);

    s = initial(); s.ram_size = ADDR(0x3c01u);
    before = s.cpu;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[ADDR(0x3c01u)] == 1u);

    s = initial(); s.cpu.ip = 0x193fu; s.cpu.ss = 0x1000u;
    s.cpu.sp = 2u; s.ram_size = 0x10001u;
    ram[0x10000u] = 0xaau;
    before = s.cpu;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x10000u] == 0xaau);

    s = initial(); s.cpu.ip = 0x193fu; s.cpu.ss = 0x1000u;
    s.cpu.sp = 1u;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_OK);
    assert(s.cpu.sp == 0xffffu && ram[0x1ffffu] == 0u &&
           ram[0x10000u] == 0x10u);

    s = initial(); s.cpu.ss = 0xffffu; s.cpu.bp = 0x0222u;
    s.ram_size = 0x100u; ram[0x49u] = 1u;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_OK);
    assert(ram[0x49u] == 2u);

    s = initial(); s.cpu.cs = 0x131cu;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_BOUNDARY);
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x1965u;
    assert(pr_slot_a_u6f_1937_step(&s) == PR_SLOT_A_U6F_1937_BOUNDARY);
    assert(pr_slot_a_u6f_1937_step(NULL) == PR_SLOT_A_U6F_1937_BOUNDARY);
}

int main(void)
{
    test_saved_pre_call();
    test_arithmetic_and_boundaries();
    return 0;
}
