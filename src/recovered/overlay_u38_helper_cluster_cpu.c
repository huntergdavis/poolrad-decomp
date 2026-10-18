/* GAME.OVR unit 0x38 +0000, +1490, +29B6, +29CB, +29D9, +29F0, +2A11 (raw CPU); docs/re/U38_HELPER_CLUSTER_CPU.md. */
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"

#define MASK (PR_U38C_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void push16(uint8_t *ram, pr_u38c_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t pop16(const uint8_t *ram, pr_u38c_cpu *cpu)
{
    uint16_t value = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

/* RETF n: IP, then CS from the next two physical bytes. */
static void retf(const uint8_t *ram, pr_u38c_cpu *cpu, uint16_t pops)
{
    size_t at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + pops);
}

static uint8_t al(const pr_u38c_cpu *cpu)
{
    return (uint8_t)cpu->ax;
}

static void set_al(pr_u38c_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void cmp8(pr_u38c_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) cpu->flags |= 0x0800u;
}

/* AND AL,imm8 then OR AL,AL: CF = OF = AF = 0, PF/ZF/SF from AL. */
static void and_or_al(pr_u38c_cpu *cpu, uint8_t mask)
{
    set_al(cpu, (uint8_t)(al(cpu) & mask));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity(al(cpu))) cpu->flags |= 0x0004u;
    if (al(cpu) == 0u) cpu->flags |= 0x0040u;
    if (al(cpu) & 0x80u) cpu->flags |= 0x0080u;
}

static int valid(const uint8_t *ram, size_t ram_size, const pr_u38c_cpu *cpu)
{
    return ram != NULL && cpu != NULL && ram_size == PR_U38C_RAM_SIZE;
}

/* Helper +0000..+001E on the live CPU, from its PUSH BP to its RETF 2. */
static void helper(uint8_t *ram, pr_u38c_cpu *cpu)
{
    uint8_t key;
    push16(ram, cpu, cpu->bp);                                        /* +0000 */
    cpu->bp = cpu->sp;
    key = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));              /* +0003 CMP BYTE [BP+6],0; JE +0011 */
    cmp8(cpu, key, 0u);
    if (key != 0u) {
        set_al(cpu, read8(ram, cpu->ds, 0x6817u));                    /* +0009 MOV AL,[6817] */
        key = read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));          /* +000C CMP AL,[BP+6]; JNE +001B */
        cmp8(cpu, al(cpu), key);
        if (al(cpu) != key)
            goto done;
    }
    write8(ram, cpu->ds, 0x6818u, 0u);                                /* +0011 */
    write8(ram, cpu->ds, 0x6817u, 0u);                                /* +0016 */
done:
    cpu->bp = pop16(ram, cpu);                                        /* +001B POP BP; RETF 2 */
    retf(ram, cpu, 2u);
}

/* MOV AL,key; PUSH AX; PUSH CS; CALL +0000 at unit offset `site` of a routine at unit offset `base`. */
static pr_u38c_status call_helper(uint8_t *ram, pr_u38c_cpu *cpu, uint16_t entry, uint16_t base, uint16_t site, uint8_t key)
{
    uint16_t caller_cs = cpu->cs, return_ip = (uint16_t)(entry + (uint16_t)(site + 3u - base));
    set_al(cpu, key);
    push16(ram, cpu, cpu->ax);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, return_ip);
    cpu->ip = (uint16_t)(entry - base);
    helper(ram, cpu);
    if (cpu->ip != return_ip || cpu->cs != caller_cs)
        return PR_U38C_TRANSFER;
    return PR_U38C_OK;
}

/* PUSH BP; MOV BP,SP. */
static void enter(uint8_t *ram, pr_u38c_cpu *cpu)
{
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
}

/* POP BP; RETF 0Ah. */
static pr_u38c_status leave(uint8_t *ram, pr_u38c_cpu *cpu)
{
    cpu->bp = pop16(ram, cpu);
    retf(ram, cpu, 0x0Au);
    return PR_U38C_OK;
}

pr_u38c_status pr_u38_helper_0000_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    helper(ram, cpu);
    return PR_U38C_OK;
}

