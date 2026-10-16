#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_loop_0e05.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.h"
#include "poolrad/recovered/overlay_u9e_entry8_system_call_0e0e.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIZE PR_U9E_ENTRY8_PREFIX_RAM_SIZE
#define MASK (SIZE - 1u)

_Static_assert(sizeof(pr_u9e_entry8_compose_cpu) ==
               sizeof(pr_u9e_entry8_call_cpu), "CPU14 call ABI");
_Static_assert(sizeof(pr_u9e_entry8_compose_cpu) ==
               sizeof(pr_u9e_entry8_postcall_cpu), "CPU14 post ABI");
_Static_assert(sizeof(pr_u9e_entry8_compose_cpu) ==
               sizeof(pr_u9e_entry8_loop_cpu), "CPU14 loop ABI");
_Static_assert(sizeof(pr_u9e_entry8_compose_cpu) ==
               sizeof(pr_u9e_entry8_tail_cpu), "CPU14 tail ABI");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint32_t seed32(const uint8_t *ram, uint16_t ds)
{
    return (uint32_t)get16(ram, ds, 0x4428u) |
           ((uint32_t)get16(ram, ds, 0x442Au) << 16);
}

static pr_u9e_entry8_compose_cpu frame(
    uint8_t *ram, uint16_t ss, uint16_t sp, uint16_t ds,
    uint16_t range, uint16_t count, uint32_t seed)
{
    pr_u9e_entry8_compose_cpu cpu = {
        0xABCDu, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        0x7777u, sp, 0x3000u, 0x0DE3u, ds, 0x4000u, ss, 0x7212u
    };
    memset(ram, 0xA5, SIZE);
    put16(ram, ss, sp, 0x9876u);
    put16(ram, ss, (uint16_t)(sp + 2u), 0x4567u);
    put16(ram, ss, (uint16_t)(sp + 4u), range);
    put16(ram, ss, (uint16_t)(sp + 6u), count);
    put16(ram, ds, 0x4428u, (uint16_t)seed);
    put16(ram, ds, 0x442Au, (uint16_t)(seed >> 16));
    put16(ram, 0x0AF8u, 0x1245u, 0x8405u);
    return cpu;
}

/* An independent integration oracle invokes the already focused-tested
 * components with representation copies rather than the wrapper's named
 * field copies. Numeric roll and seed assertions below are separate. */
static pr_u9e_entry8_compose_status reference(
    uint8_t *ram, pr_u9e_entry8_compose_cpu *cpu, unsigned max_calls)
{
    pr_u9e_entry8_prefix_status prefix =
        pr_u9e_entry8_fullstate_prefix_0de3(ram, SIZE, cpu);
    unsigned calls = 0u;
    assert(prefix != PR_U9E_ENTRY8_PREFIX_ARGUMENT);
    if (prefix == PR_U9E_ENTRY8_PREFIX_AT_CALL) {
        for (;;) {
            pr_u9e_entry8_call_cpu call_cpu;
            pr_u9e_entry8_postcall_cpu post_cpu;
            pr_u9e_entry8_postcall_status post;
            assert(cpu->ip == 0x0E0Eu);
            if (calls == max_calls) return PR_U9E_ENTRY8_COMPOSE_LIMIT;
            memcpy(&call_cpu, cpu, sizeof call_cpu);
            assert(pr_u9e_entry8_system_call_0e0e(
                       ram, SIZE, &call_cpu) == PR_U9E_ENTRY8_CALL_RETURNED);
            memcpy(cpu, &call_cpu, sizeof call_cpu);
            ++calls;
            memcpy(&post_cpu, cpu, sizeof post_cpu);
            post = pr_u9e_entry8_fullstate_postcall_0e13(
                ram, SIZE, &post_cpu);
            assert(post != PR_U9E_ENTRY8_POSTCALL_ARGUMENT);
            memcpy(cpu, &post_cpu, sizeof post_cpu);
            if (post == PR_U9E_ENTRY8_POSTCALL_AT_TAIL) break;
            {
                pr_u9e_entry8_loop_cpu loop_cpu;
                memcpy(&loop_cpu, cpu, sizeof loop_cpu);
                assert(pr_u9e_entry8_fullstate_loop_0e05(
                           ram, SIZE, &loop_cpu) ==
                       PR_U9E_ENTRY8_LOOP_AT_CALL);
                memcpy(cpu, &loop_cpu, sizeof loop_cpu);
            }
        }
    }
    {
        pr_u9e_entry8_tail_cpu tail_cpu;
        memcpy(&tail_cpu, cpu, sizeof tail_cpu);
        assert(pr_u9e_entry8_fullstate_tail_0e1f(
                   ram, SIZE, &tail_cpu) == PR_U9E_ENTRY8_TAIL_RETURNED);
        memcpy(cpu, &tail_cpu, sizeof tail_cpu);
    }
    return PR_U9E_ENTRY8_COMPOSE_RETURNED;
}

static void check_return_case(uint8_t *ram, uint8_t *want_ram,
                              uint16_t ss, uint16_t sp, uint16_t ds,
                              uint16_t range, uint16_t count, uint32_t seed,
                              unsigned cap, uint8_t want_al,
                              uint32_t want_seed)
{
    pr_u9e_entry8_compose_cpu cpu = frame(
        ram, ss, sp, ds, range, count, seed);
    pr_u9e_entry8_compose_cpu want_cpu = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(reference(want_ram, &want_cpu, cap) ==
           PR_U9E_ENTRY8_COMPOSE_RETURNED);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, cap) == PR_U9E_ENTRY8_COMPOSE_RETURNED);
    assert(memcmp(&cpu, &want_cpu, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);
    assert(cpu.ip == 0x9876u && cpu.cs == 0x4567u);
    assert(cpu.sp == (uint16_t)(sp + 8u) && cpu.bp == 0x7777u);
    assert((uint8_t)cpu.ax == want_al);
    assert(seed32(ram, ds) == want_seed);
}

