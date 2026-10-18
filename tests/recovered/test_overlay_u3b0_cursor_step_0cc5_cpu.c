/* Focused synthetic test for raw-CPU U3B0 +0CC5 composing the strict U3B0 +0B03 (docs/re/U3B0_CURSOR_STEP_0CC5.md).
 * No game bytes: the near callee and its callee are merged C models, so the test needs only synthetic parent
 * frames, node lists and DS:6B29. An independent reference of the wraparound step and the +0B03 walk predicts
 * the index, the pushed AX, the result and every byte outside the stack window. */
#include "poolrad/recovered/overlay_u3b0_cursor_step_0cc5_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3B0_0CC5_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3b0 0cc5 line %d: %s\n", __LINE__, #x);             \
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

static pr_u3b0_0cc5_cpu world(const walk *w, unsigned base)
{
    pr_u3b0_0cc5_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x0CC5u),
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
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, LINK); w16(SS, SP0 + 6u, w->direction | 0xA500u);
    return cpu;
}

/* +0000's walk: the node `target` steps from the head, or nil; returns the flag byte +0B03 tests. */
static unsigned flag_at(const walk *w, unsigned target, unsigned *node)
{
    unsigned count = 0u, k = 0u;
    int live = w->nodes != 0u;
    while (live && count != target) { ++k; live = k < w->nodes; count = (count + 1u) & 0xFFFFu; }
    *node = live && count == target ? node_off(k) : 0u;
    return live && count == target ? w->flags[k] : r8(0u, 0x29u);
}

/* The wraparound step of +0CC5 alone; returns AX as the call pushes it before MOV AL,d. */
static unsigned step(const walk *w, int *index)
{
    int g = w->floor, top = (short)(w->floor + w->limit - 1), last = (short)(w->w56 - 1);
    if (w->direction & 0xFFu) {
        *index = (short)(*index + 1);
        if (top < *index) *index = g;
        if (last < *index) { *index = g; return (unsigned)g & 0xFFFFu; }
        return (unsigned)last & 0xFFFFu;
    }
    *index = (short)(*index - 1);
    if (*index < g) *index = top;
    if (last < *index) *index = last;
    return (unsigned)last & 0xFFFFu;
}

