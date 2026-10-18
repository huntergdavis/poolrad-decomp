/* GAME.OVR unit 0x20 +0355 (raw CPU, composing the SYSTEM LongInt divide); docs/re/U20_PARTY_AWARD_0355.md. */
#include "poolrad/recovered/overlay_u20_party_award_0355_cpu.h"
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"

#include <string.h>

#define MASK (PR_U20_0355_RAM_SIZE - 1u)
#define ENTRY_IP 0x0355u
#define DIVIDE_ENTRY 0x0A76u

_Static_assert(sizeof(pr_u20_0355_cpu) == sizeof(pr_system_longdiv_cpu), "SYSTEM divide CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u20_0355_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u20_0355_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static uint8_t read8(const machine *m, uint16_t segment, uint16_t offset)
{
    return m->ram[physical(segment, offset)];
}

static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t here(const machine *m, uint16_t offset)
{
    return (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* ADD/ADC (carry_in) or SUB/CMP flags at 8 or 16 bits; returns the result. */
static uint16_t arith(pr_u20_0355_cpu *cpu, uint16_t left, uint16_t right, unsigned carry_in, unsigned bits, int subtract)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right + carry_in;
    uint32_t result = wide & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left < right : wide > mask) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (left ^ right) : ~(uint32_t)(left ^ right)) & (left ^ result) & sign) cpu->flags |= 0x0800u;
    return (uint16_t)result;
}

static int zf(const pr_u20_0355_cpu *cpu) { return (cpu->flags & 0x0040u) != 0u; }

