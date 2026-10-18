/* Focused synthetic test for u86 +07D6 (docs/re/U86_ROLL_VALUE_07D6.md), composing the merged U9E +0DE3 dice chain and
 * the raw-CPU RES_00BA_26C2. No game bytes: the chain does not read code, so only the far-call sites, the loaded
 * stub and synthetic DS data are present. */
#include "poolrad/recovered/overlay_u86_roll_value_07d6.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U86_07D6_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u86 +07D6 line %d: %s\n", __LINE__, #x);            \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, DICE_CS = 0x4800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static pr_u86_07d6_cpu world(unsigned k, unsigned mode, unsigned seed_low, unsigned seed_high)
{
    static const uint16_t dice_sites[5] = {0x07E9u, 0x0809u, 0x0820u, 0x0846u, 0x085Du};
    pr_u86_07d6_cpu cpu = {0x1234u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, CS, 0x07D6u, DS, 0x9999u, SS, 0x0202u};
    unsigned i;
    memset(ram, 0x5C, RAM);
    for (i = 0; i < 5u; ++i) {
        w8(CS, dice_sites[i], 0x9Au); w16(CS, dice_sites[i] + 1u, 0x0048u); w16(CS, dice_sites[i] + 3u, 0x00B0u);
    }
    w8(CS, 0x0880u, 0x9Au); w16(CS, 0x0881u, 0x26C2u); w16(CS, 0x0883u, 0x00BAu);
    w8(0x00B0u, 0x48u, 0xEAu); w16(0x00B0u, 0x49u, 0x0DE3u); w16(0x00B0u, 0x4Bu, DICE_CS);
    w8(DS, 0x49F3u, mode);
    w16(DS, 0x4428u, seed_low); w16(DS, 0x442Au, seed_high);
    w8(DS, 0x6DBFu, 0u);
    w16(DS, 0x5D92u, 0x0100u); w16(DS, 0x5D94u, 0x6000u);
    w16(SS, SP0, 0x7000u); w16(SS, SP0 + 2u, 0x7001u); w16(SS, SP0 + 4u, 0xC300u | k);
    return cpu;
}

static void returned(const pr_u86_07d6_cpu *cpu)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == SP0 + 6u && cpu->bp == 0x7777u && cpu->ds == DS);
    CHECK(r16(SS, SP0 - 4u) == cpu->ax && r16(SS, SP0 - 6u) == cpu->ax);    /* [BP-2] and [BP-4] */
}

static unsigned roll_value(unsigned k, unsigned mode, unsigned seed, int *status_out)
{
    pr_u86_07d6_cpu cpu = world(k, mode, seed & 0xFFFFu, seed >> 16);
    int callee = 99;
    CHECK(pr_u86_roll_value_07d6(ram, RAM, &cpu, 256u, &callee) == PR_U86_07D6_OK);
    CHECK(callee == PR_U9E_ENTRY8_COMPOSE_RETURNED);
    CHECK(r16(DS, 0x4428u) != (seed & 0xFFFFu) || r16(DS, 0x442Au) != (seed >> 16));   /* the chain stepped the seed */
    returned(&cpu);
    *status_out = callee;
    return cpu.ax;
}

int main(void)
{
    pr_u86_07d6_cpu cpu;
    int callee, status;
    unsigned v, seed;
    ram = malloc(RAM);
    CHECK(ram != NULL);

    /* k = 43h: the constant 5A0h with CMP 43h,43h's flags, no callee. */
    cpu = world(0x43u, 0u, 1u, 2u);
    CHECK(pr_u86_roll_value_07d6(ram, RAM, &cpu, 256u, &callee) == PR_U86_07D6_OK && callee == -1);
    CHECK(cpu.ax == 0x05A0u && (cpu.flags & 0x08D5u) == 0x0044u);
    returned(&cpu);

    /* Another k: T[5] * RES_00BA_26C2(k) + T[4] with class 2 (RES gives 0Ch): 3 * 12 + 7 = 43, ADD flags of 36 + 7. */
    cpu = world(0x10u, 0u, 1u, 2u);
    w8(DS, 0x31FAu + 0x100u, 2u); w8(DS, 0x31FEu + 0x100u, 7u); w8(DS, 0x31FFu + 0x100u, 3u);
    CHECK(pr_u86_roll_value_07d6(ram, RAM, &cpu, 256u, &callee) == PR_U86_07D6_OK && callee == 0);
    CHECK(cpu.ax == 43u && cpu.dx == 36u && (cpu.flags & 0x08D5u) == 0x0004u);   /* 36 + 7 = 2Bh: even parity, no AF carry */
    returned(&cpu);

    /* The dice paths stay in range and are deterministic for a seed. */
    for (seed = 0x13579BDFu; seed < 0x13579BDFu + 40u * 0x01010101u; seed += 0x01010101u) {
        v = roll_value(0x28u, 0u, seed, &status);
        CHECK(v % 10u == 0u && v >= 10u && v <= 60u);
        CHECK(roll_value(0x28u, 0u, seed, &status) == v);
        v = roll_value(0x39u, 0u, seed, &status);
        CHECK(v >= 5u && v <= 20u);
        v = roll_value(0x3Du, 0u, seed, &status);
        CHECK(v >= 5u && v <= 20u);
        v = roll_value(0x3Bu, 0u, seed, &status);
        CHECK(v == 50u || v == 60u || v == 70u || v == 80u);
        v = roll_value(0x3Fu, 5u, seed, &status);
        CHECK(v % 10u == 0u && v >= 20u && v <= 200u);
        v = roll_value(0x3Fu, 0u, seed, &status);
        CHECK(v % 10u == 0u && v >= 110u && v <= 200u);
    }

    /* A zero SYSTEM-call cap stops the chain inside its first roll: CALLEE with the chain's LIMIT. */
    cpu = world(0x28u, 0u, 1u, 2u);
    CHECK(pr_u86_roll_value_07d6(ram, RAM, &cpu, 0u, &callee) == PR_U86_07D6_CALLEE && callee == PR_U9E_ENTRY8_COMPOSE_LIMIT);

    /* A stub that is not in its loaded form stops with CODE after the two argument pushes. */
    cpu = world(0x28u, 0u, 1u, 2u);
    w8(0x00B0u, 0x48u, 0xCDu);
    CHECK(pr_u86_roll_value_07d6(ram, RAM, &cpu, 256u, &callee) == PR_U86_07D6_CODE && cpu.sp == SP0 - 2u - 4u - 4u);

    /* Argument rejections. */
    cpu = world(0x28u, 0u, 1u, 2u);
    CHECK(pr_u86_roll_value_07d6(NULL, RAM, &cpu, 256u, &callee) == PR_U86_07D6_ARGUMENT && callee == -1);
    CHECK(pr_u86_roll_value_07d6(ram, RAM - 1u, &cpu, 256u, NULL) == PR_U86_07D6_ARGUMENT);
    CHECK(pr_u86_roll_value_07d6(ram, RAM, NULL, 256u, NULL) == PR_U86_07D6_ARGUMENT);
    CHECK(cpu.sp == SP0);

    free(ram);
    printf("PASS: u86 +07D6, %u checks (constant, table path, dice ranges and determinism, CALLEE, CODE, arguments)\n", checks);
    return 0;
}
