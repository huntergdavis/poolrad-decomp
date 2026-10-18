/* Focused synthetic test for the raw-CPU RES_00BA_2199 model (docs/re/RESIDENT_SIDECAR_LOOKUP_2199_CPU.md),
 * cross-checked against the credited callback model pr_rec_res_00ba_2199. */
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "poolrad/recovered/resident_sidecar_lookup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_res_2199_cpu) == 28, "all fourteen CPU words");

#define RAM PR_RES_2199_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #x); \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SS = 0x2000, SP0 = 0x8000, CODE = 0x08BA, ENTRY = 0x2199, RET_IP = 0x0987, RET_CS = 0x4321 };

static uint8_t ram[RAM], other[RAM];
static unsigned checks;
static uint32_t seed = 0x2199u;

static unsigned rnd(unsigned bound)
{
    seed = seed * 1103515245u + 12345u;
    return (seed >> 8) % bound;
}

static size_t at(unsigned segment, unsigned offset)
{
    return (((size_t)segment << 4) + (offset & 0xFFFFu)) & (RAM - 1u);
}

static void put16(uint8_t *memory, unsigned segment, unsigned offset, unsigned value)
{
    size_t where = at(segment, offset);
    memory[where] = (uint8_t)value;
    memory[(where + 1u) & (RAM - 1u)] = (uint8_t)(value >> 8);
}

static unsigned get16(const uint8_t *memory, unsigned segment, unsigned offset)
{
    size_t where = at(segment, offset);
    return memory[where] | (unsigned)memory[(where + 1u) & (RAM - 1u)] << 8;
}

static uint16_t flags_or_zero(unsigned flags)
{
    return (uint16_t)((flags & ~0x08D5u) | 0x0044u);               /* ZF, PF; CF OF AF SF clear */
}

static uint16_t flags_cmp_one(unsigned flags)
{
    return (uint16_t)(flags & ~0x08D5u);                           /* 1 - 0: every arithmetic flag clear */
}

/* Synthetic world: node i at (0x3000 + 0x100 * i):(0x0040 + 9 * i) with key keys[i]; character X at 4800:0100;
 * cursor cell C at cseg:coff; arguments pushed for a far call at SS:SP0. */
static void node(unsigned i, uint8_t key, unsigned next_seg, unsigned next_off)
{
    unsigned seg = 0x3000u + 0x100u * i, off = 0x0040u + 9u * i;
    ram[at(seg, off)] = key;
    put16(ram, seg, off + 5u, next_off);
    put16(ram, seg, off + 7u, next_seg);
}

static pr_res_2199_cpu frame(unsigned key_word, unsigned cseg, unsigned coff, unsigned flags)
{
    pr_res_2199_cpu cpu = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, SP0, CODE, ENTRY, 0x1000, 0x9999, SS, 0};
    put16(ram, SS, SP0, RET_IP);
    put16(ram, SS, SP0 + 2u, RET_CS);
    put16(ram, SS, SP0 + 4u, coff);
    put16(ram, SS, SP0 + 6u, cseg);
    put16(ram, SS, SP0 + 8u, key_word);
    put16(ram, SS, SP0 + 10u, 0x0100u);
    put16(ram, SS, SP0 + 12u, 0x4800u);
    cpu.flags = (uint16_t)flags;
    return cpu;
}

static void background(void)
{
    memset(ram, 0xA5, RAM);
}

static void check_return(const pr_res_2199_cpu *cpu)
{
    CHECK(cpu->sp == SP0 + 14u && cpu->ip == RET_IP && cpu->cs == RET_CS);
    CHECK(cpu->bx == 0x2222 && cpu->cx == 0x3333 && cpu->si == 0x5555 && cpu->ds == 0x1000 && cpu->ss == SS);
    CHECK(get16(ram, SS, SP0 - 2u) == 0x7777);
}

/* RAM-backed callbacks for the credited callback model, reading words as the raw model does. */
static int cb_read_word(void *context, uint16_t segment, uint16_t offset, uint16_t *value)
{
    *value = (uint16_t)get16((const uint8_t *)context, segment, offset);
    return 1;
}

static int cb_write_word(void *context, uint16_t segment, uint16_t offset, uint16_t value)
{
    put16((uint8_t *)context, segment, offset, value);
    return 1;
}

static int cb_read_byte(void *context, uint16_t segment, uint16_t offset, uint8_t *value)
{
    *value = ((const uint8_t *)context)[at(segment, offset)];
    return 1;
}

