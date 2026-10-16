/* GAME.OVR unit 0x25:+0482..+0510, complete 143-byte public entry 6. */
#include "poolrad/recovered/overlay_u25_step_probe_0482.h"

#define RAM_MASK (PR_U25_0482_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

typedef struct {
    uint8_t *ram;
    pr_u25_0482_cpu *cpu;
} machine;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const machine *m, uint16_t seg, uint16_t off)
{
    return m->ram[physical(seg, off)];
}

static uint16_t read16(const machine *m, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(m->ram[a] | ((uint16_t)m->ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(machine *m, uint16_t seg, uint16_t off, uint8_t value)
{
    m->ram[physical(seg, off)] = value;
}

static void write16(machine *m, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    m->ram[a] = (uint8_t)value;
    m->ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t pop16(machine *m)
{
    uint16_t value = read16(m, m->cpu->ss, m->cpu->sp);
    m->cpu->sp = (uint16_t)(m->cpu->sp + 2u);
    return value;
}

static int parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* SUB/CMP flags at the width given by sign; returns the result. */
static uint16_t sub_flags(pr_u25_0482_cpu *cpu, uint16_t a, uint16_t b,
                          uint16_t sign)
{
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint16_t r = (uint16_t)((a - b) & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (a < b) cpu->flags |= 0x0001u;
    if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) cpu->flags |= 0x0010u;
    if (r == 0u) cpu->flags |= 0x0040u;
    if (r & sign) cpu->flags |= 0x0080u;
    if ((a ^ b) & (a ^ r) & sign) cpu->flags |= 0x0800u;
    return r;
}

static int zf(const pr_u25_0482_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

static void set_al(pr_u25_0482_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t frame8(const machine *m, uint16_t disp)
{
    return read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_frame8(machine *m, uint16_t disp, uint8_t value)
{
    write8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
}

/* INC/DEC byte [bp+disp]; the flags are dead (CMP at +04B1 follows). */
static void step_arg(machine *m, uint16_t disp, int delta)
{
    set_frame8(m, disp, (uint8_t)(frame8(m, disp) + delta));
}

pr_u25_0482_status pr_u25_step_probe_0482(
    uint8_t *ram, size_t ram_size, pr_u25_0482_cpu *cpu,
    pr_u25_0482_external external, void *context)
{
    machine m;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_0482_RAM_SIZE)
        return PR_U25_0482_ARGUMENT;
    m.ram = ram; m.cpu = cpu;
    push16(&m, cpu->bp);                                   /* +0482 */
    cpu->bp = cpu->sp;
    cpu->sp = sub_flags(cpu, cpu->sp, 4u, 0x8000u);
    set_frame8(&m, 0xFFFDu, 0u);                           /* count */
    set_frame8(&m, 0xFFFEu, 0u);                           /* stop */
    set_frame8(&m, 0xFFFFu, 0u);                           /* result */
    cpu->di = read16(&m, cpu->ds, 0x49D2u);                /* +0494 */
    cpu->es = read16(&m, cpu->ds, 0x49D4u);
    (void)sub_flags(cpu, read16(&m, cpu->es, (uint16_t)(cpu->di + 0x01CCu)),
                    0u, 0x8000u);
    if (zf(cpu)) {
        set_frame8(&m, 0xFFFFu, 2u);                       /* +04A0 */
        cpu->di = read16(&m, cpu->ds, 0x49D6u);
        cpu->es = read16(&m, cpu->ds, 0x49D8u);
        write16(&m, cpu->es, (uint16_t)(cpu->di + 0x0582u), 2u);
    } else {
        for (;;) {
            uint8_t dir;
            (void)sub_flags(cpu, frame8(&m, 0xFFFDu), 2u, 0x80u); /* +04B1 */
            if ((cpu->flags & 0x0001u) == 0u) break;       /* JAE */
            (void)sub_flags(cpu, frame8(&m, 0xFFFEu), 0u, 0x80u);
            if (!zf(cpu)) break;
            set_al(cpu, frame8(&m, 0x0Au));                /* +04BD */
            push16(&m, cpu->ax);
            set_al(cpu, frame8(&m, 8u));
            push16(&m, cpu->ax);
            set_al(cpu, frame8(&m, 6u));
            push16(&m, cpu->ax);
            cpu->ip = 0x04C9u;
            if (!external(context, ram, ram_size, cpu, PR_U25_0482_U3BD_2))
                return PR_U25_0482_CALL_ABORT;
            cpu->sp = (uint16_t)(cpu->sp + 6u);
            cpu->ip = 0x04CEu;
            /* OR AL,AL: CF OF clear, SZP from AL; AF undefined, taken as
             * the reference emulator's cleared AF. */
            cpu->flags &= (uint16_t)~ARITH_FLAGS;
            if (parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
            if ((cpu->ax & 0xFFu) == 0u) cpu->flags |= 0x0040u;
            if (cpu->ax & 0x80u) cpu->flags |= 0x0080u;
            if (!zf(cpu)) {
                set_frame8(&m, 0xFFFEu, 1u);               /* +0502 */
                continue;
            }
            /* INC byte [bp-3]: flags dead. */
            set_frame8(&m, 0xFFFDu, (uint8_t)(frame8(&m, 0xFFFDu) + 1u));
            set_al(cpu, frame8(&m, 0xFFFDu));
            set_frame8(&m, 0xFFFFu, (uint8_t)cpu->ax);
            dir = frame8(&m, 6u);                          /* +04DB */
            set_al(cpu, dir);
            if ((void)sub_flags(cpu, dir, 0u, 0x80u), zf(cpu))
                step_arg(&m, 8u, -1);
            else if ((void)sub_flags(cpu, dir, 2u, 0x80u), zf(cpu))
                step_arg(&m, 0x0Au, 1);
            else if ((void)sub_flags(cpu, dir, 4u, 0x80u), zf(cpu))
                step_arg(&m, 8u, 1);
            else if ((void)sub_flags(cpu, dir, 6u, 0x80u), zf(cpu))
                step_arg(&m, 0x0Au, -1);
        }
    }
    set_al(cpu, frame8(&m, 0xFFFFu));                      /* +0508 */
    cpu->sp = cpu->bp;
    cpu->bp = pop16(&m);
    cpu->ip = pop16(&m);
    cpu->cs = pop16(&m);
    cpu->sp = (uint16_t)(cpu->sp + 6u);
    return PR_U25_0482_OK;
}
