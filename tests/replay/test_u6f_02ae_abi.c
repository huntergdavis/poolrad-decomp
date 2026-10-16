#include "poolrad/replay/u6f_02ae_abi.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static uint8_t ram[655360u], reference_ram[655360u], before_ram[655360u];

static size_t at(uint16_t seg, uint16_t off) { return ((size_t)seg << 4) + off; }
static uint16_t get(uint16_t seg, uint16_t off)
{
    size_t p = at(seg, off);
    return (uint16_t)(ram[p] | ((uint16_t)ram[p + 1u] << 8));
}
static void put(uint16_t seg, uint16_t off, uint16_t v)
{
    size_t p = at(seg, off); ram[p] = (uint8_t)v; ram[p + 1u] = (uint8_t)(v >> 8);
}

static pr_slot_a_directory_state initial(unsigned length, uint16_t ret_cs,
                                         uint16_t ret_ip, uint16_t old_error)
{
    pr_slot_a_directory_state s = {0};
    unsigned i;
    memset(ram, 0, sizeof ram);
    s.ram = ram; s.ram_size = sizeof ram;
    s.cpu = (pr_slot_a_directory_cpu){
        .ax=0xaa66, .bx=0x1234, .cx=0x3322, .dx=0x2222,
        .si=0x4444, .di=0x5555, .bp=0x7000, .sp=0x6000,
        .cs=0x2a8e, .ds=0x1000, .es=0x1000, .ss=0x2000,
        .fs=0x3333, .gs=0x4444, .ip=0x02ae, .flags=0x7206
    };
    put(s.cpu.ss, s.cpu.sp, ret_ip);
    put(s.cpu.ss, (uint16_t)(s.cpu.sp + 2u), ret_cs);
    put(s.cpu.ds, 0x4426, old_error);
    ram[at(s.cpu.ds, 0x537a)] = (uint8_t)length;
    for (i = 0; i < length; ++i)
        ram[at(s.cpu.ds, (uint16_t)(0x537bu + i))] = (uint8_t)('A' + i % 26u);
    return s;
}

typedef struct { unsigned length, calls; uint16_t result; int carry, invalid; } event;

static pr_slot_a_directory_status dos_event(
    void *context, const pr_slot_a_directory_cpu *before,
    const uint8_t *memory, size_t size, pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t capacity, size_t *count)
{
    event *e = context;
    size_t p = at(before->ds, before->dx);
    unsigned i;
    if (before->cs != 0x131c || before->ip != 0x1821 ||
        (before->ax >> 8) != 0x39 || before->ds != before->ss ||
        capacity < 1u || p + e->length >= size)
        return PR_SLOT_A_DIRECTORY_BAD_EVENT;
    for (i = 0; i < e->length; ++i)
        if (memory[p + i] != (uint8_t)('A' + i % 26u))
            return PR_SLOT_A_DIRECTORY_BAD_EVENT;
    if (memory[p + e->length] != 0u) return PR_SLOT_A_DIRECTORY_BAD_EVENT;
    ++e->calls;
    *after = *before; after->ip = 0x1823; after->ax = e->result;
    after->flags = (uint16_t)((before->flags & (uint16_t)~1u) | !!e->carry);
    writes[0] = (pr_slot_a_directory_write){0x50, memory[0x50], 0x9b};
    if (e->invalid) writes[0].old_byte ^= 1u;
    *count = 1u;
    return PR_SLOT_A_DIRECTORY_OK;
}

static int reach(pr_slot_a_directory_state *s, uint16_t cs, uint16_t ip,
                 event *e)
{
    unsigned i;
    pr_slot_a_directory_write journal[1];
    for (i = 0; i < 400u; ++i) {
        if (s->cpu.cs == cs && s->cpu.ip == ip) return 0;
        CHECK(pr_u6f_02ae_step(s, e ? dos_event : NULL, e, journal, 1u)
              == PR_U6F_02AE_STEPPED);
    }
    CHECK(0 && "external test observation budget exhausted");
    return 1;
}

