/* Synthetic U38 +2DE4 contract tests; no original game instruction bytes. */
#include "poolrad/recovered/overlay_u38_roll_clear_2de4_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U38RC_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 +2DE4 line %d: %s\n", __LINE__, #x);        \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, DICE_CS = 0x4800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, K = 0x21 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }

static pr_u38rc_cpu world(unsigned key, uint32_t seed)
{
    pr_u38rc_cpu cpu = {0xA534u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
                       0x7777u, SP0, CS, 0x2DE4u, DS, 0x9999u, SS, 0x0603u};
    memset(ram, 0x5C, RAM);
    w8(CS, 0x2DEDu, 0x9Au); w16(CS, 0x2DEEu, 0x0048u); w16(CS, 0x2DF0u, 0x00B0u);
    w8(0x00B0u, 0x48u, 0xEAu); w16(0x00B0u, 0x49u, 0x0DE3u); w16(0x00B0u, 0x4Bu, DICE_CS);
    w16(0x0AF8u, 0x1245u, 0x8405u);
    w8(DS, 0x6DBFu, 0u); w8(DS, 0x6817u, key); w8(DS, 0x6818u, 0x77u);
    w16(DS, 0x4428u, seed & 0xFFFFu); w16(DS, 0x442Au, seed >> 16);
    w16(SS, SP0, 0x7000u); w16(SS, SP0 + 2u, 0x7001u);
    return cpu;
}

static void returned(const pr_u38rc_cpu *cpu, unsigned pops)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == SP0 + 4u + pops && cpu->bp == 0x7777u && cpu->ds == DS);
}

int main(void)
{
    pr_u38rc_cpu cpu, before;
    int callee;
    unsigned seed, cleared = 0, kept = 0, key;
    ram = malloc(RAM);
    CHECK(ram != NULL);
    for (key = 0; key < 3; ++key) {
        unsigned latch = key == 0 ? 0x0B : key == 1 ? 0x35 : 0x34;
        for (seed = 1; seed <= 64; ++seed) {
            cpu = world(latch, seed * 0x01000193u);
            CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 1, &callee) == PR_U38RC_OK);
            returned(&cpu, 10);
            CHECK((cpu.flags & 0x0600u) == 0x0600u);
            if (r8(DS, 0x6817) == 0) {
                CHECK(latch != 0x34 && r8(DS, 0x6818) == 0);
                ++cleared;
            } else {
                CHECK(r8(DS, 0x6817) == latch && r8(DS, 0x6818) == 0x77);
                ++kept;
            }
        }
    }
    CHECK(cleared > 0 && kept > 0);
    cpu = world(0x0B, 5);
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 0, &callee) == PR_U38RC_CALLEE);
    CHECK(callee == PR_U9E_ENTRY8_COMPOSE_LIMIT && cpu.cs == DICE_CS && cpu.ip == 0x0E0E);
    CHECK(r8(SS, SP0 - 3) == 0xA5 && r8(SS, SP0 - 5) == 0xA5);
    cpu = world(0x0B, 5);
    w8(0x00B0, 0x48, 0);
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 1, &callee) == PR_U38RC_CODE);
    CHECK(cpu.cs == CS && cpu.ip == 0x2DED && cpu.sp == SP0 - 6 && callee == -1);
    cpu = world(0x0B, 5);
    before = cpu;
    CHECK(pr_u38_roll_clear_2de4_cpu(NULL, RAM, &cpu, 1, &callee) == PR_U38RC_ARGUMENT);
    CHECK(memcmp(&before, &cpu, sizeof cpu) == 0 && callee == -1);
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM - 1, &cpu, 1, NULL) == PR_U38RC_ARGUMENT);
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, NULL, 1, NULL) == PR_U38RC_ARGUMENT);
    cpu.sp = 0x003F;
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 1, NULL) == PR_U38RC_ARGUMENT);
    cpu.sp = 0xFFFD;
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 1, NULL) == PR_U38RC_ARGUMENT);
    cpu.sp = SP0; cpu.ip = 0;
    CHECK(pr_u38_roll_clear_2de4_cpu(ram, RAM, &cpu, 1, NULL) == PR_U38RC_ARGUMENT);
    free(ram);
    printf("PASS: U38 +2DE4 %u checks\n", checks);
    return 0;
}
