#include "poolrad/recovered/overlay_u9e_entry8_system_call_0e0e.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_SYSTEM_120F_RAM_SIZE
#define MASK (RAM_SIZE - 1u)

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

/* The LCALL's segment word at CS:0E11 is the SYSTEM segment (0AF8h at load base 0); the multiplier word lives in that segment. */
static pr_u9e_entry8_call_cpu frame_at(uint8_t *ram, uint16_t sp,
                                        uint16_t range, uint16_t system_cs)
{
    pr_u9e_entry8_call_cpu cpu = {
        range, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
        0x7777u, sp, 0x3000u, 0x0E0Eu, 0x1000u, 0x4000u,
        0x2000u, 0x7212u
    };
    memset(ram, 0xA5, RAM_SIZE);
    put16(ram, cpu.ss, sp, range);
    put16(ram, cpu.ds, 0x4428u, 0x2345u);
    put16(ram, cpu.ds, 0x442Au, 0x6789u);
    ram[physical(cpu.cs, 0x0E0Eu)] = 0x9Au;
    put16(ram, cpu.cs, 0x0E0Fu, 0x11C0u);
    put16(ram, cpu.cs, 0x0E11u, system_cs);
    put16(ram, system_cs, 0x1245u, 0x8405u);
    return cpu;
}

static pr_u9e_entry8_call_cpu frame(uint8_t *ram, uint16_t sp,
                                     uint16_t range)
{
    return frame_at(ram, sp, range, 0x0AF8u);
}

static void check_normal(uint8_t *ram, uint8_t *expected,
                         uint16_t sp, uint16_t range, int decoy)
{
    pr_u9e_entry8_call_cpu cpu = frame(ram, sp, range);
    pr_u9e_entry8_call_cpu reference_cpu = cpu;
    uint8_t *reference = malloc(RAM_SIZE);
    uint32_t seed = 0x67892345u * 0x08088405u + 1u;
    uint16_t random_word = (uint16_t)(seed >> 16);
    assert(reference != NULL);
    if (decoy) {
        size_t linear_cs = (physical(cpu.ss, (uint16_t)(sp - 4u)) + 2u) & MASK;
        ram[linear_cs] = 0xADu;
        ram[(linear_cs + 1u) & MASK] = 0xDEu;
    }
    memcpy(expected, ram, RAM_SIZE);
    put16(expected, cpu.ss, (uint16_t)(sp - 2u), cpu.cs);
    put16(expected, cpu.ss, (uint16_t)(sp - 4u), 0x0E13u);
    put16(expected, cpu.ss, (uint16_t)(sp - 6u), 0x11C3u);
    put16(expected, cpu.ds, 0x4428u, (uint16_t)seed);
    put16(expected, cpu.ds, 0x442Au, random_word);
    memcpy(reference, ram, RAM_SIZE);
    put16(reference, reference_cpu.ss, (uint16_t)(sp - 2u),
          reference_cpu.cs);
    put16(reference, reference_cpu.ss, (uint16_t)(sp - 4u), 0x0E13u);
    reference_cpu.sp = (uint16_t)(sp - 4u);
    reference_cpu.cs = 0x0AF8u;
    reference_cpu.ip = 0x11C0u;
    assert(pr_system_random_fullstate_11c0(reference, RAM_SIZE,
                                            &reference_cpu) ==
           PR_SYSTEM_11C0_RETURNED);
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_RETURNED);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);
    assert(memcmp(ram, reference, RAM_SIZE) == 0);
    assert(memcmp(&cpu, &reference_cpu, sizeof cpu) == 0);
    assert(cpu.ip == 0x0E13u && cpu.cs == 0x3000u &&
           cpu.sp == (uint16_t)(sp + 2u));
    assert(cpu.ax == (range ? (uint16_t)(random_word % range) : 0u));
    assert(cpu.dx == (range ? (uint16_t)(random_word / range) : random_word));
    assert(cpu.bx == range && cpu.bp == 0x7777u && cpu.si == 0x5555u &&
           cpu.di == 0x6666u && cpu.ds == 0x1000u && cpu.es == 0x4000u &&
           cpu.ss == 0x2000u);
    free(reference);
}

/* Correction 2026-10-21: the SYSTEM segment is the relocated word at CS:0E11. With the image loaded at 1000h it reads 1AF8h: the result must be
 * the base-0 result, and the alias check must follow the live segment. */