static int full_prefix(unsigned length, uint16_t service_result, int carry,
                        uint16_t old_error, uint16_t expected_result)
{
    pr_slot_a_directory_state s = initial(length, 0x1111, 0x3456, old_error), ref;
    pr_slot_a_directory_write journal[1];
    event e = {length ? length - 1u : 0u, 0u, service_result, carry, 0};
    unsigned steps;
    if (e.length > 127u) e.length = 127u;
    CHECK(reach(&s, 0x131c, 0x1821, NULL) == 0);
    ref = s; memcpy(reference_ram, ram, sizeof ram); ref.ram = reference_ram;
    for (steps = 0; steps < 12u &&
         !(ref.cpu.cs == 0x2a8e && ref.cpu.ip == 0x02da); ++steps)
        CHECK(pr_slot_a_directory_step(&ref, dos_event, &e, journal, 1u)
              == PR_SLOT_A_DIRECTORY_OK);
    CHECK(ref.cpu.cs == 0x2a8e && ref.cpu.ip == 0x02da);
    CHECK(reach(&s, 0x2a8e, 0x02da, &e) == 0);
    CHECK(e.calls == 2u);
    CHECK(memcmp(&s.cpu, &ref.cpu, sizeof s.cpu) == 0);
    CHECK(memcmp(ram, reference_ram, sizeof ram) == 0);
    CHECK(s.cpu.ax == expected_result && get(0x1000, 0x4426) == 0u);
    CHECK(pr_u6f_02ae_step(&s, dos_event, &e, journal, 1u) == PR_U6F_02AE_STEPPED);
    CHECK(get(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u)) == expected_result);
    for (steps = 0; steps < 30u; ++steps) {
        pr_u6f_02ae_status r = pr_u6f_02ae_step(&s, dos_event, &e, journal, 1u);
        if (expected_result != 0u && expected_result != 5u) {
            if (r == PR_U6F_02AE_BLOCKED_MESSAGE) {
                CHECK(s.cpu.ip == 0x02e9 && e.calls == 2u); return 0;
            }
        } else if (r == PR_U6F_02AE_RETURNED) {
            CHECK(s.cpu.cs == 0x1111 && s.cpu.ip == 0x3456);
            CHECK(s.cpu.sp == 0x6004 && s.cpu.bp == 0x7000);
            CHECK(s.cpu.fs == 0x3333 && s.cpu.gs == 0x4444);
            return 0;
        }
        CHECK(r == PR_U6F_02AE_STEPPED);
    }
    CHECK(0 && "expected message boundary or generic return"); return 1;
}

static int unchanged(pr_slot_a_directory_state *s, event *e,
                     pr_u6f_02ae_status expected)
{
    pr_slot_a_directory_cpu before = s->cpu;
    pr_slot_a_directory_write journal[1];
    memcpy(before_ram, ram, sizeof ram);
    CHECK(pr_u6f_02ae_step(s, e ? dos_event : NULL, e, journal, 1u) == expected);
    CHECK(memcmp(&before, &s->cpu, sizeof before) == 0);
    CHECK(memcmp(before_ram, ram, sizeof ram) == 0);
    return 0;
}

static int blockers_and_live_result(void)
{
    pr_slot_a_directory_state s = initial(5u, 0x2345, 0x6789, 0u);
    pr_slot_a_directory_write journal[1];
    event e = {4u, 0u, 5u, 1, 1};
    CHECK(reach(&s, 0x131c, 0x1821, NULL) == 0);
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_DOS_SERVICE) == 0);
    CHECK(unchanged(&s, &e, PR_U6F_02AE_BAD_EVENT) == 0);
    CHECK(ram[0x50] == 0u);
    s = initial(5u, 0x2345, 0x6789, 0u);
    s.cpu.ip = 0x02cb; s.cpu.sp = 2u;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_MEMORY) == 0);
    s.cpu.cs = 0x131c; s.cpu.ip = 0x1826;
    s.ram_size = at(s.cpu.ds, 0x4426) + 1u;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_MEMORY) == 0);
    s = initial(5u, 0x2345, 0x6789, 0u);
    CHECK(reach(&s, 0x2a8e, 0x02cb, NULL) == 0);
    s.cpu.ip = 0x031f;
    put(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u), 2u);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, journal, 1u) == PR_U6F_02AE_STEPPED);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, journal, 1u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.ip == 0x0325);
    /* Mutate the actual result between the two CMP instructions. */
    put(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u), 5u);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, journal, 1u) == PR_U6F_02AE_STEPPED);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, journal, 1u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.ip == 0x032b);
    s.cpu.ip = 0x031f;
    put(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u), 2u);
    CHECK(reach(&s, 0x2a8e, 0x02b5, NULL) == 0);
    CHECK(get(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u)) == 2u);
    s.cpu.ip = 0x02e9;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_MESSAGE) == 0);
    s.cpu.ip = 0x02aa;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_PC) == 0);
    return 0;
}

