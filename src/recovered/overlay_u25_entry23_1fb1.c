/* GAME.OVR unit 0x25:+1FB1..+207D, complete 205-byte public entry 23. The
 * 78 bytes +207E..+20CB are +20CC's literal data, not part of this routine. */
#include "poolrad/recovered/overlay_u25_entry23_1fb1.h"

#define RAM_MASK (PR_U25_1FB1_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_1fb1_cpu *cpu;
    pr_u25_1fb1_external external;
    void *context;
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

/* SUB/CMP flags at the width given by sign. */
static uint16_t sub_flags(pr_u25_1fb1_cpu *cpu, uint16_t a, uint16_t b,
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

/* OR: CF OF clear, SZP from the result. AF is architecturally undefined;
 * the reference emulator's cleared AF is used. */
static void logic_flags(pr_u25_1fb1_cpu *cpu, uint16_t r, uint16_t sign)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
    if (r == 0u) cpu->flags |= 0x0040u;
    if (r & sign) cpu->flags |= 0x0080u;
}

static int zf(const pr_u25_1fb1_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

static void set_al(pr_u25_1fb1_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* les di,[bp+disp] */
static void les_frame(machine *m, uint16_t disp)
{
    uint16_t at = (uint16_t)(m->cpu->bp + disp);
    m->cpu->di = read16(m, m->cpu->ss, at);
    m->cpu->es = read16(m, m->cpu->ss, (uint16_t)(at + 2u));
}

static uint8_t local8(const machine *m, uint16_t disp)
{
    return read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_local8(machine *m, uint16_t disp, uint8_t value)
{
    write8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
}

/* les di,[bp-4]; mov al,es:[di+0x11C] */
static void node_value(machine *m)
{
    les_frame(m, 0xFFFCu);
    set_al(m->cpu, read8(m, m->cpu->es, (uint16_t)(m->cpu->di + 0x011Cu)));
}

/* +1FF6..+2009 / +2019..+202C: lookup(cursor SS:[bp-8], key, node). */
static int lookup(machine *m, uint8_t key, uint16_t site)
{
    pr_u25_1fb1_cpu *cpu = m->cpu;
    push16(m, read16(m, cpu->ss, (uint16_t)(cpu->bp - 2u)));
    push16(m, read16(m, cpu->ss, (uint16_t)(cpu->bp - 4u)));
    set_al(cpu, key);
    push16(m, cpu->ax);
    cpu->di = (uint16_t)(cpu->bp - 8u);
    push16(m, cpu->ss);
    push16(m, cpu->di);
    cpu->ip = site;
    if (!m->external(m->context, m->ram, m->size, cpu, PR_U25_1FB1_LOOKUP))
        return 0;
    cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
    cpu->ip = (uint16_t)(site + 5u);
    logic_flags(cpu, (uint8_t)cpu->ax, 0x80u);             /* OR AL,AL */
    return 1;
}

/* mov al,[bp-9]; xor ah,ah; shl/shr ax,1; mov [bp-9],al. The XOR and
 * shift flags are dead before the next CMP. */
static void scale(machine *m, int left)
{
    pr_u25_1fb1_cpu *cpu = m->cpu;
    set_al(cpu, local8(m, 0xFFF7u));
    cpu->ax &= 0x00FFu;
    cpu->ax = left ? (uint16_t)(cpu->ax << 1) : (uint16_t)(cpu->ax >> 1);
    set_local8(m, 0xFFF7u, (uint8_t)cpu->ax);
}

pr_u25_1fb1_status pr_u25_entry23_1fb1(
    uint8_t *ram, size_t ram_size, pr_u25_1fb1_cpu *cpu,
    pr_u25_1fb1_external external, void *context)
{
    machine m;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_1FB1_RAM_SIZE)
        return PR_U25_1FB1_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = external; m.context = context;
    push16(&m, cpu->bp);                                   /* +1FB1 */
    cpu->bp = cpu->sp;
    cpu->sp = sub_flags(cpu, cpu->sp, 0x0Au, 0x8000u);
    cpu->ax = read16(&m, cpu->ds, 0x5D96u);                /* +1FB7 */
    cpu->dx = read16(&m, cpu->ds, 0x5D98u);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    /* Both outputs are seeded from the head node, even a null head. */
    node_value(&m);                                        /* +1FC4 */
    les_frame(&m, 0x0Au);
    write8(&m, cpu->es, cpu->di, (uint8_t)cpu->ax);
    node_value(&m);                                        /* +1FD2 */
    les_frame(&m, 6u);
    write8(&m, cpu->es, cpu->di, (uint8_t)cpu->ax);
    for (;;) {
        uint8_t value;
        cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u)); /* +1FE0 */
        cpu->ax = (uint16_t)(cpu->ax | read16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u)));
        logic_flags(cpu, cpu->ax, 0x8000u);
        if (zf(cpu)) break;
        node_value(&m);                                    /* +1FEB */
        set_local8(&m, 0xFFF7u, (uint8_t)cpu->ax);
        if (!lookup(&m, 0x27u, 0x2004u)) return PR_U25_1FB1_CALL_ABORT;
        if (!zf(cpu)) {
            scale(&m, 1);                                  /* +200D */
        } else {
            if (!lookup(&m, 0x2Au, 0x2027u)) return PR_U25_1FB1_CALL_ABORT;
            if (!zf(cpu)) scale(&m, 0);                    /* +2030 */
        }
        value = local8(&m, 0xFFF7u);                       /* +203A */
        set_al(cpu, value);
        les_frame(&m, 0x0Au);
        (void)sub_flags(cpu, value, read8(&m, cpu->es, cpu->di), 0x80u);
        if ((cpu->flags & 0x0041u) == 0u) {                /* not JBE */
            set_al(cpu, local8(&m, 0xFFF7u));
            les_frame(&m, 0x0Au);
            write8(&m, cpu->es, cpu->di, (uint8_t)cpu->ax);
        }
        value = local8(&m, 0xFFF7u);                       /* +204E */
        set_al(cpu, value);
        les_frame(&m, 6u);
        (void)sub_flags(cpu, value, read8(&m, cpu->es, cpu->di), 0x80u);
        if (cpu->flags & 0x0001u) {                        /* not JAE */
            set_al(cpu, local8(&m, 0xFFF7u));
            les_frame(&m, 6u);
            write8(&m, cpu->es, cpu->di, (uint8_t)cpu->ax);
        }
        les_frame(&m, 0xFFFCu);                            /* +2062 */
        cpu->ax = read16(&m, cpu->es, (uint16_t)(cpu->di + 0x0104u));
        cpu->dx = read16(&m, cpu->es, (uint16_t)(cpu->di + 0x0106u));
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    }
    cpu->sp = cpu->bp;                                     /* +2078 */
    cpu->bp = pop16(&m);
    cpu->ip = pop16(&m);
    cpu->cs = pop16(&m);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U25_1FB1_OK;
}
