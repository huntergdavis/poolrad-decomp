#include "poolrad/recovered/overlay_u25_entry23_1fb1.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define CS 0xF000u
#define NODE_SEG 0x4000u
#define OUT_SEG 0x2000u
#define MIN_OFF 0x0010u
#define MAX_OFF 0x0020u

typedef struct {
    unsigned calls;
    unsigned abort_at;
    uint8_t has27[8], has2a[8];
    char log[256];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_1FB1_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_1FB1_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_1FB1_RAM_SIZE - 1u)] << 8));
}

static uint16_t node_off(unsigned i)
{
    return (uint16_t)(0x0200u * (i + 1u));
}

/* Synthetic BA:2199: AL=1 when the node at the pushed far pointer has the
 * requested key; the cursor cell is set to that node or zero. */
static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_1fb1_cpu *cpu, pr_u25_1fb1_call call)
{
    fixture *f = context;
    uint16_t cursor = get16(ram, cpu->ss, cpu->sp);
    uint16_t cursor_seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    uint16_t key = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    uint16_t off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    uint16_t seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    unsigned i = off / 0x0200u - 1u;
    int found;
    char note[4];
    assert(ram_size == PR_U25_1FB1_RAM_SIZE && call == PR_U25_1FB1_LOOKUP);
    assert(cursor == (uint16_t)(BP - 8u) && cursor_seg == cpu->ss);
    assert(seg == NODE_SEG && i < 8u);
    assert(cpu->ip == ((key & 0xFFu) == 0x27u ? 0x2004u : 0x2027u));
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    found = (key & 0xFFu) == 0x27u ? f->has27[i] : f->has2a[i];
    note[0] = (key & 0xFFu) == 0x27u ? 'D' : 'H';
    note[1] = (char)('0' + i);
    note[2] = ' ';
    note[3] = '\0';
    strcat(f->log, note);
    put16(ram, cpu->ss, cursor, found ? off : 0u);
    put16(ram, cpu->ss, (uint16_t)(cursor + 2u), found ? seg : 0u);
    cpu->ax = (uint16_t)(0x5A00u | (found ? 1u : 0u));
    cpu->flags = 0x0A53u;
    return 1;
}

static uint8_t *prepare(pr_u25_1fb1_cpu *cpu, const uint8_t *values, unsigned n)
{
    uint8_t *ram = calloc(PR_U25_1FB1_RAM_SIZE, 1u);
    unsigned i;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = CS; cpu->ip = 0x1FB1u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    put16(ram, SS, SP, 0x1F1Fu);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, MIN_OFF);
    put16(ram, SS, SP + 6u, OUT_SEG);
    put16(ram, SS, SP + 8u, MAX_OFF);
    put16(ram, SS, SP + 10u, OUT_SEG);
    put16(ram, DS, 0x5D96u, n ? node_off(0) : 0u);
    put16(ram, DS, 0x5D98u, n ? NODE_SEG : 0u);
    for (i = 0u; i < n; ++i) {
        ram[physical(NODE_SEG, (uint16_t)(node_off(i) + 0x11Cu))] = values[i];
        put16(ram, NODE_SEG, (uint16_t)(node_off(i) + 0x104u),
              i + 1u < n ? node_off(i + 1u) : 0u);
        put16(ram, NODE_SEG, (uint16_t)(node_off(i) + 0x106u),
              i + 1u < n ? NODE_SEG : 0u);
    }
    ram[physical(0u, 0x011Cu)] = 0x77u; /* what a null head would read */
    return ram;
}

static void returned(const pr_u25_1fb1_cpu *cpu)
{
    assert(cpu->sp == SP + 12u && cpu->bp == 0x7777u);
    assert(cpu->ip == 0x1F1Fu && cpu->cs == 0xD000u);
    /* OR AX,[BP-2] on the null link is the last flag writer: ZF PF set. */
    assert((cpu->flags & 0x08D5u) == 0x0044u && cpu->ax == 0u);
}

