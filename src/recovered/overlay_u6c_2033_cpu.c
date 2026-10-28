#include "poolrad/recovered/overlay_u6c_2033_cpu.h"

#include "poolrad/recovered/resident_00ba_0973_cpu.h"

#define MASK (PR_U6C_2033_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((((size_t)segment << 4) + (size_t)offset) & MASK);
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | (uint16_t)(ram[(at + 1u) & MASK] << 8));
}

static void push16(uint8_t *ram, pr_u6c_2033_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

/* CMP r/m8, imm8 leaves the six arithmetic flags computed from a - b and every
 * other bit of the incoming FLAGS word untouched, which is what the model must
 * hand to the caller (and to the differential's 14-word CPU comparison). */
static uint16_t cmp8_flags(uint16_t flags, uint8_t left, uint8_t right)
{
    unsigned difference = (unsigned)left - (unsigned)right;
    uint8_t result = (uint8_t)(difference & 0xFFu);
    uint16_t out = (uint16_t)(flags & (uint16_t)~0x08D5u); /* CF PF AF ZF SF OF */
    unsigned ones = 0;
    unsigned bit;

    if (difference & 0x100u) out |= 0x0001u;                              /* CF */
    if (result == 0u) out |= 0x0040u;                                     /* ZF */
    if (result & 0x80u) out |= 0x0080u;                                   /* SF */
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u) out |= 0x0800u; /* OF */
    if (((left ^ right ^ result) & 0x10u) != 0u) out |= 0x0010u;          /* AF */
    for (bit = 0u; bit < 8u; bit++) ones += (result >> bit) & 1u;
    if ((ones & 1u) == 0u) out |= 0x0004u;                                /* PF */
    return out;
}

static void note(pr_u6c_2033_diagnostic *diagnostic, uint16_t site, int callee, int status)
{
    if (diagnostic == NULL) return;
    diagnostic->site = site;
    diagnostic->callee = callee;
    diagnostic->callee_status = status;
}

/* The opcodes and load-zero fixups the model's control flow relies on. Each is
 * read from the bus, so a caller that runs the model on different bytes fails
 * instead of silently taking this branch structure on trust. */
static int code_is_the_expected_body(pr_bus *bus, uint16_t cs)
{
    static const uint8_t cmp[] = {0x36u, 0x80u, 0x7Du, 0xF3u, 0x06u};   /* cmp byte ss:[di-0Dh], 6 */
    static const uint8_t jae[] = {0x73u, 0x13u};                        /* jae 2053 */
    static const uint8_t low[] = {0x9Au, 0x2Fu, 0x00u, 0x85u, 0x00u};   /* lcall 0085:002F */
    static const uint8_t high[] = {0x9Au, 0x73u, 0x09u, 0xBAu, 0x00u};  /* lcall 00BA:0973 */
    static const uint8_t retf[] = {0xCAu, 0x04u, 0x00u};                /* retf 4 */
    size_t at;
    size_t i;

    at = physical(cs, 0x2039u);
    for (i = 0u; i < sizeof cmp; i++) if (bus->ram[(at + i) & MASK] != cmp[i]) return 0;
    at = physical(cs, 0x203Eu);
    for (i = 0u; i < sizeof jae; i++) if (bus->ram[(at + i) & MASK] != jae[i]) return 0;
    at = physical(cs, 0x204Cu);
    for (i = 0u; i < sizeof low; i++) if (bus->ram[(at + i) & MASK] != low[i]) return 0;
    at = physical(cs, 0x2065u);
    for (i = 0u; i < sizeof high; i++) if (bus->ram[(at + i) & MASK] != high[i]) return 0;
    at = physical(cs, 0x206Bu);
    for (i = 0u; i < sizeof retf; i++) if (bus->ram[(at + i) & MASK] != retf[i]) return 0;
    return 1;
}

