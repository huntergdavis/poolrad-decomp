/* GAME.OVR unit 0x25:+1788..+1814, complete 141-byte public entry 14,
 * composed with the full-state +156D near callee. */
#include "poolrad/recovered/overlay_u25_entry14_1788.h"

#define RAM_MASK (PR_U25_1788_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_1788_cpu *cpu;
    pr_u25_1788_external external;
    void *context;
} machine;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const machine *m, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(m->ram[a] | ((uint16_t)m->ram[(a + 1u) & RAM_MASK] << 8));
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

/* SUB SP,0x154 flags. */
static uint16_t sub_flags(pr_u25_1788_cpu *cpu, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (a < b) cpu->flags |= 0x0001u;
    if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) cpu->flags |= 0x0010u;
    if (r == 0u) cpu->flags |= 0x0040u;
    if (r & 0x8000u) cpu->flags |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) cpu->flags |= 0x0800u;
    return r;
}

static void set_al(pr_u25_1788_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t frame8(const machine *m, uint16_t disp)
{
    return m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp + disp))];
}

/* les di,[bp+disp]; push es; push di */
static void push_far_arg(machine *m, uint16_t disp)
{
    uint16_t at = (uint16_t)(m->cpu->bp + disp);
    m->cpu->di = read16(m, m->cpu->ss, at);
    m->cpu->es = read16(m, m->cpu->ss, (uint16_t)(at + 2u));
    push16(m, m->cpu->es);
    push16(m, m->cpu->di);
}

/* lea di,[bp-disp]; push ss; push di */
static void push_local(machine *m, uint16_t disp)
{
    m->cpu->di = (uint16_t)(m->cpu->bp - disp);
    push16(m, m->cpu->ss);
    push16(m, m->cpu->di);
}

/* mov al,[bp+disp]; push ax */
static void push_byte_arg(machine *m, uint16_t disp)
{
    set_al(m->cpu, frame8(m, disp));
    push16(m, m->cpu->ax);
}

static int far_call(machine *m, unsigned call, uint16_t site, uint16_t cleanup)
{
    m->cpu->ip = site;
    if (!m->external(m->context, m->ram, m->size, m->cpu, call)) return 0;
    m->cpu->sp = (uint16_t)(m->cpu->sp + cleanup);
    m->cpu->ip = (uint16_t)(site + 5u);
    return 1;
}

/* Forward the composed +156D's runtime calls to the +1788 hook. */
static int forward_156d(void *context, uint8_t *ram, size_t ram_size,
                        pr_u25_156d_cpu *cpu, pr_u25_156d_call call)
{
    const machine *m = context;
    return m->external(m->context, ram, ram_size, cpu,
                       (unsigned)PR_U25_1788_156D_BASE + (unsigned)call);
}

static int store_local(machine *m, uint16_t arg, uint16_t local,
                       uint16_t cap, uint16_t site)
{
    push_far_arg(m, arg);
    push_local(m, local);
    m->cpu->ax = cap;
    push16(m, m->cpu->ax);
    return far_call(m, PR_U25_1788_STRSTORE, site, 0x0Au);
}

pr_u25_1788_status pr_u25_entry14_1788(
    uint8_t *ram, size_t ram_size, pr_u25_1788_cpu *cpu,
    pr_u25_1788_external external, void *context)
{
    static const uint16_t byte_args[7] = {
        0x24u, 0x22u, 0x20u, 0x1Eu, 0x1Cu, 0x1Au, 0x18u
    };
    machine m;
    uint16_t entry_cs;
    unsigned i;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_1788_RAM_SIZE)
        return PR_U25_1788_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = external; m.context = context;
    entry_cs = cpu->cs;
    push16(&m, cpu->bp);                                   /* +1788 */
    cpu->bp = cpu->sp;
    cpu->sp = sub_flags(cpu, cpu->sp, 0x0154u);
    if (!store_local(&m, 0x2Au, 0x29u, 0x28u, 0x179Du))    /* +178F */
        return PR_U25_1788_CALL_ABORT;
    if (!store_local(&m, 0x26u, 0x129u, 0xFFu, 0x17B1u))   /* +17A2 */
        return PR_U25_1788_CALL_ABORT;
    push_local(&m, 0x129u);                                /* +17B6 */
    push_local(&m, 0x153u);
    push16(&m, cpu->cs);
    push16(&m, 0x17C6u);
    cpu->ip = 0x156Du;
    switch (pr_u25_tilde_caps_156d(ram, ram_size, cpu, forward_156d, &m)) {
    case PR_U25_156D_OK:
        break;
    case PR_U25_156D_CALL_ABORT:
        return PR_U25_1788_CALL_ABORT;
    default:
        return PR_U25_1788_ARGUMENT;
    }
    if (cpu->ip != 0x17C6u || cpu->cs != entry_cs)
        return PR_U25_1788_TRANSFER;
    push_local(&m, 0x29u);                                 /* +17C6 */
    push_local(&m, 0x129u);
    for (i = 0u; i < 7u; ++i)
        push_byte_arg(&m, byte_args[i]);
    push16(&m, read16(&m, cpu->ss, (uint16_t)(cpu->bp + 0x16u)));
    push16(&m, read16(&m, cpu->ss, (uint16_t)(cpu->bp + 0x14u)));
    push_byte_arg(&m, 0x12u);
    push_far_arg(&m, 0x0Eu);
    push_far_arg(&m, 0x0Au);
    push_far_arg(&m, 0x06u);
    if (!far_call(&m, PR_U25_1788_U3B0_4, 0x1806u, 0x28u))
        return PR_U25_1788_CALL_ABORT;
    m.ram[physical(cpu->ss, (uint16_t)(cpu->bp - 0x12Au))] = /* +180B */
        (uint8_t)cpu->ax;
    cpu->sp = cpu->bp;                                     /* +180F */
    cpu->bp = pop16(&m);
    cpu->ip = pop16(&m);
    cpu->cs = pop16(&m);
    cpu->sp = (uint16_t)(cpu->sp + 0x28u);
    return PR_U25_1788_OK;
}