static int ordered_copy(void)
{
    pr_slot_a_directory_state s = initial(0u, 0u, 0u, 0u);
    s.cpu.cs = 0x131c; s.cpu.ip = 0x1814;
    s.cpu.ds = s.cpu.es = 0; s.cpu.si = 0x100; s.cpu.di = 0x101;
    s.cpu.cx = 3; ram[0x100] = 'A'; ram[0x101] = 'B'; ram[0x102] = 'C';
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(ram[0x101] == 'A' && ram[0x102] == 'A' && ram[0x103] == 'A');
    CHECK(s.cpu.si == 0x103 && s.cpu.di == 0x104 && s.cpu.cx == 0u);
    s.cpu.ip = 0x1814; s.cpu.si = 0x100; s.cpu.di = 0x101;
    s.cpu.cx = 3; s.ram_size = 0x103;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_MEMORY) == 0);
    s.ram_size = sizeof ram; s.cpu.cx = 0;
    s.cpu.ds = s.cpu.es = s.cpu.si = s.cpu.di = 0xffff;
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    s.cpu.ip = 0x1814; s.cpu.flags |= 0x400;
    CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_PC) == 0);
    return 0;
}

static int generic_returns(void)
{
    static const uint16_t ips[] = {0x0027,0x01c3,0x07f0,0x160d,0x19ea,0x1dc4};
    size_t i;
    for (i = 0; i < sizeof ips / sizeof ips[0]; ++i) {
        pr_slot_a_directory_state s = initial(0u, 0x2a8e, ips[i], 0u);
        s.cpu.ip = 0x032e;
        CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_RETURNED);
        CHECK(s.cpu.cs == 0x2a8e && s.cpu.ip == ips[i] && s.cpu.sp == 0x6004);
    }
    {
        pr_slot_a_directory_state s = initial(0u, 0x1234, 0x5678, 0u);
        s.cpu.ip = 0x032e; s.cpu.sp = 0xffff;
        CHECK(unchanged(&s, NULL, PR_U6F_02AE_BLOCKED_MEMORY) == 0);
        CHECK(pr_u6f_02ae_step(NULL, NULL, NULL, NULL, 0u) == PR_U6F_02AE_ARGUMENT);
        CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 1u) == PR_U6F_02AE_ARGUMENT);
    }
    return 0;
}

static int defined_flags(void)
{
    pr_slot_a_directory_state s = initial(0u, 0u, 0u, 0u);
    s.cpu.ip = 0x02b1; s.cpu.sp = 0x1000; s.cpu.flags = 0x7207;
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.sp == 0x0dfe && s.cpu.flags == 0x7212);
    s.cpu.ip = 0x02c9; s.cpu.ax = 0; s.cpu.flags = 0x7203;
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.ax == 0xffff && s.cpu.flags == 0x7297); /* DEC retains CF. */
    s.cpu.ip = 0x031f; s.cpu.flags = 0x7246;
    put(s.cpu.ss, (uint16_t)(s.cpu.bp - 2u), 5u);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.flags == 0x7206);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.ip == 0x0325);
    CHECK(pr_u6f_02ae_step(&s, NULL, NULL, NULL, 0u) == PR_U6F_02AE_STEPPED);
    CHECK(s.cpu.flags == 0x7246);
    return 0;
}

int main(void)
{
    CHECK(full_prefix(5u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(5u,0u,0,0u,0u) == 0);
    CHECK(full_prefix(5u,2u,1,0u,2u) == 0);
    CHECK(full_prefix(5u,0xffffu,1,0u,0xffffu) == 0);
    CHECK(full_prefix(0u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(1u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(128u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(129u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(255u,5u,1,0u,5u) == 0);
    CHECK(full_prefix(5u,0u,0,5u,5u) == 0);
    CHECK(full_prefix(5u,0x1234u,0,0u,0u) == 0);
    CHECK(full_prefix(5u,0u,0,2u,2u) == 0);
    CHECK(blockers_and_live_result() == 0);
    CHECK(ordered_copy() == 0);
    CHECK(generic_returns() == 0);
    CHECK(defined_flags() == 0);
    puts("U6F02AE: 12 synthetic directory CPU/full-RAM differential cases; "
         "6 generic returns; live-result, event, bounds and overlap gates passed");
    return 0;
}