pr_u6c_2033_status pr_ovl_006c_2033_cpu(pr_bus *bus, size_t ram_size, pr_u6c_2033_cpu *cpu,
                                        const pr_u6c_2033_env *env, pr_u6c_2033_diagnostic *diagnostic)
{
    pr_res_0973_cpu composed;
    pr_res_0973_env composed_env;
    pr_res_0973_diagnostic composed_diagnostic;
    pr_u6c_2033_env defaults = {65536u, 65536u};
    uint16_t node_word;
    uint8_t first;
    uint16_t status;

    if (ram_size != PR_U6C_2033_RAM_SIZE || cpu == NULL || cpu->ip != 0x2033u) return PR_U6C_2033_ARGUMENT;
    if (bus == NULL || bus->ram == NULL) return PR_U6C_2033_ARGUMENT;
    if (diagnostic != NULL) {
        diagnostic->site = 0u;
        diagnostic->callee = -1;
        diagnostic->callee_status = -1;
    }
    if (env == NULL) env = &defaults;
    if (!code_is_the_expected_body(bus, cpu->cs)) return PR_U6C_2033_CODE;

    push16(bus->ram, cpu, cpu->bp);            /* 2033 push bp */
    cpu->bp = cpu->sp;                         /* 2034 mov bp, sp */
    node_word = read16(bus->ram, cpu->ss, (uint16_t)(cpu->bp + 6u));      /* 2036 */
    first = read8(bus->ram, cpu->ss, (uint16_t)(node_word - 0x0Du));      /* 2039 */
    cpu->flags = cmp8_flags(cpu->flags, first, 6u);

    if (first < 6u) {
        /* 2040..204B: the byte below 6 and [bp+8] go to unit 0x77 entry 3. */
        cpu->di = node_word;
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (uint16_t)first);
        push16(bus->ram, cpu, cpu->ax);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                             (uint16_t)read8(bus->ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
        push16(bus->ram, cpu, cpu->ax);
        cpu->ip = 0x204Cu;                    /* the far call has not executed: the hand-over site */
        note(diagnostic, 0x204Cu, 0, -1);
        return PR_U6C_2033_BOUNDARY;
    }

    /* 2053..2064: the active far pointer, x 4, y 012h and [bp+8]. */
    push16(bus->ram, cpu, read16(bus->ram, cpu->ds, 0x5D94u));
    push16(bus->ram, cpu, read16(bus->ram, cpu->ds, 0x5D92u));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 4u);
    push16(bus->ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | 0x12u);
    push16(bus->ram, cpu, cpu->ax);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         (uint16_t)read8(bus->ram, cpu->ss, (uint16_t)(cpu->bp + 8u)));
    push16(bus->ram, cpu, cpu->ax);

    /* 2065 lcall 00BA:0973, the accepted composed model. It consumes its own
     * five argument words and the far return, leaving CS:IP at 206A. */
    push16(bus->ram, cpu, cpu->cs);
    push16(bus->ram, cpu, 0x206Au);
    composed.ax = cpu->ax; composed.bx = cpu->bx; composed.cx = cpu->cx; composed.dx = cpu->dx;
    composed.si = cpu->si; composed.di = cpu->di; composed.bp = cpu->bp; composed.sp = cpu->sp;
    composed.ds = cpu->ds; composed.es = cpu->es; composed.ss = cpu->ss; composed.flags = cpu->flags;
    composed.cs = 0x00BAu;
    composed.ip = 0x0973u;
    composed_env.printer_loopbacks = env->printer_loopbacks;
    composed_env.printer_glyph_loopbacks = env->printer_glyph_loopbacks;
    status = pr_res_00ba_0973_cpu(bus, ram_size, &composed, &composed_env, &composed_diagnostic);
    cpu->ax = composed.ax; cpu->bx = composed.bx; cpu->cx = composed.cx; cpu->dx = composed.dx;
    cpu->si = composed.si; cpu->di = composed.di; cpu->bp = composed.bp; cpu->sp = composed.sp;
    cpu->ds = composed.ds; cpu->es = composed.es; cpu->ss = composed.ss; cpu->flags = composed.flags;
    cpu->cs = composed.cs; cpu->ip = composed.ip;
    note(diagnostic, 0x2065u, 1, (int)status);
    if (status != PR_RES_0973_RETURNED) {
        if (status == PR_RES_0973_ARGUMENT) return PR_U6C_2033_ARGUMENT;
        if (status == PR_RES_0973_CODE) return PR_U6C_2033_CODE;
        if (status == PR_RES_0973_TRANSFER) return PR_U6C_2033_TRANSFER;
        return PR_U6C_2033_CALLEE;
    }
    if (cpu->ip != 0x206Au) return PR_U6C_2033_TRANSFER;

    /* 206A pop bp; 206B retf 4 */
    cpu->bp = read16(bus->ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(bus->ram, cpu->ss, cpu->sp);
    cpu->cs = read16(bus->ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    return PR_U6C_2033_RETURNED;
}
