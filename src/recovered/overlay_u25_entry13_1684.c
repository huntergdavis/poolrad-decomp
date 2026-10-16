/* GAME.OVR unit 0x25:+1684..+1787, complete 260-byte public entry 13,
 * composed with the full-state +156D near callee. Its 32-byte set constant
 * is the preceding CS:1664..1683, read only by the set-test callee. */
#include "poolrad/recovered/overlay_u25_entry13_1684.h"

#define RAM_MASK (PR_U25_1684_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_1684_cpu *cpu;
    pr_u25_1684_external external;
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

/* SUB/CMP flags at the width given by sign. */
static uint16_t sub_flags(pr_u25_1684_cpu *cpu, uint16_t a, uint16_t b,
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

static int zf(const pr_u25_1684_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

static void set_al(pr_u25_1684_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint8_t frame8(const machine *m, uint16_t disp)
{
    return m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp + disp))];
}

static void set_frame8(machine *m, uint16_t disp, uint8_t value)
{
    m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp + disp))] = value;
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

/* mov al,value; push ax */
static void push_al(machine *m, uint8_t value)
{
    set_al(m->cpu, value);
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

static int forward_156d(void *context, uint8_t *ram, size_t ram_size,
                        pr_u25_156d_cpu *cpu, pr_u25_156d_call call)
{
    const machine *m = context;
    return m->external(m->context, ram, ram_size, cpu,
                       (unsigned)PR_U25_1684_156D_BASE + (unsigned)call);
}

static int store_local(machine *m, uint16_t arg, uint16_t local,
                       uint16_t cap, uint16_t site)
{
    push_far_arg(m, arg);
    push_local(m, local);
    m->cpu->ax = cap;
    push16(m, m->cpu->ax);
    return far_call(m, PR_U25_1684_STRSTORE, site, 0x0Au);
}

pr_u25_1684_status pr_u25_entry13_1684(
    uint8_t *ram, size_t ram_size, pr_u25_1684_cpu *cpu,
    pr_u25_1684_external external, void *context)
{
    machine m;
    uint16_t entry_cs;
    uint8_t key, i;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_1684_RAM_SIZE)
        return PR_U25_1684_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = external; m.context = context;
    entry_cs = cpu->cs;
    push16(&m, cpu->bp);                                   /* +1684 */
    cpu->bp = cpu->sp;
    cpu->sp = sub_flags(cpu, cpu->sp, 0x0156u, 0x8000u);
    if (!store_local(&m, 0x14u, 0x2Au, 0x28u, 0x1699u))    /* +168B */
        return PR_U25_1684_CALL_ABORT;
    if (!store_local(&m, 0x10u, 0x12Au, 0xFFu, 0x16ADu))   /* +169E */
        return PR_U25_1684_CALL_ABORT;
    push_local(&m, 0x12Au);                                /* +16B2 */
    push_local(&m, 0x153u);
    push16(&m, cpu->cs);
    push16(&m, 0x16C2u);
    cpu->ip = 0x156Du;
    switch (pr_u25_tilde_caps_156d(ram, ram_size, cpu, forward_156d, &m)) {
    case PR_U25_156D_OK:
        break;
    case PR_U25_156D_CALL_ABORT:
        return PR_U25_1684_CALL_ABORT;
    default:
        return PR_U25_1684_ARGUMENT;
    }
    if (cpu->ip != 0x16C2u || cpu->cs != entry_cs)
        return PR_U25_1684_TRANSFER;
    for (;;) {
        push_local(&m, 0x2Au);                             /* +16C2 */
        push_local(&m, 0x12Au);
        push_al(&m, frame8(&m, 0x0Eu));
        push_al(&m, frame8(&m, 0x0Cu));
        push_al(&m, frame8(&m, 0x0Au));
        push_al(&m, 1u);
        push_al(&m, frame8(&m, 6u));
        push_local(&m, 0x156u);
        if (!far_call(&m, PR_U25_1684_U3B0_2, 0x16E6u, 0x16u))
            return PR_U25_1684_CALL_ABORT;
        set_frame8(&m, 0xFEABu, (uint8_t)cpu->ax);         /* [bp-0x155] */
        (void)sub_flags(cpu, frame8(&m, 0xFEAAu), 0u, 0x80u);
        if (!zf(cpu)) {
            push_al(&m, frame8(&m, 0xFEABu));              /* +16F6 */
            if (!far_call(&m, PR_U25_1684_U77_10, 0x16FBu, 2u))
                return PR_U25_1684_CALL_ABORT;
            push16(&m, read16(&m, cpu->ds, 0x5D94u));      /* +1700 */
            push16(&m, read16(&m, cpu->ds, 0x5D92u));
            if (!far_call(&m, PR_U25_1684_LOAD_PIECES, 0x1708u, 4u))
                return PR_U25_1684_CALL_ABORT;
            set_frame8(&m, 0xFEABu, 0u);                   /* +170D */
        }
        push_al(&m, frame8(&m, 0xFEABu));                  /* +1712 */
        cpu->di = 0x1664u;
        push16(&m, cpu->cs);
        push16(&m, cpu->di);
        if (!far_call(&m, PR_U25_1684_IN_SET, 0x171Cu, 6u))
            return PR_U25_1684_CALL_ABORT;
        if (!zf(cpu)) break;                               /* member: +1730 */
        (void)sub_flags(cpu, frame8(&m, 0xFEABu), 0x0Du, 0x80u); /* +1723 */
        if (!zf(cpu)) continue;
        (void)sub_flags(cpu, frame8(&m, 8u), 0u, 0x80u);   /* +172A */
        if (zf(cpu)) continue;
        break;
    }
    key = frame8(&m, 0xFEABu);                             /* +1730 */
    set_al(cpu, key);
    (void)sub_flags(cpu, key, 0x0Du, 0x80u);
    if (zf(cpu)) {
        set_frame8(&m, 0xFFFFu, 0u);                       /* +1738 */
    } else {
        set_frame8(&m, 0xFEACu, 1u);                       /* +173E */
        for (;;) {
            i = frame8(&m, 0xFEACu);                       /* +1743 */
            set_al(cpu, i);
            (void)sub_flags(cpu, i, frame8(&m, 0xFEADu), 0x80u);
            if ((cpu->flags & 0x0041u) == 0u) break;       /* JA */
            set_al(cpu, frame8(&m, 0xFEACu));              /* +174D */
            cpu->ax &= 0x00FFu;  /* XOR AH,AH: flags dead (CMP follows) */
            cpu->di = cpu->ax;
            set_al(cpu, m.ram[physical(cpu->ss,
                                       (uint16_t)(cpu->bp + cpu->di - 0x153u))]);
            (void)sub_flags(cpu, (uint8_t)cpu->ax, frame8(&m, 0xFEABu), 0x80u);
            if (zf(cpu)) break;
            /* INC byte [bp-0x154]: flags dead (CMP at +1743 follows). */
            set_frame8(&m, 0xFEACu, (uint8_t)(frame8(&m, 0xFEACu) + 1u));
        }
        i = frame8(&m, 0xFEACu);                           /* +1765 */
        set_al(cpu, i);
        (void)sub_flags(cpu, i, frame8(&m, 0xFEADu), 0x80u);
        if ((cpu->flags & 0x0041u) == 0u) {                /* JA */
            set_frame8(&m, 0xFFFFu, 0xFFu);                /* +177B */
        } else {
            uint16_t r;
            set_al(cpu, frame8(&m, 0xFEACu));              /* +176F */
            /* XOR AH,AH clears CF; DEC AX keeps CF and sets the rest. These
             * flags reach the RETF. AF is DEC's borrow out of bit 3. */
            cpu->ax &= 0x00FFu;
            r = (uint16_t)(cpu->ax - 1u);
            cpu->flags &= (uint16_t)~ARITH_FLAGS;
            if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
            if ((r & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
            if (r == 0u) cpu->flags |= 0x0040u;
            if (r & 0x8000u) cpu->flags |= 0x0080u;
            if (r == 0x7FFFu) cpu->flags |= 0x0800u;
            cpu->ax = r;
            set_frame8(&m, 0xFFFFu, (uint8_t)cpu->ax);
        }
    }
    set_al(cpu, frame8(&m, 0xFFFFu));                      /* +177F */
    cpu->sp = cpu->bp;
    cpu->bp = pop16(&m);
    cpu->ip = pop16(&m);
    cpu->cs = pop16(&m);
    cpu->sp = (uint16_t)(cpu->sp + 0x12u);
    return PR_U25_1684_OK;
}
