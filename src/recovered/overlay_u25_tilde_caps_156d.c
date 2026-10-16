/* GAME.OVR unit 0x25:+156D..+1663, complete 247-byte string filter. Its
 * set constant is the preceding CS:154D..156C; CS:1664..1683 is +1684's. */
#include "poolrad/recovered/overlay_u25_tilde_caps_156d.h"

#define RAM_MASK (PR_U25_156D_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_156d_cpu *cpu;
    pr_u25_156d_external external;
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

/* ADD (subtract == 0) or SUB/CMP flags at the width given by sign. */
static uint16_t arith(pr_u25_156d_cpu *cpu, uint16_t a, uint16_t b,
                      uint16_t sign, int subtract)
{
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t r = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? a < b : wide > mask) cpu->flags |= 0x0001u;
    if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) cpu->flags |= 0x0010u;
    if (r == 0u) cpu->flags |= 0x0040u;
    if (r & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (uint16_t)(a ^ b) : (uint16_t)~(a ^ b)) & (a ^ r) & sign)
        cpu->flags |= 0x0800u;
    return r;
}

static int zf(const pr_u25_156d_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

/* XOR AH,AH. AF is architecturally undefined; the reference emulator's
 * cleared AF is used. */
static void xor_ah_ah(pr_u25_156d_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) | 0x0044u);
}

static void set_al(pr_u25_156d_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void les_arg(machine *m, uint16_t disp)
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

static int far_call(machine *m, pr_u25_156d_call call, uint16_t site,
                    uint16_t cleanup)
{
    m->cpu->ip = site;
    if (!m->external(m->context, m->ram, m->size, m->cpu, call)) return 0;
    m->cpu->sp = (uint16_t)(m->cpu->sp + cleanup);
    m->cpu->ip = (uint16_t)(site + 5u);
    return 1;
}

/* mov al,[bp-1]; xor ah,ah; les di,[bp+0xA]; add di,ax: ES:DI addresses
 * source byte i. The ADD flags reach the next call or CMP. */
static void source_index(machine *m)
{
    pr_u25_156d_cpu *cpu = m->cpu;
    set_al(cpu, local8(m, 0xFFFFu));
    xor_ah_ah(cpu);
    les_arg(m, 0x0Au);
    cpu->di = arith(cpu, cpu->di, cpu->ax, 0x8000u, 0);
}

/* +1596..+164E: one character. */
static int character(machine *m)
{
    pr_u25_156d_cpu *cpu = m->cpu;
    source_index(m);                                       /* +1596 */
    set_al(cpu, read8(m, cpu->es, cpu->di));
    push16(m, cpu->ax);
    cpu->di = 0x154Du;
    push16(m, cpu->cs);
    push16(m, cpu->di);
    if (!far_call(m, PR_U25_156D_IN_SET, 0x15A9u, 6u)) return 0;
    if (!zf(cpu)) {                                        /* +15B0 */
        uint8_t dl;
        source_index(m);
        set_al(cpu, read8(m, cpu->es, cpu->di));
        xor_ah_ah(cpu);
        cpu->ax = arith(cpu, cpu->ax, 0x20u, 0x8000u, 0);  /* flags dead */
        dl = (uint8_t)cpu->ax;
        cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | dl);
        source_index(m);
        write8(m, cpu->es, cpu->di, dl);
    }
    source_index(m);                                       /* +15D1 */
    (void)arith(cpu, read8(m, cpu->es, cpu->di), 0x7Eu, 0x80u, 1);
    if (!zf(cpu)) return 1;
    les_arg(m, 0x0Au);                                     /* +15E1 */
    push16(m, cpu->es);
    push16(m, cpu->di);
    set_al(cpu, local8(m, 0xFFFFu));
    xor_ah_ah(cpu);
    push16(m, cpu->ax);
    cpu->ax = 1u;
    push16(m, cpu->ax);
    if (!far_call(m, PR_U25_156D_DELETE, 0x15F0u, 8u)) return 0;
    source_index(m);                                       /* +15F5 */
    set_al(cpu, read8(m, cpu->es, cpu->di));
    push16(m, cpu->ax);
    if (!far_call(m, PR_U25_156D_UPCASE, 0x1603u, 2u)) return 0;
    cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) | (cpu->ax & 0x00FFu));
    source_index(m);
    write8(m, cpu->es, cpu->di, (uint8_t)cpu->dx);
    cpu->di = (uint16_t)(cpu->bp - 0x202u);                /* +1617 */
    push16(m, cpu->ss);
    push16(m, cpu->di);
    les_arg(m, 6u);
    push16(m, cpu->es);
    push16(m, cpu->di);
    if (!far_call(m, PR_U25_156D_STRLOAD, 0x1622u, 4u)) return 0;
    cpu->di = (uint16_t)(cpu->bp - 0x102u);                /* +1627 */
    push16(m, cpu->ss);
    push16(m, cpu->di);
    source_index(m);
    set_al(cpu, read8(m, cpu->es, cpu->di));
    push16(m, cpu->ax);
    if (!far_call(m, PR_U25_156D_CHARSTR, 0x163Bu, 2u)) return 0;
    if (!far_call(m, PR_U25_156D_CONCAT, 0x1640u, 4u)) return 0;
    les_arg(m, 6u);                                        /* +1645 */
    push16(m, cpu->es);
    push16(m, cpu->di);
    cpu->ax = 0x0028u;
    push16(m, cpu->ax);
    return far_call(m, PR_U25_156D_STRSTORE, 0x164Eu, 0x0Au);
}

pr_u25_156d_status pr_u25_tilde_caps_156d(
    uint8_t *ram, size_t ram_size, pr_u25_156d_cpu *cpu,
    pr_u25_156d_external external, void *context)
{
    machine m;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_156D_RAM_SIZE)
        return PR_U25_156D_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = external; m.context = context;
    push16(&m, cpu->bp);                                   /* +156D */
    cpu->bp = cpu->sp;
    cpu->sp = arith(cpu, cpu->sp, 0x0202u, 0x8000u, 1);
    les_arg(&m, 6u);                                       /* +1574 */
    write8(&m, cpu->es, cpu->di, 0u);
    les_arg(&m, 0x0Au);
    set_al(cpu, read8(&m, cpu->es, cpu->di));
    set_local8(&m, 0xFFFEu, (uint8_t)cpu->ax);             /* length */
    set_al(cpu, 1u);                                       /* +1584 */
    (void)arith(cpu, 1u, local8(&m, 0xFFFEu), 0x80u, 1);
    if (cpu->flags & 0x0041u) {                            /* JBE */
        set_local8(&m, 0xFFFFu, (uint8_t)cpu->ax);         /* +158E */
        for (;;) {
            if (!character(&m)) return PR_U25_156D_CALL_ABORT;
            set_al(cpu, local8(&m, 0xFFFFu));              /* +1653 */
            (void)arith(cpu, (uint8_t)cpu->ax, local8(&m, 0xFFFEu), 0x80u, 1);
            if (zf(cpu)) break;
            /* INC byte [bp-1]: flags dead (XOR AH,AH follows). */
            set_local8(&m, 0xFFFFu, (uint8_t)(local8(&m, 0xFFFFu) + 1u));
        }
    }
    cpu->sp = cpu->bp;                                     /* +165E */
    cpu->bp = pop16(&m);
    cpu->ip = pop16(&m);
    cpu->cs = pop16(&m);
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U25_156D_OK;
}
