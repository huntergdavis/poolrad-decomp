/* Focused synthetic test for u38 +2915 and entries 89/90/91 (docs/re/U38_ROLL_GATE_2915_CPU.md). No game bytes: only the
 * far-call sites, the loaded 00B0:0048 stub and synthetic DS data; the composed dice chain does not read code. */
#include "poolrad/recovered/overlay_u38_roll_gate_2915_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U38R_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u38 cluster 2 line %d: %s\n", __LINE__, #x);        \
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

/* DS:681B = K whose table class is 2 (RES_00BA_26C2 gives 0Ch, so [BP-2] = b + 5); DS:6817/6819 as given. */
static pr_u38r_cpu world(unsigned ip, unsigned key, unsigned b6817, unsigned b6819, unsigned seed)
{
    pr_u38r_cpu cpu = {0x1234u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, CS, 0, DS, 0x9999u, SS, 0x0202u};
    unsigned i;
    memset(ram, 0x5C, RAM);
    w8(CS, 0x291Fu, 0x9Au); w16(CS, 0x2920u, 0x26C2u); w16(CS, 0x2922u, 0x00BAu);
    w8(CS, 0x295Fu, 0x9Au); w16(CS, 0x2960u, 0x0048u); w16(CS, 0x2962u, 0x00B0u);
    w8(CS, 0x299Bu, 0x9Au); w16(CS, 0x299Cu, 0x0048u); w16(CS, 0x299Eu, 0x00B0u);
    w8(0x00B0u, 0x48u, 0xEAu); w16(0x00B0u, 0x49u, 0x0DE3u); w16(0x00B0u, 0x4Bu, DICE_CS);
    w8(DS, 0x681Bu, K); w8(DS, 0x31FAu + 16u * K, 2u); w8(DS, 0x6DBFu, 0u);
    w8(DS, 0x6817u, b6817); w8(DS, 0x6818u, 0x77u); w8(DS, 0x6819u, b6819);
    w16(DS, 0x4428u, seed & 0xFFFFu); w16(DS, 0x442Au, seed >> 16);
    w16(SS, SP0, 0x7000u); w16(SS, SP0 + 2u, 0x7001u);
    if (ip == 0x2915u)
        w16(SS, SP0 + 4u, 0xA500u | key);
    else
        for (i = 0; i < 5u; ++i) w16(SS, SP0 + 4u + 2u * i, 0x1111u * (i + 1u));
    cpu.ip = (uint16_t)ip;
    return cpu;
}

static void returned(const pr_u38r_cpu *cpu, unsigned pops)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == SP0 + 4u + pops && cpu->bp == 0x7777u && cpu->ds == DS);
}

