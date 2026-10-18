/* Focused synthetic test for the composed (strict full-state) entry of GAME.OVR unit 0x25:+1FB1, public entry 23
 * (docs/re/U25_ENTRY23_1FB1.md), with the raw-CPU RES_00BA_2199 model. No game bytes: only the two far-call sites. */
#include "poolrad/recovered/overlay_u25_entry23_1fb1.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U25_1FB1_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u25 +1FB1 composed line %d: %s\n", __LINE__, #x);   \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP0 = SP0 - 2, OUT_SEG = 0x6800, MIN_OFF = 0x10, MAX_OFF = 0x20 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off)
{
    return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u);
}

static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static unsigned node_seg(unsigned i) { return 0x6000u + 0x20u * i; }

/* Node i at node_seg(i):0010 with value byte +11Ch, link +104h/+106h and a sidecar list of `n` keys. */
static void node(unsigned i, unsigned value, int next, const uint8_t *keys, unsigned n, int cyclic_sidecars)
{
    unsigned seg = node_seg(i), j;
    w8(seg, 0x0010u + 0x11Cu, value);
    w16(seg, 0x0010u + 0x104u, next >= 0 ? 0x0010u : 0u);
    w16(seg, 0x0010u + 0x106u, next >= 0 ? node_seg((unsigned)next) : 0u);
    for (j = 0; j < n; ++j) {
        unsigned off = 0x0040u + 0x10u * j;
        w8(0x7000u + 0x40u * i, off, keys[j]);
        w16(0x7000u + 0x40u * i, off + 5u, j + 1u < n ? off + 0x10u : (cyclic_sidecars ? 0x0040u : 0u));
        w16(0x7000u + 0x40u * i, off + 7u, j + 1u < n || cyclic_sidecars ? 0x7000u + 0x40u * i : 0u);
    }
    w16(seg, 0x0010u + 0x7Fu, n ? 0x0040u : 0u);
    w16(seg, 0x0010u + 0x81u, n ? 0x7000u + 0x40u * i : 0u);
}

static pr_u25_1fb1_cpu world(int head)
{
    pr_u25_1fb1_cpu cpu = {0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, CS, 0x1FB1u, DS, 0x9999u, SS, 0x0202u};
    static const uint16_t sites[2] = {0x2004u, 0x2027u};
    unsigned n;
    memset(ram, 0xA5, RAM);
    for (n = 0; n < 2u; ++n) {
        w8(CS, sites[n], 0x9Au);
        w16(CS, sites[n] + 1u, 0x2199u);
        w16(CS, sites[n] + 3u, 0x00BAu);
    }
    w16(DS, 0x5D96u, head >= 0 ? 0x0010u : 0u);
    w16(DS, 0x5D98u, head >= 0 ? node_seg((unsigned)head) : 0u);
    w8(OUT_SEG, MIN_OFF, 0xEEu);
    w8(OUT_SEG, MAX_OFF, 0xEEu);
    w16(SS, SP0, 0x7000u);
    w16(SS, SP0 + 2u, 0x7001u);
    w16(SS, SP0 + 4u, MIN_OFF);
    w16(SS, SP0 + 6u, OUT_SEG);
    w16(SS, SP0 + 8u, MAX_OFF);
    w16(SS, SP0 + 10u, OUT_SEG);
    return cpu;
}

static void returned(const pr_u25_1fb1_cpu *cpu)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == SP0 + 12u && cpu->bp == 0x7777u);
    CHECK(cpu->ax == 0u && (cpu->flags & 0x08D5u) == 0x0044u);      /* the final OR AX,[BP-2] finds the null link */
    CHECK(cpu->ds == DS && cpu->ss == SS && cpu->bx == 0x2222u && cpu->cx == 0x3333u);
}

