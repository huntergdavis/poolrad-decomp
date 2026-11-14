#include "poolrad/replay/slot_a_movement_0747.h"

#include <string.h>

static void put_word(uint8_t *ram, unsigned at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

pr_slot_a_movement_status pr_slot_a_movement_0747(
    uint8_t *ram, size_t capacity, size_t backed,
    const uint8_t *authenticated_code, size_t code_size,
    pr_r08ae_00dc_cpu *cpu)
{
    const unsigned ds = 0x14a00u, stack = 0x1d160u, code = 0x2e600u + 0x747u;
    if (!ram || !authenticated_code || !cpu ||
        capacity != PR_SLOT_A_MOVEMENT_RAM_SIZE ||
        backed != PR_SLOT_A_MOVEMENT_BACKED_SIZE ||
        code_size != PR_SLOT_A_MOVEMENT_CODE_SIZE ||
        code + PR_SLOT_A_MOVEMENT_CODE_SIZE > backed || stack + 0x3dbeu > backed ||
        memcmp(ram + code, authenticated_code, PR_SLOT_A_MOVEMENT_CODE_SIZE) ||
        cpu->ax != 0xffffu || cpu->bx != 0 || cpu->cx != 10 || cpu->dx != 0xfea5u ||
        cpu->si != 0 || cpu->di != 0 || cpu->bp != 0x3dbeu || cpu->sp != 0x3dbeu ||
        cpu->cs != 0x2e60u || cpu->ip != 0x0747u || cpu->ds != 0x14a0u ||
        cpu->es != 0x1d16u || cpu->ss != 0x1d16u || cpu->flags != 0x7246u ||
        ram[ds + 0x6aadu] != 15 || ram[ds + 0x6aaeu] != 4 || ram[ds + 0x6aafu] != 6 ||
        ram[ds + 0x273cu] != 0xffu || ram[ds + 0x2745u] != 0)
        return PR_SLOT_A_MOVEMENT_REFUSED;

    /* Fixed final CMP is 4-15; its arithmetic flags replace 08D5. */
    ram[ds + 0x6aadu] = 14;
    ram[ds + 0x6aaeu] = 4;
    cpu->di = 6;
    cpu->ax = 6;
    cpu->flags = (uint16_t)((cpu->flags & ~0x08d5u) | 0x0095u);
    cpu->sp = 0x3db8u;
    put_word(ram, stack + 0x3dbcu, 14);
    put_word(ram, stack + 0x3dbau, 4);
    put_word(ram, stack + 0x3db8u, 6);
    cpu->ip = 0x07a1u;
    return PR_SLOT_A_MOVEMENT_AT_LOOKUP;
}