int main(void)
{
    uint8_t *ram = malloc(SIZE);
    uint8_t *want_ram = malloc(SIZE);
    pr_u9e_entry8_compose_cpu cpu, before, want_cpu;
    assert(ram != NULL && want_ram != NULL);

    /* Count zero skips SYSTEM and leaves the seed untouched. */
    check_return_case(ram, want_ram, 0x2000u, 0x7000u, 0x1000u,
                      7u, 0u, 0x12345678u, 0u, 0u, 0x12345678u);
    /* Range zero still advances the seed once. */
    check_return_case(ram, want_ram, 0x2000u, 0x7000u, 0x1000u,
                      0u, 1u, 0u, 1u, 1u, 1u);
    /* Two range-one calls each return zero before U9E adds one. */
    check_return_case(ram, want_ram, 0x2000u, 0x7000u, 0x1000u,
                      1u, 2u, 0u, 2u, 2u, 0x08088406u);
    /* SS:FFFF return IP crosses physical FFFFF into physical 00000. */
    check_return_case(ram, want_ram, 0xF000u, 0xFFFFu, 0x1000u,
                      0u, 1u, 0u, 1u, 1u, 1u);
    /* DS seed aliases both caller argument WORDs. The second call
     * rereads range 6 after the first seed step; its roll is 5. */
    check_return_case(ram, want_ram, 0x1000u, 0x4424u, 0x1000u,
                      1u, 2u, 0x00020001u, 2u, 7u, 0x2C9FAC1Fu);

    /* Limit leaves the first SYSTEM/postcall/loopback effects live and
     * stops before the second far call. */
    cpu = frame(ram, 0x2000u, 0x7000u, 0x1000u,
                1u, 2u, 0u);
    want_cpu = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(reference(want_ram, &want_cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_LIMIT);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) == PR_U9E_ENTRY8_COMPOSE_LIMIT);
    assert(cpu.ip == 0x0E0Eu && seed32(ram, cpu.ds) == 1u);
    assert(memcmp(&cpu, &want_cpu, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);

    /* Fifteen zero-range calls leave index 0F; the next loopback INC
     * carries to 10, and XOR AH preserves its AF by source convention. */
    cpu = frame(ram, 0x2000u, 0x7000u, 0x1000u,
                0u, 16u, 0u);
    want_cpu = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(reference(want_ram, &want_cpu, 15u) ==
           PR_U9E_ENTRY8_COMPOSE_LIMIT);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 15u) == PR_U9E_ENTRY8_COMPOSE_LIMIT);
    assert(cpu.ip == 0x0E0Eu && (cpu.flags & 0x08D5u) == 0x0054u);
    assert(memcmp(&cpu, &want_cpu, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);

    cpu = frame(ram, 0x2000u, 0x7000u, 0x1000u,
                7u, 1u, 0u);
    before = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_compose_0de3(NULL, SIZE, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE - 1u, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_ARGUMENT);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, NULL, 1u) == PR_U9E_ENTRY8_COMPOSE_ARGUMENT);
    cpu.ip = 0x0DE4u;
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) == PR_U9E_ENTRY8_COMPOSE_ARGUMENT);
    cpu.ip = 0x0DE3u;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);

    /* Prefix PUSH BP would write loaded U9E instruction memory. */
    cpu = frame(ram, 0x3000u, 0x0DE5u, 0x1000u,
                7u, 1u, 0u);
    before = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_PREFIX_REJECTED);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);

    /* The SYSTEM seed would write loaded U9E code at CS:+0DE8.
     * Prefix effects remain when the call adapter rejects. */
    cpu = frame(ram, 0x2000u, 0x7000u, 0x2C9Cu,
                7u, 1u, 0u);
    want_cpu = cpu;
    memcpy(want_ram, ram, SIZE);
    assert(pr_u9e_entry8_fullstate_prefix_0de3(
               want_ram, SIZE, &want_cpu) ==
           PR_U9E_ENTRY8_PREFIX_AT_CALL);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_CALL_ARGUMENT);
    assert(memcmp(&cpu, &want_cpu, sizeof cpu) == 0);
    assert(memcmp(ram, want_ram, SIZE) == 0);

    /* DS seed overwrites the pending far return, so the adapter reports
     * transfer with its live state instead of running the postcall. */
    cpu = frame(ram, 0x2000u, 0x4434u, 0x2000u,
                5u, 1u, 0u);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_RETURN_TRANSFER);
    assert(cpu.ip == get16(ram, cpu.ss, 0x4428u) &&
           cpu.cs == get16(ram, cpu.ss, 0x442Au));

    /* DS seed overwrites SYSTEM's near return before the outer RETF. */
    cpu = frame(ram, 0x2000u, 0x4436u, 0x2000u,
                5u, 1u, 0u);
    assert(pr_u9e_entry8_fullstate_compose_0de3(
               ram, SIZE, &cpu, 1u) ==
           PR_U9E_ENTRY8_COMPOSE_SYSTEM_TRANSFER);
    assert(cpu.cs == 0x0AF8u && cpu.ip != 0x11C3u);

    free(want_ram);
    free(ram);
    return 0;
}