/* LES DI,[BP-4] (p) */
static void les_p(machine *m)
{
    size_t at = physical(m->cpu->ss, (uint16_t)(m->cpu->bp - 4u));
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

/* CMP BYTE ES:[DI+field],value after LES DI,[BP-4]; returns the byte. */
static uint8_t compare_field(machine *m, uint16_t field, uint8_t value)
{
    uint8_t byte;
    les_p(m);
    byte = read8(m, m->cpu->es, (uint16_t)(m->cpu->di + field));
    (void)arith(m->cpu, byte, value, 0u, 8u, 1);
    return byte;
}

/* MOV AX,[BP+6]; MOV DX,[BP+8]; MOV CX,divisor; XOR BX,BX; LCALL xxxx:0A76 at original offset `site`. */
static int divide(machine *m, uint16_t divisor, uint16_t site)
{
    pr_u20_0355_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs;
    pr_system_longdiv_cpu leaf;
    pr_system_longdiv_status status;
    cpu->ax = read16(m, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->dx = read16(m, cpu->ss, (uint16_t)(cpu->bp + 8u));
    cpu->cx = divisor;
    cpu->bx = 0u;
    cpu->flags = (uint16_t)((cpu->flags & ~0x08D5u) | 0x0044u);          /* XOR BX,BX: live at a CODE stop */
    cpu->ip = at;
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != DIVIDE_ENTRY) {
        m->stop = PR_U20_0355_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16(m, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = DIVIDE_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_system_longdiv_0a76(m->ram, m->size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (m->callee_status != NULL)
        *m->callee_status = (int)status;
    if (status != PR_SYSTEM_LONGDIV_RETURNED) {
        m->stop = PR_U20_0355_CALLEE;
        return 0;
    }
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U20_0355_TRANSFER;
        return 0;
    }
    return 1;
}

/* v := X div 10 + X (ADD AX,[BP+6]; ADC DX,[BP+8], flags folded), stored in [BP-8]/[BP-6]. */
static int tenth_plus(machine *m, uint16_t site)
{
    pr_u20_0355_cpu *cpu = m->cpu;
    uint16_t low;
    unsigned carry;
    if (!divide(m, 10u, site))
        return 0;
    low = read16(m, cpu->ss, (uint16_t)(cpu->bp + 6u));
    carry = (uint32_t)cpu->ax + low > 0xFFFFu;
    cpu->ax = (uint16_t)(cpu->ax + low);
    cpu->dx = (uint16_t)(cpu->dx + read16(m, cpu->ss, (uint16_t)(cpu->bp + 8u)) + carry);
    write16(m, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    write16(m, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
    return 1;
}

/* v := X div n, stored in [BP-8]/[BP-6]. */
static int fraction(machine *m, uint16_t divisor, uint16_t site)
{
    if (!divide(m, divisor, site))
        return 0;
    write16(m, m->cpu->ss, (uint16_t)(m->cpu->bp - 8u), m->cpu->ax);
    write16(m, m->cpu->ss, (uint16_t)(m->cpu->bp - 6u), m->cpu->dx);
    return 1;
}

/* The class dispatch +039B..+0497; returns 0 on a stop. */
static int award(machine *m)
{
    static const struct { uint8_t c; uint16_t field, site; } bonus[4] = {
        {0u, 0x12u, 0x03BBu}, {2u, 0x10u, 0x03E8u}, {5u, 0x11u, 0x0415u}, {6u, 0x13u, 0x0441u}
    };
    pr_u20_0355_cpu *cpu = m->cpu;
    uint8_t c;
    les_p(m);                                                             /* +039B MOV AL,ES:[DI+2Fh] */
    c = read8(m, cpu->es, (uint16_t)(cpu->di + 0x2Fu));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | c);
    for (unsigned k = 0u; k < 4u; ++k) {                                  /* CMP AL,0/2/5/6; JNE next */
        (void)arith(cpu, c, bonus[k].c, 0u, 8u, 1);
        if (c == bonus[k].c) {
            if (compare_field(m, bonus[k].field, 0x0Fu) > 0x0Fu)          /* CMP BYTE ES:[DI+field],0Fh; JBE skip */
                return tenth_plus(m, bonus[k].site);
            return 1;
        }
    }
    (void)arith(cpu, c, 8u, 0u, 8u, 1);                                   /* +0454 CMP AL,8; JE +0464 */
    if (c != 8u) {
        int in_range = 0;
        (void)arith(cpu, c, 0x0Au, 0u, 8u, 1);                            /* +0458 CMP AL,0Ah; JL +0460 */
        if ((int8_t)c >= 0x0A) {
            (void)arith(cpu, c, 0x0Eu, 0u, 8u, 1);                        /* +045C CMP AL,0Eh; JLE +0464 */
            in_range = (int8_t)c <= 0x0E;
        }
        if (!in_range) {
            (void)arith(cpu, c, 0x10u, 0u, 8u, 1);                        /* +0460 CMP AL,10h; JNE +047C */
            if (c != 0x10u) {
                (void)arith(cpu, c, 9u, 0u, 8u, 1);                       /* +047C CMP AL,9; JE +0484 */
                if (c != 9u) {
                    (void)arith(cpu, c, 0x0Fu, 0u, 8u, 1);                /* +0480 CMP AL,0Fh; JNE +049A */
                    if (c != 0x0Fu)
                        return 1;
                }
                return fraction(m, 3u, 0x048Fu);                          /* +0484 X div 3 */
            }
        }
    }
    return fraction(m, 2u, 0x046Fu);                                      /* +0464 X div 2 */
}

pr_u20_0355_status pr_u20_party_award_0355_cpu(uint8_t *ram, size_t ram_size, pr_u20_0355_cpu *cpu, unsigned max_loopbacks,
                                               int *callee_status)
{
    machine m;
    unsigned loopbacks = 0u;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U20_0355_RAM_SIZE)
        return PR_U20_0355_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U20_0355_OK;
    push16(&m, cpu->bp);                                                  /* +0355 PUSH BP; MOV BP,SP; SUB SP,8 */
    cpu->bp = cpu->sp;
    cpu->sp = arith(cpu, cpu->sp, 8u, 0u, 16u, 1);
    cpu->ax = read16(&m, cpu->ds, 0x5D96u);                               /* +035B p := DS:[5D96] into [BP-4]/[BP-2] */
    cpu->dx = read16(&m, cpu->ds, 0x5D98u);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
    write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
    for (;;) {
        cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u));          /* +0368 MOV AX,[BP-4]; OR AX,[BP-2]; JNE */
        cpu->ax = (uint16_t)(cpu->ax | read16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u)));
        cpu->flags &= (uint16_t)~0x08D5u;
        if (even_parity((uint8_t)cpu->ax)) cpu->flags |= 0x0004u;
        if (cpu->ax == 0u) cpu->flags |= 0x0040u;
        if (cpu->ax & 0x8000u) cpu->flags |= 0x0080u;
        if (cpu->ax == 0u)
            break;                                                        /* +0370 JMP +04C3 */
        if (compare_field(&m, 0x010Du, 0u) != 0u && (compare_field(&m, 0x010Cu, 1u), !zf(cpu))) {
            uint16_t low, high;
            cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));      /* +038F MOV AX,[BP+6]; MOV DX,[BP+8]; v := X */
            cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp + 8u));
            write16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
            write16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
            if (!award(&m))
                return m.stop;
            cpu->ax = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 8u));      /* +049A MOV AX,[BP-8]; MOV DX,[BP-6] */
            cpu->dx = read16(&m, cpu->ss, (uint16_t)(cpu->bp - 6u));
            les_p(&m);                                                    /* +04A0 ADD ES:[DI+0ACh],AX; ADC ES:[DI+0AEh],DX */
            low = read16(&m, cpu->es, (uint16_t)(cpu->di + 0xACu));
            low = arith(cpu, low, cpu->ax, 0u, 16u, 0);
            write16(&m, cpu->es, (uint16_t)(cpu->di + 0xACu), low);
            high = read16(&m, cpu->es, (uint16_t)(cpu->di + 0xAEu));
            high = arith(cpu, high, cpu->dx, cpu->flags & 0x0001u, 16u, 0);
            write16(&m, cpu->es, (uint16_t)(cpu->di + 0xAEu), high);
        }
        les_p(&m);                                                        /* +04AD p := p^[104h] */
        cpu->ax = read16(&m, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = read16(&m, cpu->es, (uint16_t)(cpu->di + 0x106u));
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 4u), cpu->ax);
        write16(&m, cpu->ss, (uint16_t)(cpu->bp - 2u), cpu->dx);
        if (loopbacks++ == max_loopbacks) {                               /* +04C0 JMP +0368 */
            cpu->ip = here(&m, 0x0368u);
            return PR_U20_0355_LIMIT;
        }
    }
    cpu->sp = cpu->bp;                                                    /* +04C3 MOV SP,BP; POP BP; RETF 4 */
    cpu->bp = read16(&m, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 4u);
    return PR_U20_0355_OK;
}