int main(void)
{
    pr_res_2199_cpu cpu;

    /* Empty list: head 0:0. AL = 0, AH = 0 (the OR), DX = head segment 0, ES:DI = the cursor pointer, OR flags. */
    background();
    put16(ram, 0x4800u, 0x017Fu, 0u); put16(ram, 0x4800u, 0x0181u, 0u);
    cpu = frame(0x5511u, 0x4C00u, 0x0020u, 0x0ED7u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 0u) == PR_RES_2199_OK);
    CHECK(cpu.ax == 0x0000 && cpu.dx == 0x0000 && cpu.es == 0x4C00 && cpu.di == 0x0020);
    CHECK(cpu.flags == flags_or_zero(0x0ED7u) && cpu.bp == 0x7777);
    CHECK(get16(ram, 0x4C00u, 0x0020u) == 0u && get16(ram, 0x4C00u, 0x0022u) == 0u);
    CHECK(ram[at(SS, SP0 - 4u)] == 0u && ram[at(SS, SP0 - 3u)] == 0u);
    check_return(&cpu);

    /* Found at the third of four nodes; the key's high byte is ignored. Two misses and the match each take one
     * loop-back, so a cap of 3 is exactly enough. AH = high byte of (offset | segment) of the match, DX = its segment (last link load). */
    background();
    node(0, 0x10, 0x3100u, 0x0049u); node(1, 0x20, 0x3200u, 0x0052u); node(2, 0x30, 0x3300u, 0x005Bu); node(3, 0x40, 0u, 0u);
    put16(ram, 0x4800u, 0x017Fu, 0x0040u); put16(ram, 0x4800u, 0x0181u, 0x3000u);
    cpu = frame(0xFF30u, 0x4C00u, 0x0020u, 0x0202u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 3u) == PR_RES_2199_OK);
    CHECK(cpu.ax == (((0x0052u | 0x3200u) & 0xFF00u) | 1u) && cpu.dx == 0x3200 && cpu.es == 0x4C00 && cpu.di == 0x0020);
    CHECK(cpu.flags == flags_cmp_one(0x0202u) && cpu.bp == 0x7777);
    CHECK(get16(ram, 0x4C00u, 0x0020u) == 0x0052u && get16(ram, 0x4C00u, 0x0022u) == 0x3200u);
    CHECK(ram[at(SS, SP0 - 4u)] == 1u && ram[at(SS, SP0 - 3u)] == 1u);
    check_return(&cpu);

    /* Same world, one loop-back too few: LIMIT at the loop head before the exit pass, after the match. */
    background();
    node(0, 0x10, 0x3100u, 0x0049u); node(1, 0x20, 0x3200u, 0x0052u); node(2, 0x30, 0x3300u, 0x005Bu); node(3, 0x40, 0u, 0u);
    put16(ram, 0x4800u, 0x017Fu, 0x0040u); put16(ram, 0x4800u, 0x0181u, 0x3000u);
    cpu = frame(0x0030u, 0x4C00u, 0x0020u, 0x0202u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 2u) == PR_RES_2199_LIMIT);
    CHECK(cpu.ip == ENTRY + 0x20u && cpu.cs == CODE && cpu.bp == SP0 - 2u && cpu.sp == SP0 - 4u);
    CHECK(get16(ram, 0x4C00u, 0x0020u) == 0x0052u && ram[at(SS, SP0 - 4u)] == 1u);

    /* Key absent: walk off the end. AL = 0, DX = the last link's segment 0, OR flags, cursor 0:0. */
    background();
    node(0, 0x10, 0x3100u, 0x0049u); node(1, 0x20, 0u, 0u);
    put16(ram, 0x4800u, 0x017Fu, 0x0040u); put16(ram, 0x4800u, 0x0181u, 0x3000u);
    cpu = frame(0x0099u, 0x4C00u, 0x0020u, 0x0002u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 254u) == PR_RES_2199_OK);
    CHECK(cpu.ax == 0x0000 && cpu.dx == 0x0000 && cpu.flags == flags_or_zero(0x0002u));
    CHECK(get16(ram, 0x4C00u, 0x0020u) == 0u && get16(ram, 0x4C00u, 0x0022u) == 0u);
    check_return(&cpu);

    /* A two-node cycle without the key never ends in DOS: the cap stops it after 1000 loop-backs. */
    background();
    node(0, 0x10, 0x3100u, 0x0049u); node(1, 0x20, 0x3000u, 0x0040u);
    put16(ram, 0x4800u, 0x017Fu, 0x0040u); put16(ram, 0x4800u, 0x0181u, 0x3000u);
    cpu = frame(0x0077u, 0x4C00u, 0x0020u, 0x0002u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 1000u) == PR_RES_2199_LIMIT);
    CHECK(cpu.ip == ENTRY + 0x20u && cpu.sp == SP0 - 4u);
    CHECK(get16(ram, 0x4C00u, 0x0020u) == 0x0049u);                 /* 1001 misses: odd, so node 1 */

    /* Cursor cell = SS:BP-2: the head lands on the match flag, the result copy and the saved BP. Head 3000:1200,
     * node key matches: [BP-2] := 1 makes the cursor 3000:1201, the CMP exits, and POP BP takes the head segment. */
    background();
    ram[at(0x3000u, 0x1200u)] = 0x5A;
    put16(ram, 0x4800u, 0x017Fu, 0x1200u); put16(ram, 0x4800u, 0x0181u, 0x3000u);
    cpu = frame(0x005Au, SS, SP0 - 4u, 0x0202u);
    CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 254u) == PR_RES_2199_OK);
    CHECK(cpu.ax == 0x3201 && cpu.dx == 0x3000 && cpu.bp == 0x3000 && cpu.es == SS && cpu.di == SP0 - 4u);
    CHECK(cpu.flags == flags_cmp_one(0x0202u) && cpu.sp == SP0 + 14u && cpu.ip == RET_IP && cpu.cs == RET_CS);
    CHECK(get16(ram, SS, SP0 - 4u) == 0x0101u && get16(ram, SS, SP0 - 2u) == 0x3000u);

    /* 200 random non-aliasing worlds against the credited callback model: same AL, same cursor cell, and RAM equal
     * outside the raw model's own frame bytes. */
    for (unsigned trial = 0u; trial < 200u; ++trial) {
        pr_rec_lookup_io io;
        pr_rec_far_ptr cursor_cell, character;
        uint8_t found = 0xEE, keys[8];
        unsigned n = rnd(9u), hit = rnd(2u) ? rnd(n + 1u) : n, key;
        background();
        for (unsigned i = 0u; i < n; ++i) {
            unsigned k;
            do {
                k = rnd(256u);
                for (unsigned j = 0u; j < i; ++j)
                    if (keys[j] == k) k = 256u;
            } while (k == 256u);
            keys[i] = (uint8_t)k;
            node(i, keys[i], i + 1u < n ? 0x3000u + 0x100u * (i + 1u) : 0u, i + 1u < n ? 0x0040u + 9u * (i + 1u) : 0u);
        }
        put16(ram, 0x4800u, 0x017Fu, n ? 0x0040u : 0u); put16(ram, 0x4800u, 0x0181u, n ? 0x3000u : 0u);
        if (hit < n) {
            key = keys[hit];
        } else {
            key = 0u;
            for (int clash = 1; clash;) {
                clash = 0;
                key = rnd(256u);
                for (unsigned j = 0u; j < n; ++j)
                    clash |= keys[j] == key;
            }
        }
        key |= rnd(256u) << 8;
        cpu = frame(key, 0x4C00u, 0x0020u + rnd(0x100u), rnd(2u) ? 0x0202u : 0x0ED7u);
        memcpy(other, ram, RAM);
        io.context = other; io.read_word = cb_read_word; io.write_word = cb_write_word; io.read_byte = cb_read_byte;
        io.max_nodes = 1000u;
        cursor_cell.offset = (uint16_t)get16(ram, SS, SP0 + 4u);
        cursor_cell.segment = 0x4C00u;
        character.offset = 0x0100u;
        character.segment = 0x4800u;
        CHECK(pr_rec_res_00ba_2199(&io, cursor_cell, (uint16_t)key, character, &found) == PR_REC_LOOKUP_OK);
        CHECK(pr_res_00ba_2199_cpu(ram, RAM, &cpu, 254u) == PR_RES_2199_OK);
        CHECK(found == (uint8_t)cpu.ax && found == (hit < n));
        CHECK(get16(ram, 0x4C00u, cursor_cell.offset) == get16(other, 0x4C00u, cursor_cell.offset));
        CHECK(get16(ram, 0x4C00u, cursor_cell.offset + 2u) == get16(other, 0x4C00u, cursor_cell.offset + 2u));
        for (unsigned k = 0u; k < 4u; ++k)                     /* saved BP, [BP-2], [BP-1]: the raw model's own frame */
            other[at(SS, SP0 - 4u + k)] = ram[at(SS, SP0 - 4u + k)];
        CHECK(memcmp(ram, other, RAM) == 0);
        check_return(&cpu);
    }

    /* Argument rejections leave RAM and CPU unchanged. */
    {
        pr_res_2199_cpu saved;
        background();
        cpu = frame(0x0011u, 0x4C00u, 0x0020u, 0x0202u);
        saved = cpu;
        memcpy(other, ram, RAM);
        CHECK(pr_res_00ba_2199_cpu(NULL, RAM, &cpu, 254u) == PR_RES_2199_ARGUMENT);
        CHECK(pr_res_00ba_2199_cpu(ram, RAM, NULL, 254u) == PR_RES_2199_ARGUMENT);
        CHECK(pr_res_00ba_2199_cpu(ram, 0u, &cpu, 254u) == PR_RES_2199_ARGUMENT);
        CHECK(pr_res_00ba_2199_cpu(ram, RAM - 1u, &cpu, 254u) == PR_RES_2199_ARGUMENT);
        CHECK(pr_res_00ba_2199_cpu(ram, (size_t)RAM + 1u, &cpu, 254u) == PR_RES_2199_ARGUMENT);
        CHECK(memcmp(&cpu, &saved, sizeof cpu) == 0 && memcmp(ram, other, RAM) == 0);
    }

    printf("PASS: RES_00BA_2199 raw CPU, %u checks (empty, hit, miss, cap, cycle, frame alias, 200 callback-model "
           "cross-checks, arguments)\n", checks);
    return 0;
}
