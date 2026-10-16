/* GAME.OVR unit 0x25:+0511..+0543, complete 51-byte public entry 7. The 10
 * bytes +0544..+054D that follow are +054E's data, not part of this
 * routine. */
#include "poolrad/recovered/overlay_u25_entry7_0511.h"

#define RAM_MASK (PR_U25_0511_RAM_SIZE - 1u)

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

static void push16(uint8_t *ram, pr_u25_0511_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(const uint8_t *ram, pr_u25_0511_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t arg8(const uint8_t *ram, const pr_u25_0511_cpu *cpu, uint16_t disp)
{
    return ram[physical(cpu->ss, (uint16_t)(cpu->bp + disp))];
}

/* mov al,value; push ax */
static void push_al(uint8_t *ram, pr_u25_0511_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
    push16(ram, cpu, cpu->ax);
}

static int far_call(uint8_t *ram, size_t size, pr_u25_0511_cpu *cpu,
                    pr_u25_0511_external external, void *context,
                    pr_u25_0511_call call, uint16_t site, uint16_t cleanup)
{
    cpu->ip = site;
    if (!external(context, ram, size, cpu, call)) return 0;
    cpu->sp = (uint16_t)(cpu->sp + cleanup);
    cpu->ip = (uint16_t)(site + 5u);
    return 1;
}

pr_u25_0511_status pr_u25_entry7_0511(
    uint8_t *ram, size_t ram_size, pr_u25_0511_cpu *cpu,
    pr_u25_0511_external external, void *context)
{
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_0511_RAM_SIZE)
        return PR_U25_0511_ARGUMENT;
    /* No instruction in this body writes flags: each hook sees the flags
     * of whatever ran before it. */
    push16(ram, cpu, cpu->bp);                             /* +0511 */
    cpu->bp = cpu->sp;
    ram[physical(cpu->ds, 0x84E1u)] = 0u;                  /* +0514 */
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | arg8(ram, cpu, 8u));
    ram[physical(cpu->ds, 0x49FDu)] = (uint8_t)cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | arg8(ram, cpu, 6u));
    ram[physical(cpu->ds, 0x49FEu)] = (uint8_t)cpu->ax;
    push_al(ram, cpu, arg8(ram, cpu, 8u));                 /* +0525 */
    push_al(ram, cpu, arg8(ram, cpu, 6u));
    if (!far_call(ram, ram_size, cpu, external, context,
                  PR_U25_0511_U3B9_6, 0x052Du, 4u))
        return PR_U25_0511_CALL_ABORT;
    push_al(ram, cpu, 3u);                                 /* +0532 */
    push_al(ram, cpu, 3u);
    push_al(ram, cpu, 1u);
    if (!far_call(ram, ram_size, cpu, external, context,
                  PR_U25_0511_U3B9_5, 0x053Bu, 6u))
        return PR_U25_0511_CALL_ABORT;
    /* +0540: POP BP reads the live stack word at SP (no MOV SP,BP). */
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_0511_OK;
}
