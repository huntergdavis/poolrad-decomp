#include "poolrad/recovered/overlay_u3bd_place_0211.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U3BD_0211_RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = at(seg, off);
    return (uint16_t)(ram[p] |
                      ((uint16_t)ram[(p + 1u) & (PR_U3BD_0211_RAM_SIZE - 1u)] << 8));
}

static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = at(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U3BD_0211_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u3bd_0211_cpu frame(uint8_t *ram, uint16_t sp,
                              uint8_t view, uint8_t slice,
                              uint8_t y, uint8_t x)
{
    pr_u3bd_0211_cpu cpu = {0xABCDu, 0x2345u, 0x3456u, 0x4567u,
                            0x5678u, 0x6789u, 0x7777u, sp,
                            0x2000u, 0x0211u, 0x3000u, 0x4000u,
                            0x1000u, 0x0202u};
    put_word(ram, cpu.ss, sp, 0xCAFEu);
    {
        size_t p = (at(cpu.ss, sp) + 2u) & (PR_U3BD_0211_RAM_SIZE - 1u);
        ram[p] = 0xEFu;
        ram[(p + 1u) & (PR_U3BD_0211_RAM_SIZE - 1u)] = 0xBEu;
    }
    put_word(ram, cpu.ss, (uint16_t)(sp + 4u), view);
    put_word(ram, cpu.ss, (uint16_t)(sp + 6u), slice);
    put_word(ram, cpu.ss, (uint16_t)(sp + 8u), y);
    put_word(ram, cpu.ss, (uint16_t)(sp + 10u), x);
    return cpu;
}

typedef struct { unsigned calls; uint16_t return_alias; } callback_state;

static int tile(uint8_t *ram, size_t ram_size, pr_u3bd_0211_cpu *cpu,
                void *context)
{
    callback_state *state = context;
    uint16_t ip, cs;
    assert(ram_size == PR_U3BD_0211_RAM_SIZE);
    assert(cpu->cs == 0x0C4Cu && cpu->ip == 0x002Au);
    assert(word(ram, cpu->ss, cpu->sp) == 0x0300u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0x2000u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 1u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 1u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 8u)) == 7u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 10u)) == 3u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 12u)) == 4u);
    ip = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cs = word(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 12u); /* return CS and five arguments */
    cpu->ip = ip;
    cpu->cs = cs;
    cpu->es = 0x5555u;
    ram[at(0x6000u, 0x1234u)] = 0xA5u;
    if (state->return_alias != 0u) {
        put_word(ram, cpu->ss, state->return_alias, 0x1234u);
        put_word(ram, cpu->ss, (uint16_t)(state->return_alias + 2u), 0x5678u);
    }
    ++state->calls;
    return 0;
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3BD_0211_RAM_SIZE, 1u);
    pr_u3bd_0211_cpu cpu;
    callback_state state = {0, 0};
    assert(ram != NULL);
    assert(pr_u3bd_place_0211(NULL, PR_U3BD_0211_RAM_SIZE,
                              &cpu, NULL, NULL) == PR_U3BD_0211_ARGUMENT);

    /* Empty row count takes the +0261 CMP exit and live RETF 8 frame. */
    cpu = frame(ram, 0x8000u, 0u, 14u, 3u, 5u);
    ram[at(cpu.ds, 0x0C96u)] = 1u;
    assert(pr_u3bd_place_0211(ram, PR_U3BD_0211_RAM_SIZE,
                              &cpu, NULL, NULL) == PR_U3BD_0211_OK);
    assert(cpu.ax == 3u && cpu.bp == 0x7777u && cpu.sp == 0x800Cu);
    assert(cpu.cs == 0xBEEFu && cpu.ip == 0xCAFEu);
    assert(cpu.flags == 0x0202u);

    /* Clipping still advances the byte cursor, including FF -> 00. */
    cpu = frame(ram, 0x8100u, 1u, 1u, 0u, 0xFFu);
    ram[at(cpu.ds, 0x0C8Du)] = 0xFFu;
    ram[at(cpu.ds, 0x0C97u)] = 1u;
    ram[at(cpu.ds, 0x0CA1u)] = 1u;
    assert(pr_u3bd_place_0211(ram, PR_U3BD_0211_RAM_SIZE,
                              &cpu, NULL, NULL) == PR_U3BD_0211_OK);
    assert(ram[at(cpu.ss, 0x8100u - 2u - 9u)] == 0u);
    assert(cpu.sp == 0x810Cu && cpu.flags == 0x0246u);

    /* One positive tile reaches a far call with the original five words. */
    cpu = frame(ram, 0x8200u, 2u, 1u, 1u, 2u);
    ram[at(cpu.ds, 0x0C8Eu)] = 0u;
    ram[at(cpu.ds, 0x0C98u)] = 1u;
    ram[at(cpu.ds, 0x0CA2u)] = 1u;
    put_word(ram, cpu.ds, 0x6A58u, 0x0200u);
    put_word(ram, cpu.ds, 0x6A5Au, 0x5000u);
    ram[at(0x5000u, 0x0200u)] = 7u;
    put_word(ram, cpu.cs, 0x02FCu, 0x002Au);
    put_word(ram, cpu.cs, 0x02FEu, 0x0C4Cu);
    assert(pr_u3bd_place_0211(ram, PR_U3BD_0211_RAM_SIZE,
                              &cpu, tile, &state) == PR_U3BD_0211_OK);
    assert(state.calls == 1u && cpu.es == 0x5555u);
    assert(ram[at(0x6000u, 0x1234u)] == 0xA5u);
    assert(cpu.sp == 0x820Cu && cpu.ip == 0xCAFEu && cpu.cs == 0xBEEFu);

    /* The callee may alias the helper's far return; RETF reads it live. */
    state.return_alias = 0x8300u;
    cpu = frame(ram, 0x8300u, 2u, 1u, 1u, 2u);
    assert(pr_u3bd_place_0211(ram, PR_U3BD_0211_RAM_SIZE,
                              &cpu, tile, &state) == PR_U3BD_0211_OK);
    assert(state.calls == 2u && cpu.sp == 0x830Cu);
    assert(cpu.ip == 0x1234u && cpu.cs == 0x5678u);

    /* RETF's far-return words are contiguous across the SS offset end. */
    cpu = frame(ram, 0xFFFEu, 0u, 0u, 0u, 0u);
    ram[at(cpu.ds, 0x0CA0u)] = 0u;
    assert(pr_u3bd_place_0211(ram, PR_U3BD_0211_RAM_SIZE,
                              &cpu, NULL, NULL) == PR_U3BD_0211_OK);
    assert(cpu.sp == 0x000Au && cpu.ip == 0xCAFEu && cpu.cs == 0xBEEFu);

    free(ram);
    return 0;
}