int main(void)
{
    static const uint8_t none[1] = {0x11u}, half[2] = {0x55u, 0x2Au}, twice[1] = {0x27u}, both[2] = {0x2Au, 0x27u};
    pr_u25_1fb1_cpu cpu;
    int callee;
    ram = malloc(RAM);
    CHECK(ram != NULL);

    /* 10, 200 (2Ah: halved to 100), 50 (27h: doubled to 100): min 10, max 100 from the head seed of 10. */
    cpu = world(0);
    node(0, 10u, 1, none, 1u, 0);
    node(1, 200u, 2, half, 2u, 0);
    node(2, 50u, -1, twice, 1u, 0);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 3u, 254u, &callee) == PR_U25_1FB1_OK);
    CHECK(r8(OUT_SEG, MIN_OFF) == 10u && r8(OUT_SEG, MAX_OFF) == 100u && callee == PR_RES_2199_OK);
    CHECK(r8(SS, BP0 - 9u) == 100u);                                     /* [BP-9]: the last node's value */
    CHECK(r16(SS, BP0 - 8u) == 0x0040u && r16(SS, BP0 - 6u) == 0x7080u); /* cursor: node 2's 27h sidecar */
    returned(&cpu);

    /* Doubling wraps to a byte: 90h with 27h gives 20h, below the head seed 90h, so min = 20h. Key 27h is tried
     * first, so a node holding both keys doubles. */
    cpu = world(0);
    node(0, 0x90u, 1, twice, 1u, 0);
    node(1, 0x40u, -1, both, 2u, 0);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 2u, 254u, &callee) == PR_U25_1FB1_OK);
    CHECK(r8(OUT_SEG, MIN_OFF) == 0x20u && r8(OUT_SEG, MAX_OFF) == 0x90u);
    CHECK(r8(SS, BP0 - 9u) == 0x80u);
    returned(&cpu);

    /* Null head: both outputs take the byte at 0000:011C and no lookup runs. */
    cpu = world(-1);
    w8(0u, 0x011Cu, 0x5Bu);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 0u, 254u, &callee) == PR_U25_1FB1_OK);
    CHECK(r8(OUT_SEG, MIN_OFF) == 0x5Bu && r8(OUT_SEG, MAX_OFF) == 0x5Bu && callee == -1);
    returned(&cpu);

    /* A two-node cycle never returns in DOS: five loop-backs stop at +1FE0 with SP = BP - 0Ah. */
    cpu = world(0);
    node(0, 7u, 1, none, 1u, 0);
    node(1, 9u, 0, none, 1u, 0);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 5u, 254u, &callee) == PR_U25_1FB1_LIMIT);
    CHECK(cpu.ip == 0x1FE0u && cpu.cs == CS && cpu.bp == BP0 && cpu.sp == BP0 - 0x0Au);
    CHECK(r8(OUT_SEG, MIN_OFF) == 7u && r8(OUT_SEG, MAX_OFF) == 9u);

    /* A cyclic sidecar list without either key: the first lookup's model stops at its cap (CALLEE, LIMIT). */
    cpu = world(0);
    node(0, 7u, -1, half, 1u, 1);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 5u, 30u, &callee) == PR_U25_1FB1_CALLEE);
    CHECK(callee == PR_RES_2199_LIMIT && cpu.ip == 0x21B9u && cpu.cs == 0x00BAu);

    /* A reached call site that is not LCALL xxxx:2199 stops with CODE before it, the five words pushed. */
    cpu = world(0);
    node(0, 7u, -1, none, 1u, 0);
    w16(CS, 0x2005u, 0x2198u);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, &cpu, 5u, 254u, &callee) == PR_U25_1FB1_CODE);
    CHECK(callee == -1 && cpu.ip == 0x2004u && cpu.sp == BP0 - 0x0Au - 0x0Au);

    /* Argument rejections: the composed entry needs exactly 1 MiB. */
    cpu = world(0);
    CHECK(pr_u25_entry23_1fb1_composed(NULL, RAM, &cpu, 5u, 254u, &callee) == PR_U25_1FB1_ARGUMENT && callee == -1);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM + 1u, &cpu, 5u, 254u, NULL) == PR_U25_1FB1_ARGUMENT);
    CHECK(pr_u25_entry23_1fb1_composed(ram, RAM, NULL, 5u, 254u, NULL) == PR_U25_1FB1_ARGUMENT);
    CHECK(cpu.sp == SP0 && cpu.ip == 0x1FB1u);

    free(ram);
    printf("PASS: u25 +1FB1 composed, %u checks (walk, wrap, key order, null head, LIMIT, CALLEE, CODE, arguments)\n", checks);
    return 0;
}
