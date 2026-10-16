/* GAME.OVR unit 0x25:+1CC3..+1DEF, complete 301-byte public entry 20:
 * scale seven record words by a Real48 factor through SYSTEM helpers. */
#include "poolrad/recovered/overlay_u25_entry20_1cc3.h"

#define RAM_MASK (PR_U25_1CC3_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

/* les di,[bp+6] */
static void les_record(const uint8_t *ram, pr_u25_1cc3_cpu *cpu)
{
    uint16_t at = (uint16_t)(cpu->bp + 6u);
    cpu->di = read16(ram, cpu->ss, at);
    cpu->es = read16(ram, cpu->ss, (uint16_t)(at + 2u));
}

static int helper(uint8_t *ram, size_t size, pr_u25_1cc3_cpu *cpu,
                  pr_u25_1cc3_external external, void *context,
                  pr_u25_1cc3_call call, uint16_t site)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->ip = (uint16_t)(site + 5u);
    return 1;
}

pr_u25_1cc3_status pr_u25_entry20_1cc3(
    uint8_t *ram, size_t ram_size, pr_u25_1cc3_cpu *cpu,
    pr_u25_1cc3_external external, void *context)
{
    unsigned k;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_1CC3_RAM_SIZE)
        return PR_U25_1CC3_ARGUMENT;
    cpu->sp = (uint16_t)(cpu->sp - 2u);                    /* +1CC3 */
    write16(ram, cpu->ss, cpu->sp, cpu->bp);
    cpu->bp = cpu->sp;
    for (k = 0u; k < 7u; ++k) {
        uint16_t field = (uint16_t)(0x0088u + 2u * k);
        uint16_t site = (uint16_t)(0x1CD0u + 0x2Au * k);
        les_record(ram, cpu);                              /* +1CC6 + 2Ah*k */
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + field));
        cpu->dx = 0u;  /* XOR DX,DX: CF OF SF clear, ZF PF set, AF 0 */
        cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) | 0x0044u);
        if (!helper(ram, ram_size, cpu, external, context,
                    PR_U25_1CC3_LONG_TO_REAL, site))
            return PR_U25_1CC3_NO_RETURN;
        cpu->cx = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Au));
        cpu->si = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));
        cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x0Eu));
        if (!helper(ram, ram_size, cpu, external, context,
                    PR_U25_1CC3_REAL_MUL, (uint16_t)(site + 0x0Eu)))
            return PR_U25_1CC3_NO_RETURN;
        if (!helper(ram, ram_size, cpu, external, context,
                    PR_U25_1CC3_REAL_TRUNC, (uint16_t)(site + 0x13u)))
            return PR_U25_1CC3_NO_RETURN;
        les_record(ram, cpu);                              /* +1CE8 + 2Ah*k */
        write16(ram, cpu->es, (uint16_t)(cpu->di + field), cpu->ax);
    }
    cpu->bp = read16(ram, cpu->ss, cpu->sp);               /* +1DEC */
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->cs = read16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_U25_1CC3_OK;
}
