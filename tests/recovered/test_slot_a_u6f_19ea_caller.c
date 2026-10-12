#include "poolrad/recovered/slot_a_u6f_19ea_caller.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 0x30000u
#define SS 0x1d16u
#define ADDR(off) (((size_t)SS << 4) + (off))

static uint8_t ram[RAM_SIZE], original[RAM_SIZE];

static pr_slot_a_directory_state initial(void)
{
    pr_slot_a_directory_state s;
    memset(&s, 0, sizeof s);
    memset(ram, 0, sizeof ram);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x19eau;
    s.cpu.ds = 0x14a0u; s.cpu.es = SS; s.cpu.ss = SS;
    s.cpu.sp = 0x37b8u; s.cpu.bp = 0x3dcau;
    s.cpu.ax = 5u; s.cpu.bx = 0x35a0u;
    s.cpu.dx = 0x3526u; s.cpu.si = 0x36bbu; s.cpu.di = 0x3531u;
    s.cpu.flags = 0x7246u;
    ram[ADDR(0x3c01u)] = 1u;
    ram[ADDR(0x3bb8u)] = 6u;
    return s;
}

static void test_saved_unequal(void)
{
    pr_slot_a_directory_state s = initial();
    const uint16_t stops[] = {0x19eeu, 0x19f2u, 0x19f4u, 0x1937u};
    const uint16_t flags[] = {0x7246u, 0x7293u, 0x7293u, 0x7293u};
    size_t i;
    memcpy(original, ram, sizeof ram);
    for (i = 0; i < 4u; ++i) {
        assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
        assert(s.cpu.ip == stops[i] && s.cpu.flags == flags[i]);
        assert(s.cpu.ax == 1u && s.cpu.sp == 0x37b8u &&
               s.cpu.bp == 0x3dcau && s.cpu.ss == SS);
        assert(memcmp(ram, original, sizeof ram) == 0);
    }
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_BOUNDARY);
}

static void test_equal_boundary_and_flags(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_cpu before;
    ram[ADDR(0x3bb8u)] = 1u;
    s.cpu.ax = 0xab05u;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.ax == 0xab01u && s.cpu.flags == 0x7246u);
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.flags == 0x7246u && s.cpu.ip == 0x19f2u);
    before = s.cpu;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);

    s = initial(); s.cpu.ip = 0x19eeu; s.cpu.ax = 0x0080u;
    ram[ADDR(0x3bb8u)] = 1u;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.flags == 0x7a12u); /* 0x80-1: OF and AF set. */
}

static void test_address_and_boundary(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_cpu before = s.cpu;
    s.ram_size = ADDR(0x3c01u);
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_BOUNDARY);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);

    /* SS:BP offsets wrap at 16 bits; physical addresses wrap at 20. */
    s = initial(); s.cpu.ss = 0xffffu; s.cpu.bp = 0x0222u;
    s.ram_size = 0x100u;
    ram[0x49u] = 0x13u; ram[0] = 0x22u;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.ax == 0x0013u);
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.ip == 0x19f2u);
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_OK);
    assert(s.cpu.ip == 0x19f4u);

    s = initial(); s.cpu.cs = 0x131cu;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_BOUNDARY);
    s.cpu.cs = 0x2a8eu; s.cpu.ip = 0x1937u;
    assert(pr_slot_a_u6f_19ea_step(&s) == PR_SLOT_A_U6F_19EA_BOUNDARY);
    assert(pr_slot_a_u6f_19ea_step(NULL) == PR_SLOT_A_U6F_19EA_BOUNDARY);
}

int main(void)
{
    test_saved_unequal();
    test_equal_boundary_and_flags();
    test_address_and_boundary();
    return 0;
}
