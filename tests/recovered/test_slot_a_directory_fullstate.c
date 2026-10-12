#include "poolrad/recovered/slot_a_directory_fullstate.h"

#include <assert.h>
#include <string.h>

#define RAM_SIZE 0x30000u
#define SS 0x1d16u
#define DS 0x14a0u
#define ADDR(seg, off) (((size_t)(seg) << 4) + (off))

typedef struct {
    uint16_t result;
    int carry;
    int bad_old;
    int bad_pc;
    int duplicate;
    int bad_duplicate;
    int bad_address;
} service_fixture;

static uint8_t ram[RAM_SIZE];

static void put_word(size_t p, uint16_t value)
{
    ram[p] = (uint8_t)value;
    ram[p + 1u] = (uint8_t)(value >> 8);
}

static uint16_t get_word(size_t p)
{
    return (uint16_t)(ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static pr_slot_a_directory_state initial(void)
{
    pr_slot_a_directory_state s;
    memset(ram, 0, sizeof ram);
    memset(&s, 0, sizeof s);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu.cs = 0x131cu; s.cpu.ip = 0x1821u;
    s.cpu.ds = SS; s.cpu.ss = SS; s.cpu.es = SS;
    s.cpu.fs = 0x1234u; s.cpu.gs = 0x5678u;
    s.cpu.sp = 0x3522u; s.cpu.bp = 0x35a6u;
    s.cpu.ax = 0x3900u; s.cpu.bx = 0x1101u;
    s.cpu.cx = 0x2202u; s.cpu.dx = 0x3526u;
    s.cpu.si = 0x3303u; s.cpu.di = 0x4404u;
    s.cpu.flags = 0x7246u;
    put_word(ADDR(SS, 0x3522u), DS);      /* saved DS */
    put_word(ADDR(SS, 0x3524u), 0x17e5u); /* helper's near return */
    put_word(ADDR(SS, 0x35a6u), 0x37b2u); /* wrapper saved BP */
    put_word(ADDR(SS, 0x35a8u), 0x02d5u); /* far return offset */
    put_word(ADDR(SS, 0x35aau), 0x2a8eu); /* far return segment */
    put_word(ADDR(DS, 0x4426u), 0u);
    return s;
}

static pr_slot_a_directory_status service(
    void *context, const pr_slot_a_directory_cpu *before,
    const uint8_t *memory, size_t size, pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t capacity, size_t *count)
{
    service_fixture *f = context;
    assert(before->cs == 0x131cu && before->ip == 0x1821u);
    assert(before->ax == 0x3900u && size == RAM_SIZE);
    assert(memory[0x0816eu] == 0x8eu);
    if (capacity < (f->duplicate ? 3u : 2u)) return PR_SLOT_A_DIRECTORY_BAD_EVENT;
    *after = *before;
    after->ax = f->result;
    after->flags = (uint16_t)((before->flags & ~1u) | (f->carry ? 1u : 0u));
    after->ip = f->bad_pc ? 0x1824u : 0x1823u;
    writes[0] = (pr_slot_a_directory_write){0x0816eu,
        (uint8_t)(f->bad_old ? 0x77u : 0x8eu), 0x0au};
    writes[1] = (pr_slot_a_directory_write){
        f->bad_address ? RAM_SIZE : 0x2066bu, 0x33u, 0x44u};
    if (f->duplicate)
        writes[2] = (pr_slot_a_directory_write){
            0x0816eu, (uint8_t)(f->bad_duplicate ? 0x8eu : 0x0au), 0x12u};
    *count = f->duplicate ? 3u : 2u;
    return PR_SLOT_A_DIRECTORY_OK;
}

static void test_path(uint16_t result, int carry)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_write journal[3];
    service_fixture f = {result, carry, 0, 0, 1, 0, 0};
    pr_slot_a_directory_cpu expected;
    ram[0x0816eu] = 0x8eu; ram[0x2066bu] = 0x33u;

    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.ip == 0x1823u && s.cpu.ax == result);
    assert(ram[0x0816eu] == 0x12u && ram[0x2066bu] == 0x44u);
    assert(s.cpu.bx == 0x1101u && s.cpu.cx == 0x2202u &&
           s.cpu.si == 0x3303u && s.cpu.di == 0x4404u &&
           s.cpu.fs == 0x1234u && s.cpu.gs == 0x5678u);
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.ds == DS && s.cpu.sp == 0x3524u);
    assert(s.cpu.ip == (carry ? 0x1826u : 0x1829u));
    if (carry) {
        assert(get_word(ADDR(DS, 0x4426u)) == 0u);
        assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
               PR_SLOT_A_DIRECTORY_OK);
    }
    assert(s.cpu.ip == 0x1829u);
    assert(get_word(ADDR(DS, 0x4426u)) == (carry ? result : 0u));
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.cs == 0x2a8eu && s.cpu.ip == 0x02d5u &&
           s.cpu.sp == 0x35b0u && s.cpu.bp == 0x37b2u);
    expected = s.cpu;
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.cs == 0x131cu && s.cpu.ip == 0x04a2u &&
           s.cpu.sp == 0x35acu);
    assert(get_word(ADDR(SS, 0x35acu)) == 0x02dau);
    assert(get_word(ADDR(SS, 0x35aeu)) == 0x2a8eu);
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.ip == 0x04a8u && s.cpu.ax == (carry ? result : 0u));
    assert(get_word(ADDR(DS, 0x4426u)) == 0u);
    assert((s.cpu.flags & 1u) == 0u);
    assert(pr_slot_a_directory_step(&s, NULL, NULL, NULL, 0u) ==
           PR_SLOT_A_DIRECTORY_OK);
    assert(s.cpu.cs == expected.cs && s.cpu.ip == 0x02dau &&
           s.cpu.sp == expected.sp && s.cpu.bp == expected.bp);
    assert(s.cpu.ax == (carry ? result : 0u));
}

static void test_blocked_and_invalid(void)
{
    pr_slot_a_directory_state s = initial();
    pr_slot_a_directory_write journal[3];
    pr_slot_a_directory_cpu before = s.cpu;
    service_fixture f = {5u, 1, 0, 0, 0, 0, 0};
    ram[0x0816eu] = 0x8eu; ram[0x2066bu] = 0x33u;
    assert(pr_slot_a_directory_step(&s, NULL, NULL, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BLOCKED_DOS_SERVICE);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x0816eu] == 0x8eu);
    f.bad_pc = 0; f.bad_address = 1;
    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BAD_EVENT);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x0816eu] == 0x8eu && ram[0x2066bu] == 0x33u);
    f.bad_address = 0; f.duplicate = 1; f.bad_duplicate = 1;
    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BAD_EVENT);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x0816eu] == 0x8eu && ram[0x2066bu] == 0x33u);
    f.bad_old = 1;
    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BAD_EVENT);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x0816eu] == 0x8eu && ram[0x2066bu] == 0x33u);
    f.bad_old = 0; f.bad_pc = 1;
    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BAD_EVENT);
    assert(memcmp(&s.cpu, &before, sizeof before) == 0);
    assert(ram[0x0816eu] == 0x8eu);
    s.cpu.ax = 0x3800u;
    assert(pr_slot_a_directory_step(&s, service, &f, journal, 3u) ==
           PR_SLOT_A_DIRECTORY_BOUNDARY);
}

int main(void)
{
    test_path(0u, 0);
    test_path(5u, 1);
    test_path(7u, 1);
    test_blocked_and_invalid();
    return 0;
}
