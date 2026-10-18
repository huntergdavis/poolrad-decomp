/* Focused synthetic test for raw-CPU U3B0 +0B03 composing the strict U3B0 +0000 (docs/re/U3B0_STEP_SCAN_0B03.md).
 * No game bytes: the near callee is the merged C model, so the test needs only synthetic parent frames, node lists
 * and DS:6B29. An independent reference of both walks predicts the index, count, result and every byte outside the
 * stack window. */
#include "poolrad/recovered/overlay_u3b0_step_scan_0b03_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3B0_0B03_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3b0 0b03 line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, LINK = SP0 + 0x80, NODE = 0x5000, XSEG = 0x6000, XOFF = 0x0100,
       RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777, CAP = 1000 };

typedef struct {
    unsigned nodes, direction;
    int limit, index, w56, floor;
    uint8_t flags[16];
} walk;

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }
static unsigned node_off(unsigned k) { return 0x0100u + 0x40u * k; }

static pr_u3b0_0b03_cpu world(const walk *w, unsigned base)
{
    pr_u3b0_0b03_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x0B03u),
                            DS, 0x4444u, SS, 0x0202u};
    memset(ram, 0x5C, RAM);
    for (unsigned k = 0u; k < w->nodes; ++k) {
        unsigned next = k + 1u < w->nodes ? node_off(k + 1u) : 0u;
        w8(NODE, node_off(k) + 0x29u, w->flags[k]);
        w16(NODE, node_off(k) + 0x2Au, next); w16(NODE, node_off(k) + 0x2Cu, next ? NODE : 0u);
    }
    w16(SS, LINK - 0x59u, (unsigned)w->limit & 0xFFFFu); w16(SS, LINK - 0x56u, (unsigned)w->w56 & 0xFFFFu);
    w16(SS, LINK + 0x0Au, XOFF); w16(SS, LINK + 0x0Cu, XSEG);
    w16(SS, LINK + 0x14u, w->nodes ? node_off(0) : 0u); w16(SS, LINK + 0x16u, w->nodes ? NODE : 0u);
    w16(XSEG, XOFF, (unsigned)w->index & 0xFFFFu);
    w16(DS, 0x6B29u, (unsigned)w->floor & 0xFFFFu);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, LINK); w16(SS, SP0 + 6u, w->direction);
    return cpu;
}

/* +0000's walk: the node `target` steps from the head, or nil; returns the flag byte the caller tests. */
static unsigned flag_at(const walk *w, unsigned target, unsigned *node)
{
    unsigned count = 0u, k = 0u;
    int live = w->nodes != 0u;
    while (live && count != target) { ++k; live = k < w->nodes; count = (count + 1u) & 0xFFFFu; }
    *node = live && count == target ? node_off(k) : 0u;
    return live && count == target ? w->flags[k] : r8(0u, 0x29u);
}

static void run_case(const walk *w)
{
    pr_u3b0_0b03_cpu cpu = world(w, 0u);
    int count = 0, index = w->index, limit = w->limit, top_exit = 0;
    unsigned node = 0u, ax;
    int callee = 99;
    memcpy(want, ram, RAM);
    for (;;) {
        if (!((short)count < (short)limit)) { top_exit = 1; break; }
        if (flag_at(w, (unsigned)index & 0xFFFFu, &node) == 0u) break;
        ++count;
        if (w->direction) {
            index = (short)(index + 1);
            if ((short)(w->floor + limit - 1) < (short)index) index = w->floor;
            if ((short)(w->w56 - 1) < (short)index) index = w->floor;
        } else {
            index = (short)(index - 1);
            if ((short)index < (short)w->floor) index = (short)(w->floor + limit - 1);
            if ((short)(w->w56 - 1) < (short)index) index = (short)(w->w56 - 1);
        }
    }
    ax = top_exit ? (unsigned)count & 0xFFFFu : node;
    want[at(XSEG, XOFF)] = (uint8_t)index; want[at(XSEG, XOFF + 1u)] = (uint8_t)((unsigned)index >> 8);
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U3B0_0B03_OK);
    CHECK(callee == ((top_exit && count == 0) ? -1 : 0));
    CHECK(r16(XSEG, XOFF) == ((unsigned)index & 0xFFFFu) && r16(SS, BP1 - 2u) == ((unsigned)count & 0xFFFFu));
    CHECK(cpu.ax == ax && cpu.bp == BP0 && cpu.sp == SP0 + 8u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
    for (unsigned off = BP1 - 0x40u; off < SP0 + 8u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
}

static void walks(void)
{
    unsigned seed = 0x0B03u;
    walk w;
    for (unsigned n = 0u; n < 1500u; ++n) {
        memset(&w, 0, sizeof w);
        seed = seed * 1103515245u + 12345u;
        w.nodes = (seed >> 16) % 13u;
        w.direction = (seed >> 8) & 1u ? 0u : ((seed >> 9) & 1u ? 1u : 0xFFu);
        seed = seed * 1103515245u + 12345u;
        w.limit = (int)((seed >> 16) % 14u) - 2;
        w.index = (int)((seed >> 20) % 17u) - 3;
        seed = seed * 1103515245u + 12345u;
        w.w56 = (int)((seed >> 16) % 16u) - 2;
        w.floor = (int)((seed >> 20) % 8u) - 2;
        for (unsigned k = 0u; k < 16u; ++k) { seed = seed * 1103515245u + 12345u; w.flags[k] = ((seed >> 16) % 5u) ? 1u : 0u; }
        run_case(&w);
    }
}

static void stops(void)
{
    walk w;
    pr_u3b0_0b03_cpu cpu;
    int callee = 99;
    memset(&w, 0, sizeof w);
    w.nodes = 12u; w.direction = 1u; w.limit = 8; w.index = 0; w.w56 = 40; w.floor = 0;
    memset(w.flags, 1, sizeof w.flags);
    cpu = world(&w, 0u);                                                   /* a cap of 2: the third loop-back stops */
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM, &cpu, 2u, &callee) == PR_U3B0_0B03_LIMIT && callee == 0);
    CHECK(cpu.ip == 0x0B17u && r16(SS, BP1 - 2u) == 3u && r16(XSEG, XOFF) == 3u);
    cpu = world(&w, 0x1000u);                                              /* entered at another offset: +0000 is not at IP 0 */
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U3B0_0B03_CALLEE && callee == 1);
    CHECK(cpu.ip == 0x1000u && cpu.cs == CS && r16(SS, cpu.sp) == 0x1B3Cu && r16(SS, cpu.sp + 2u) == CS);
}

static void arguments(void)
{
    walk w;
    pr_u3b0_0b03_cpu cpu, before;
    int callee = 99;
    memset(&w, 0, sizeof w);
    w.nodes = 3u; w.limit = 2; w.direction = 1u;
    cpu = world(&w, 0u);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u3b0_step_scan_0b03_cpu(NULL, RAM, &cpu, CAP, &callee) == PR_U3B0_0B03_ARGUMENT && callee == -1);
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM, NULL, CAP, NULL) == PR_U3B0_0B03_ARGUMENT);
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM - 1u, &cpu, CAP, NULL) == PR_U3B0_0B03_ARGUMENT);
    CHECK(pr_u3b0_step_scan_0b03_cpu(ram, RAM + 1u, &cpu, CAP, NULL) == PR_U3B0_0B03_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    walks();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u3b0 +0b03: %u checks passed\n", checks);
    return 0;
}