static void test_reference(void)
{
    uint32_t seed = 0x1FB1u;
    unsigned round;
    for (round = 0u; round < 400u; ++round) {
        uint8_t values[8], lo, hi;
        unsigned n = 1u + round % 8u, i;
        pr_u25_1fb1_cpu cpu;
        fixture f;
        uint8_t *ram;
        memset(&f, 0, sizeof f);
        for (i = 0u; i < n; ++i) {
            seed = seed * 1103515245u + 12345u;
            values[i] = (uint8_t)(seed >> 16);
            f.has27[i] = (uint8_t)((seed >> 8) & 1u);
            f.has2a[i] = (uint8_t)((seed >> 9) & 1u);
        }
        lo = hi = values[0];
        for (i = 0u; i < n; ++i) {
            uint8_t v = values[i];
            if (f.has27[i]) v = (uint8_t)(v << 1);
            else if (f.has2a[i]) v = (uint8_t)(v >> 1);
            if (v > hi) hi = v;
            if (v < lo) lo = v;
        }
        ram = prepare(&cpu, values, n);
        assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, &cpu, external, &f)
               == PR_U25_1FB1_OK);
        assert(ram[physical(OUT_SEG, MIN_OFF)] == lo);
        assert(ram[physical(OUT_SEG, MAX_OFF)] == hi);
        returned(&cpu);
        free(ram);
    }
}

static void test_paths(void)
{
    static const uint8_t values[3] = {0x90u, 0x10u, 0x40u};
    pr_u25_1fb1_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, values, 3u);
    memset(&f, 0, sizeof f);
    f.has27[0] = 1u;               /* 0x90 doubles to 0x20 (byte) */
    f.has2a[1] = 1u;               /* 0x10 halves to 0x08 */
    f.has27[2] = 0u;               /* 0x40 unchanged */
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1FB1_OK);
    assert(strcmp(f.log, "D0 D1 H1 D2 H2 ") == 0);
    assert(ram[physical(OUT_SEG, MIN_OFF)] == 0x08u);
    assert(ram[physical(OUT_SEG, MAX_OFF)] == 0x90u);
    /* [bp-9] keeps the last node's value; the cursor holds its last write. */
    assert(ram[physical(SS, (uint16_t)(BP - 9u))] == 0x40u);
    assert(get16(ram, SS, (uint16_t)(BP - 8u)) == 0u);
    returned(&cpu);
    free(ram);

    /* Null head: both outputs still take the byte at 0000:011C. */
    ram = prepare(&cpu, values, 0u);
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1FB1_OK);
    assert(f.calls == 0u);
    assert(ram[physical(OUT_SEG, MIN_OFF)] == 0x77u);
    assert(ram[physical(OUT_SEG, MAX_OFF)] == 0x77u);
    returned(&cpu);
    free(ram);
}

static void test_abort_and_arguments(void)
{
    static const uint8_t values[2] = {5u, 6u};
    pr_u25_1fb1_cpu cpu;
    fixture f;
    uint8_t *ram;
    memset(&f, 0, sizeof f);
    f.abort_at = 2u;
    ram = prepare(&cpu, values, 2u);
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1FB1_CALL_ABORT);
    assert(cpu.ip == 0x2027u && strcmp(f.log, "D0 ") == 0);
    free(ram);
    ram = prepare(&cpu, values, 2u);
    assert(pr_u25_entry23_1fb1(NULL, PR_U25_1FB1_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1FB1_ARGUMENT);
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_1FB1_ARGUMENT);
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, NULL, external, &f)
           == PR_U25_1FB1_ARGUMENT);
    assert(pr_u25_entry23_1fb1(ram, PR_U25_1FB1_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_1FB1_ARGUMENT);
    assert(cpu.sp == SP && ram[physical(OUT_SEG, MIN_OFF)] == 0u);
    free(ram);
}

int main(void)
{
    test_reference();
    test_paths();
    test_abort_and_arguments();
    return 0;
}