int main(void)
{
    pr_u38r_cpu cpu;
    int callee;
    unsigned seed, cleared = 0u, kept = 0u;
    ram = malloc(RAM);
    CHECK(ram != NULL);

    /* +2915 skip: DS:6817 = 0 and DS:6819 bit 3 clear: no roll, OR AL,AL's flags (ZF, PF); [BP-2] = 40h + 5. */
    cpu = world(0x2915u, 0x40u, 0u, 0xF7u, 1u);
    CHECK(pr_u38_roll_gate_2915_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK && callee == PR_RES_26C2_OK);
    CHECK(r8(SS, SP0 - 4u) == 0x45u && r8(SS, SP0 - 3u) == 0x0Cu && r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0x77u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u && cpu.si == 0xFFFFu && cpu.dx == 0xFFFBu);   /* SI = 11 - 12, DX = 5 * SI */
    returned(&cpu, 2u);

    /* +2915 roll with [BP-2] = 64h + 5 = 69h: every 1d100 is at most 69h, so helper(0) clears 6817/6818. */
    for (seed = 1u; seed < 20u; ++seed) {
        cpu = world(0x2915u, 0x64u, 0x34u, 0u, seed * 0x9E3779B9u);
        CHECK(pr_u38_roll_gate_2915_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK && callee == 0);
        CHECK(r8(DS, 0x6817u) == 0u && r8(DS, 0x6818u) == 0u);
        returned(&cpu, 2u);
        /* [BP-2] = FBh + 5 = 0: every roll is above it (JA), so nothing is cleared and CMP's CF/ZF are clear. */
        cpu = world(0x2915u, 0xFBu, 0x34u, 0u, seed * 0x9E3779B9u);
        CHECK(pr_u38_roll_gate_2915_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK);
        CHECK(r8(DS, 0x6817u) == 0x34u && (cpu.flags & 0x0041u) == 0u);
        returned(&cpu, 2u);
    }

    /* Entry 89 (+2976): key 32h through +2915; the skip path keeps DS:6817 and returns with RETF 0Ah. */
    cpu = world(0x2976u, 0u, 0u, 0u, 3u);
    CHECK(pr_u38_entry89_2976_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK && r8(DS, 0x6817u) == 0u);
    CHECK(r8(SS, SP0 - 4u) == 0xA532u - 0xA500u);                          /* the pushed key word's low byte */
    returned(&cpu, 0x0Au);
    /* Entry 90 (+2984): key 0Fh, same shape. */
    cpu = world(0x2984u, 0u, 0u, 0u, 3u);
    CHECK(pr_u38_entry90_2984_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK && r8(SS, SP0 - 4u) == 0x0Fu);
    returned(&cpu, 0x0Au);

    /* Entry 91 (+2992): a roll at most 5Ah runs helper(35h) (clearing 6817 = 35h); above it, CMP AL,5Ah leaves CF/ZF
     * clear and nothing changes. Both outcomes occur over 64 seeds. */
    for (seed = 1u; seed <= 64u; ++seed) {
        cpu = world(0x2992u, 0u, 0x35u, 0u, seed * 0x01000193u);
        CHECK(pr_u38_entry91_2992_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_OK);
        if (r8(DS, 0x6817u) == 0u) {
            CHECK(r8(DS, 0x6818u) == 0u);
            ++cleared;
        } else {
            CHECK(r8(DS, 0x6817u) == 0x35u && (cpu.flags & 0x0041u) == 0u && (uint8_t)cpu.ax > 0x5Au);
            ++kept;
        }
        returned(&cpu, 0x0Au);
    }
    CHECK(cleared > 0u && kept > 0u);

    /* A zero SYSTEM-call cap stops the chain: CALLEE with its LIMIT. A broken stub stops with CODE. */
    cpu = world(0x2992u, 0u, 0x35u, 0u, 5u);
    CHECK(pr_u38_entry91_2992_cpu(ram, RAM, &cpu, 0u, &callee) == PR_U38R_CALLEE && callee == PR_U9E_ENTRY8_COMPOSE_LIMIT);
    cpu = world(0x2992u, 0u, 0x35u, 0u, 5u);
    w8(0x00B0u, 0x48u, 0xCDu);
    CHECK(pr_u38_entry91_2992_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U38R_CODE);

    /* Argument rejections. */
    cpu = world(0x2915u, 0u, 0u, 0u, 1u);
    CHECK(pr_u38_roll_gate_2915_cpu(NULL, RAM, &cpu, 256u, &callee) == PR_U38R_ARGUMENT && callee == -1);
    CHECK(pr_u38_entry89_2976_cpu(ram, RAM - 1u, &cpu, 256u, NULL) == PR_U38R_ARGUMENT);
    CHECK(pr_u38_entry91_2992_cpu(ram, RAM, NULL, 256u, NULL) == PR_U38R_ARGUMENT);
    CHECK(cpu.sp == SP0);

    free(ram);
    printf("PASS: u38 cluster 2, %u checks (skip and roll paths, entries 89/90/91, CALLEE, CODE, arguments)\n", checks);
    return 0;
}