pr_u38c_status pr_u38_entry50_1490_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu, unsigned max_loopbacks)
{
    uint16_t entry;
    uint8_t count, value, limit;
    unsigned loopbacks = 0u;
    pr_u38c_status status;
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    entry = cpu->ip;
    enter(ram, cpu);                                                  /* +1490 */
    cpu->sp = (uint16_t)(cpu->sp - 2u);                               /* +1493 SUB SP,2 (flags folded) */
    set_al(cpu, read8(ram, cpu->ds, 0x6819u));                        /* +1496 */
    and_or_al(cpu, 0x01u);
    if (al(cpu) == 0u)                                                /* +149D JE +14E5 */
        goto out;
    set_al(cpu, read8(ram, cpu->ds, 0x681Cu));                        /* +149F */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u), al(cpu));
    set_al(cpu, 1u);                                                  /* +14A5 MOV AL,1; CMP AL,[BP-2]; JA +14D0 */
    count = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
    cmp8(cpu, 1u, count);
    if (!(1u > count)) {
        write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u), al(cpu));      /* +14AC; JMP +14B4 */
        for (;;) {
            /* +14B4 SUB BYTE [6818],2 (flags folded); MOV AL,[6818]; CMP AL,[681C]; JAE +14C8. */
            write8(ram, cpu->ds, 0x6818u, (uint8_t)(read8(ram, cpu->ds, 0x6818u) - 2u));
            set_al(cpu, read8(ram, cpu->ds, 0x6818u));
            value = read8(ram, cpu->ds, 0x681Cu);
            cmp8(cpu, al(cpu), value);
            if (al(cpu) < value) {
                set_al(cpu, read8(ram, cpu->ds, 0x681Cu));            /* +14C2 */
                write8(ram, cpu->ds, 0x6818u, al(cpu));
            }
            set_al(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u))); /* +14C8 MOV AL,[BP-1]; CMP AL,[BP-2]; JNE */
            limit = read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));
            cmp8(cpu, al(cpu), limit);
            if (al(cpu) == limit)
                break;
            if (loopbacks == max_loopbacks) {
                cpu->ip = (uint16_t)(entry + 0x21u);
                return PR_U38C_LIMIT;
            }
            ++loopbacks;
            write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u),            /* +14B1 INC BYTE [BP-1] (flags folded) */
                   (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)) + 1u));
        }
    }
    write8(ram, cpu->ds, 0x6816u, (uint8_t)(read8(ram, cpu->ds, 0x6816u) + 4u)); /* +14D0 (flags folded) */
    set_al(cpu, read8(ram, cpu->ds, 0x6819u));                        /* +14D5 */
    and_or_al(cpu, 0x08u);
    if (al(cpu) == 0u) {                                              /* +14DC JNE +14E5 */
        status = call_helper(ram, cpu, entry, 0x1490u, 0x14E2u, 0u);  /* +14DE */
        if (status != PR_U38C_OK)
            return status;
    }
out:
    cpu->sp = cpu->bp;                                                /* +14E5 MOV SP,BP */
    return leave(ram, cpu);
}

pr_u38c_status pr_u38_entry92_29b6_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    uint16_t entry;
    pr_u38c_status status;
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    entry = cpu->ip;
    enter(ram, cpu);
    status = call_helper(ram, cpu, entry, 0x29B6u, 0x29BDu, 0x0Bu);
    if (status == PR_U38C_OK)
        status = call_helper(ram, cpu, entry, 0x29B6u, 0x29C4u, 0x35u);
    return status != PR_U38C_OK ? status : leave(ram, cpu);
}

pr_u38c_status pr_u38_entry93_29cb_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    uint16_t entry;
    pr_u38c_status status;
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    entry = cpu->ip;
    enter(ram, cpu);
    status = call_helper(ram, cpu, entry, 0x29CBu, 0x29D2u, 0x34u);
    return status != PR_U38C_OK ? status : leave(ram, cpu);
}

/* Entries 94 and 96: MOV AL,[6819]; AND AL,mask; OR AL,AL; JE skip; helper(0). */
static pr_u38c_status gated(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu, uint16_t base, uint16_t site, uint8_t mask)
{
    uint16_t entry;
    pr_u38c_status status;
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    entry = cpu->ip;
    enter(ram, cpu);
    set_al(cpu, read8(ram, cpu->ds, 0x6819u));
    and_or_al(cpu, mask);
    if (al(cpu) != 0u) {
        status = call_helper(ram, cpu, entry, base, site, 0u);
        if (status != PR_U38C_OK)
            return status;
    }
    return leave(ram, cpu);
}

pr_u38c_status pr_u38_entry94_29d9_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    return gated(ram, ram_size, cpu, 0x29D9u, 0x29E9u, 0x02u);
}

pr_u38c_status pr_u38_entry96_2a11_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    return gated(ram, ram_size, cpu, 0x2A11u, 0x2A21u, 0x01u);
}

pr_u38c_status pr_u38_entry95_29f0_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu)
{
    uint16_t entry;
    uint8_t value;
    pr_u38c_status status;
    if (!valid(ram, ram_size, cpu))
        return PR_U38C_ARGUMENT;
    entry = cpu->ip;
    enter(ram, cpu);
    status = call_helper(ram, cpu, entry, 0x29F0u, 0x29F7u, 0x37u);
    if (status == PR_U38C_OK)
        status = call_helper(ram, cpu, entry, 0x29F0u, 0x29FEu, 0x34u);
    if (status != PR_U38C_OK)
        return status;
    value = read8(ram, cpu->ds, 0x682Au);                             /* +2A01 CMP BYTE [682A],0; JNE +2A0D */
    cmp8(cpu, value, 0u);
    if (value == 0u)
        write8(ram, cpu->ds, 0x6816u, 0x64u);                         /* +2A08 */
    return leave(ram, cpu);
}