static void run_case(const walk *w)
{
    pr_u3b0_0cc5_cpu cpu = world(w, 0u);
    int count = 0, index = w->index, limit = w->limit, top_exit = 0;
    unsigned node = 0u, ax, pushed;
    int callee = 99, leaf = 99;
    memcpy(want, ram, RAM);
    pushed = (step(w, &index) & 0xFF00u) | (w->direction & 0xFFu);
    for (;;) {
        if (!((short)count < (short)limit)) { top_exit = 1; break; }
        if (flag_at(w, (unsigned)index & 0xFFFFu, &node) == 0u) break;
        ++count;
        if (w->direction & 0xFFu) {
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
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, &cpu, CAP, &callee, &leaf) == PR_U3B0_0CC5_OK);
    CHECK(callee == PR_U3B0_0CC5_OK && leaf == ((top_exit && count == 0) ? -1 : 0));
    CHECK(r16(XSEG, XOFF) == ((unsigned)index & 0xFFFFu));
    CHECK(r16(SS, BP1) == BP0 && r16(SS, BP1 - 10u) == pushed && r16(SS, BP1 - 12u) == LINK && r16(SS, BP1 - 14u) == CS);
    CHECK(r16(SS, BP1 - 16u) == 0x0D82u && r16(SS, BP1 - 18u) == BP1 && r16(SS, BP1 - 20u) == ((unsigned)count & 0xFFFFu));
    CHECK(cpu.ax == ax && cpu.bp == BP0 && cpu.sp == SP0 + 8u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
    for (unsigned off = BP1 - 0x40u; off < SP0 + 8u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
}

static void walks(void)
{
    unsigned seed = 0x0CC5u;
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

/* Each wraparound and clamp edge, with a walk that stops at once (a zero flag) so the stepped index is visible. */
static void clamps(void)
{
    static const struct { unsigned direction; int index, limit, w56, floor, after; unsigned ah_from; } edge[] = {
        {1u, 6, 5, 40, 2, 2, 39u},      /* 2 + 5 - 1 = 6 < 7: wraps to the floor; AX = B - 1 */
        {1u, 6, 9, 7, 0, 0, 0u},        /* 7 <= 0 + 9 - 1, but B - 1 = 6 < 7: second clamp to the floor; AX = G */
        {1u, 2, 9, 40, 0, 3, 39u},      /* inside both bounds */
        {0u, 2, 4, 40, 2, 5, 39u},      /* 1 < 2: wraps to 2 + 4 - 1 = 5 */
        {0u, 9, 20, 6, 0, 5, 5u},       /* 8 > B - 1 = 5: clamps to 5 */
        {0u, 0x7FFF, 3, 0x7FFF, -0x8000, 0x7FFE, 0x7FFEu}, /* signed edges */
        {1u, 0x7FFF, 3, -0x7FFF, -0x8000, -0x8000, 0x8000u},
    };
    for (unsigned i = 0u; i < sizeof edge / sizeof edge[0]; ++i) {
        walk w;
        pr_u3b0_0cc5_cpu cpu;
        int callee = 99, leaf = 99;
        memset(&w, 0, sizeof w);
        w.nodes = 12u; w.direction = edge[i].direction; w.limit = edge[i].limit; w.index = edge[i].index;
        w.w56 = edge[i].w56; w.floor = edge[i].floor;
        cpu = world(&w, 0u);
        w16(0u, 0x28u, 0u);                                                /* nil nodes read a zero flag from 0000:0029 */
        for (unsigned k = 0u; k < 12u; ++k) w8(NODE, node_off(k) + 0x29u, 0u);
        CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, &cpu, CAP, &callee, &leaf) == PR_U3B0_0CC5_OK);
        CHECK(r16(XSEG, XOFF) == ((unsigned)edge[i].after & 0xFFFFu));
        CHECK(r16(SS, BP1 - 10u) == ((edge[i].ah_from & 0xFF00u) | edge[i].direction));
        CHECK(callee == 0 && leaf == ((short)edge[i].limit > 0 ? 0 : -1));
    }
}

static void stops(void)
{
    walk w;
    pr_u3b0_0cc5_cpu cpu;
    int callee = 99, leaf = 99;
    memset(&w, 0, sizeof w);
    w.nodes = 12u; w.direction = 1u; w.limit = 8; w.index = 0; w.w56 = 40; w.floor = 0;
    memset(w.flags, 1, sizeof w.flags);
    cpu = world(&w, 0u);                                                   /* +0B03 with a cap of 2: its third loop-back stops */
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, &cpu, 2u, &callee, &leaf) == PR_U3B0_0CC5_CALLEE && callee == 4 && leaf == 0);
    CHECK(cpu.ip == 0x0B17u && cpu.cs == CS && r16(SS, BP1 - 20u) == 3u && r16(XSEG, XOFF) == 4u);
    cpu = world(&w, 0x1000u);                                              /* entered elsewhere: +0000 is not reached at IP 0 */
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, &cpu, CAP, &callee, &leaf) == PR_U3B0_0CC5_CALLEE && callee == 2 && leaf == 1);
    CHECK(cpu.ip == 0x1000u && cpu.cs == CS && r16(SS, cpu.sp) == 0x1B3Cu && r16(SS, BP1 - 16u) == 0x1D82u);
    w.limit = 3; w.flags[0] = 0u;
    cpu = world(&w, 0u);                                                   /* X^ is +0B03's return IP: the walk rewrites it */
    w16(SS, LINK + 0x0Au, BP1 - 16u); w16(SS, LINK + 0x0Cu, SS);
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, &cpu, CAP, &callee, &leaf) == PR_U3B0_0CC5_TRANSFER && callee == 0 && leaf == 0);
    CHECK(cpu.ip == 0u && cpu.cs == CS && cpu.sp == BP1 - 8u && cpu.bp == BP1);
}

static void arguments(void)
{
    walk w;
    pr_u3b0_0cc5_cpu cpu, before;
    int callee = 99, leaf = 99;
    memset(&w, 0, sizeof w);
    w.nodes = 3u; w.limit = 2; w.direction = 1u;
    cpu = world(&w, 0u);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(NULL, RAM, &cpu, CAP, &callee, &leaf) == PR_U3B0_0CC5_ARGUMENT && callee == -1 && leaf == -1);
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM, NULL, CAP, NULL, NULL) == PR_U3B0_0CC5_ARGUMENT);
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM - 1u, &cpu, CAP, NULL, NULL) == PR_U3B0_0CC5_ARGUMENT);
    CHECK(pr_u3b0_cursor_step_0cc5_cpu(ram, RAM + 1u, &cpu, CAP, NULL, NULL) == PR_U3B0_0CC5_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    walks();
    clamps();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u3b0 +0cc5: %u checks passed\n", checks);
    return 0;
}