static void check_live_base(uint8_t *ram, uint8_t *expected)
{
    pr_u9e_entry8_call_cpu live, base0;
    uint8_t *other = malloc(RAM_SIZE);
    assert(other != NULL);
    live = frame_at(ram, 0x7FF8u, 7u, 0x1AF8u);
    base0 = frame_at(other, 0x7FF8u, 7u, 0x0AF8u);
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &live) == PR_U9E_ENTRY8_CALL_RETURNED);
    assert(pr_u9e_entry8_system_call_0e0e(other, RAM_SIZE, &base0) == PR_U9E_ENTRY8_CALL_RETURNED);
    assert(memcmp(&live, &base0, sizeof live) == 0);                        /* same registers, CS/IP back at 3000:0E13 */
    assert(get16(ram, 0x1000u, 0x4428u) == get16(other, 0x1000u, 0x4428u) && get16(ram, 0x1000u, 0x442Au) == get16(other, 0x1000u, 0x442Au));
    assert(get16(ram, 0x2000u, 0x7FF6u) == 0x3000u && get16(ram, 0x2000u, 0x7FF4u) == 0x0E13u && get16(ram, 0x2000u, 0x7FF2u) == 0x11C3u);
    assert(memcmp(ram + physical(0x2000u, 0x7FF0u), other + physical(0x2000u, 0x7FF0u), 16u) == 0);
    /* The alias check follows the live segment: a seed over 1AF8:11C0.. is code and is refused unchanged; a seed over 0AF8:11C0.. is not. */
    live = frame_at(ram, 0x7FF8u, 7u, 0x1AF8u);
    live.ds = 0x17D2u;                                                      /* seed at physical 1C148 = 1AF8:11C8 */
    memcpy(expected, ram, RAM_SIZE);
    {
        pr_u9e_entry8_call_cpu before = live;
        assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &live) == PR_U9E_ENTRY8_CALL_ARGUMENT);
        assert(memcmp(&live, &before, sizeof live) == 0 && memcmp(ram, expected, RAM_SIZE) == 0);
    }
    live = frame_at(ram, 0x7FF8u, 7u, 0x1AF8u);
    live.ds = 0x07D2u;                                                      /* seed at physical 0C148 = 0AF8:11C8: not code here */
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &live) == PR_U9E_ENTRY8_CALL_RETURNED);
    live = frame_at(ram, 0x7FF8u, 7u, 0x0AF8u);
    live.ds = 0x07D2u;                                                      /* the same seed at base 0 IS code */
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &live) == PR_U9E_ENTRY8_CALL_ARGUMENT);
    free(other);
}

int main(void)
{
    uint8_t *ram = malloc(RAM_SIZE);
    uint8_t *expected = malloc(RAM_SIZE);
    pr_u9e_entry8_call_cpu cpu, before;
    assert(ram != NULL && expected != NULL);

    check_live_base(ram, expected);

    check_normal(ram, expected, 0x7FF8u, 7u, 0);
    check_normal(ram, expected, 0x7FF8u, 0u, 0);
    /* Far IP at FFFE, far CS at offset 0000; linear CS is a decoy. */
    check_normal(ram, expected, 0x0002u, 5u, 1);

    /* Seed writes alias the outer far return and redirect it. */
    cpu = frame(ram, 0x442Cu, 5u);
    cpu.ds = cpu.ss;
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_RETURN_TRANSFER);
    assert(cpu.ip == get16(ram, cpu.ss, 0x4428u) &&
           cpu.cs == get16(ram, cpu.ss, 0x442Au) &&
           (cpu.ip != 0x0E13u || cpu.cs != 0x3000u));
    assert(cpu.sp == 0x442Eu);

    /* Seed writes alias SYSTEM's near return and stop at its transfer. */
    cpu = frame(ram, 0x442Eu, 5u);
    cpu.ds = cpu.ss;
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_SYSTEM_TRANSFER);
    assert(cpu.cs == 0x0AF8u && cpu.ip != 0x11C3u);

    cpu = frame(ram, 0x7FF8u, 5u);
    before = cpu;
    memcpy(expected, ram, RAM_SIZE);
    assert(pr_u9e_entry8_system_call_0e0e(NULL, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_ARGUMENT);
    assert(pr_u9e_entry8_system_call_0e0e(ram, 0u, &cpu) ==
           PR_U9E_ENTRY8_CALL_ARGUMENT);
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, NULL) ==
           PR_U9E_ENTRY8_CALL_ARGUMENT);
    cpu.ip = 0x0E0Fu;
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_ARGUMENT);
    cpu.ip = 0x0E0Eu;
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);

    /* The CS push would overwrite loaded U9E code, so reject unchanged. */
    cpu = frame(ram, 0x0DE5u, 5u);
    cpu.ss = cpu.cs;
    before = cpu;
    memcpy(expected, ram, RAM_SIZE);
    assert(pr_u9e_entry8_system_call_0e0e(ram, RAM_SIZE, &cpu) ==
           PR_U9E_ENTRY8_CALL_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(memcmp(ram, expected, RAM_SIZE) == 0);

    free(expected);
    free(ram);
    return 0;
}
