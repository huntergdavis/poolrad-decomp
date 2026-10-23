/* U25 +1153/+12FB and exact string dependencies; see U25_STRING_CORES.md. */
#include "poolrad/recovered/overlay_u25_string_cores.h"

#define RAM_MASK (PR_U25_STRINGS_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t a = physical(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint8_t parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (uint8_t)((value & 1u) == 0u);
}

static uint16_t arithmetic(pr_u25_strings_cpu *cpu, uint16_t left,
                           uint16_t right, uint16_t sign, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint16_t result = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? left < right : wide > mask) cpu->flags |= 1u;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? (left ^ right) : (uint16_t)~(left ^ right)) &
         (left ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}

static void cmp8(pr_u25_strings_cpu *cpu, uint8_t left, uint8_t right)
{
    (void)arithmetic(cpu, left, right, 0x80u, 1);
}

static void cmp16(pr_u25_strings_cpu *cpu, uint16_t left, uint16_t right)
{
    (void)arithmetic(cpu, left, right, 0x8000u, 1);
}

static void set_al(pr_u25_strings_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static uint16_t pop16(const uint8_t *ram, pr_u25_strings_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static void push16(uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}



static void logical(pr_u25_strings_cpu *cpu, uint16_t result, unsigned bits)
{
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (parity((uint8_t)result)) cpu->flags |= 4u;
    if (result == 0u) cpu->flags |= 0x40u;
    if (result & (bits == 8u ? 0x80u : 0x8000u)) cpu->flags |= 0x80u;
}

static uint16_t increment(pr_u25_strings_cpu *cpu, uint16_t value, unsigned bits)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits == 8u ? 0x80u : 0x8000u, 0);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}

static uint16_t shift(pr_u25_strings_cpu *cpu, uint16_t value, unsigned count, int right)
{
    uint16_t old = value, carry = 0, overflow = cpu->flags & 0x800u;
    unsigned i;
    count &= 31u;
    if (!count) return value;
    for (i = 0; i < count; ++i) {
        carry = (uint16_t)(right ? value & 1u : value >> 15);
        value = (uint16_t)(right ? value >> 1 : (uint32_t)value << 1);
    }
    logical(cpu, value, 16u);
    /* pinned FillFlags: AF = (count & 1Fh) != 0; SHL OF = MSB(result) ^ CF; SHR OF = (count == 1 && var1 >= 8000h). */
    overflow = right ? ((count == 1u && (old & 0x8000u)) ? 0x800u : 0u) : ((((value >> 15) ^ carry) & 1u) ? 0x800u : 0u);
    cpu->flags |= (uint16_t)(carry | overflow | 0x0010u);
    return value;
}

/* LES/LDS read all four bytes before updating either destination register. */
static void load_far(const uint8_t *ram, pr_u25_strings_cpu *cpu,
                     uint16_t segment, uint16_t offset, int data_segment)
{
    size_t a = physical(segment, offset);
    uint16_t off = (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
    uint16_t seg = (uint16_t)(ram[(a + 2u) & RAM_MASK] |
                            ((uint16_t)ram[(a + 3u) & RAM_MASK] << 8));
    if (data_segment) { cpu->si = off; cpu->ds = seg; }
    else { cpu->di = off; cpu->es = seg; }
}

static uint16_t advance(const pr_u25_strings_cpu *cpu, uint16_t offset)
{
    return (uint16_t)(offset + ((cpu->flags & 0x400u) ? -1 : 1));
}

static void copy_bytes(uint8_t *ram, pr_u25_strings_cpu *cpu)
{
    /* Forward ordered bytes, never memcpy/memmove: overlapping strings and
     * frame aliases can change subsequent reads. Runtime entries execute CLD. */
    while (cpu->cx) {
        uint8_t value = read8(ram, cpu->ds, cpu->si);
        write8(ram, cpu->es, cpu->di, value);
        cpu->si = advance(cpu, cpu->si);
        cpu->di = advance(cpu, cpu->di);
        --cpu->cx;
    }
}


static pr_u25_strings_status body_0b0b(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    cpu->flags &= (uint16_t)~0x400u; /* +0B0B */
    cpu->bx = (uint16_t)(cpu->sp); /* +0B0C */
    cpu->dx = (uint16_t)(cpu->ds); /* +0B0E */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 8), 0); /* +0B10 */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 4), 1); /* +0B14 */
    set_al(cpu, read8(ram, cpu->ds, cpu->si));
    cpu->si = advance(cpu, cpu->si); /* +0B18 */
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    cpu->di = advance(cpu, cpu->di); /* +0B19 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)((uint8_t)cpu->ax)); /* +0B1A */
    cpu->cx &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0B1C */
    copy_bytes(ram, cpu); /* +0B1E */
    cpu->ds = (uint16_t)(cpu->dx); /* +0B20 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_STRINGS_OK; /* +0B22 */
}

static pr_u25_strings_status body_0b25(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    cpu->flags &= (uint16_t)~0x400u; /* +0B25 */
    cpu->bx = (uint16_t)(cpu->sp); /* +0B26 */
    cpu->dx = (uint16_t)(cpu->ds); /* +0B28 */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 10), 1); /* +0B2A */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 6), 0); /* +0B2E */
    cpu->cx = (uint16_t)(read16(ram, cpu->ss, (uint16_t)(cpu->bx + 4))); /* +0B32 */
    set_al(cpu, read8(ram, cpu->ds, cpu->si));
    cpu->si = advance(cpu, cpu->si); /* +0B36 */
    cmp8(cpu, (uint8_t)cpu->ax, (uint8_t)cpu->cx); /* +0B37 */
    if ((cpu->flags & 0x41u) != 0u) goto L_0b3d; /* +0B39 */
    set_al(cpu, (uint8_t)((uint8_t)cpu->cx)); /* +0B3B */

L_0b3d:
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    cpu->di = advance(cpu, cpu->di); /* +0B3D */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)((uint8_t)cpu->ax)); /* +0B3E */
    cpu->cx &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0B40 */
    copy_bytes(ram, cpu); /* +0B42 */
    cpu->ds = (uint16_t)(cpu->dx); /* +0B44 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 10u);
    return PR_U25_STRINGS_OK; /* +0B46 */
}

static pr_u25_strings_status body_0b98(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    cpu->flags &= (uint16_t)~0x400u; /* +0B98 */
    cpu->bx = (uint16_t)(cpu->sp); /* +0B99 */
    cpu->dx = (uint16_t)(cpu->ds); /* +0B9B */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 8), 0); /* +0B9D */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 4), 1); /* +0BA1 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di)))); /* +0BA5 */
    cpu->cx &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +0BA8 */
    set_al(cpu, read8(ram, cpu->ds, cpu->si));
    cpu->si = advance(cpu, cpu->si); /* +0BAA */
    write8(ram, cpu->es, (uint16_t)(cpu->di), (uint8_t)(arithmetic(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di)), (uint8_t)cpu->ax, 0x80u, 0))); /* +0BAB */
    if ((cpu->flags & 1u) == 0u) goto L_0bb8; /* +0BAE */
    write8(ram, cpu->es, (uint16_t)(cpu->di), (uint8_t)(0xffu)); /* +0BB0 */
    set_al(cpu, (uint8_t)((uint8_t)cpu->cx)); /* +0BB4 */
    set_al(cpu, (uint8_t)(~((uint8_t)cpu->ax))); /* +0BB6 */

L_0bb8:
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->cx, 0x8000u, 0)); /* +0BB8 */
    cpu->di = (uint16_t)(increment(cpu, cpu->di, 16u)); /* +0BBA */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)((uint8_t)cpu->ax)); /* +0BBB */
    copy_bytes(ram, cpu); /* +0BBD */
    cpu->ds = (uint16_t)(cpu->dx); /* +0BBF */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_STRINGS_OK; /* +0BC1 */
}

static pr_u25_strings_status body_0c3b(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    cpu->flags &= (uint16_t)~0x400u; /* +0C3B */
    cpu->bx = (uint16_t)(cpu->sp); /* +0C3C */
    load_far(ram, cpu, cpu->ss, (uint16_t)(cpu->bx + 6), 0); /* +0C3E */
    set_al(cpu, (uint8_t)(0x1u)); /* +0C42 */
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    cpu->di = advance(cpu, cpu->di); /* +0C44 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bx + 4)))); /* +0C45 */
    write8(ram, cpu->es, cpu->di, (uint8_t)cpu->ax);
    cpu->di = advance(cpu, cpu->di); /* +0C49 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U25_STRINGS_OK; /* +0C4A */
}

static pr_u25_strings_status body_06d3(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    push16(ram, cpu, cpu->bp); /* +06D3 */
    cpu->bp = (uint16_t)(cpu->sp); /* +06D4 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x2u, 0x8000u, 1)); /* +06D6 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x1fu); /* +06D9 */
    if ((cpu->flags & 0x41u) != 0u) goto L_06e7; /* +06DD */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +06DF */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)((uint8_t)cpu->ax)); /* +06E2 */
    goto L_06f2; /* +06E5 */

L_06e7:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +06E7 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +06EA */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, 0x40u, 0x8000u, 0)); /* +06EC */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)((uint8_t)cpu->ax)); /* +06EF */

L_06f2:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +06F2 */
    cpu->sp = (uint16_t)(cpu->bp); /* +06F5 */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +06F7 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U25_STRINGS_OK; /* +06F8 */
}

static pr_u25_strings_status body_06fb(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    push16(ram, cpu, cpu->bp); /* +06FB */
    cpu->bp = (uint16_t)(cpu->sp); /* +06FC */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x2u, 0x8000u, 1)); /* +06FE */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x4u)); /* +0701 */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x4900u); /* +0705 */
    if ((cpu->flags & 1u) != 0u) goto L_0717; /* +070A */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x4cffu); /* +070C */
    if ((cpu->flags & 0x41u) == 0u) goto L_0717; /* +0711 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x0u)); /* +0713 */

L_0717:
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x6b00u); /* +0717 */
    if ((cpu->flags & 1u) != 0u) goto L_0729; /* +071C */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x6effu); /* +071E */
    if ((cpu->flags & 0x41u) == 0u) goto L_0729; /* +0723 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x1u)); /* +0725 */

L_0729:
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x9700u); /* +0729 */
    if ((cpu->flags & 1u) != 0u) goto L_073b; /* +072E */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x98ffu); /* +0730 */
    if ((cpu->flags & 0x41u) == 0u) goto L_073b; /* +0735 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x2u)); /* +0737 */

L_073b:
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x9900u); /* +073B */
    if ((cpu->flags & 1u) != 0u) goto L_074d; /* +0740 */
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0xb6ffu); /* +0742 */
    if ((cpu->flags & 0x41u) == 0u) goto L_074d; /* +0747 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x3u)); /* +0749 */

L_074d:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +074D */
    cpu->sp = (uint16_t)(cpu->bp); /* +0750 */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +0752 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U25_STRINGS_OK; /* +0753 */
}

static pr_u25_strings_status body_1153(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    push16(ram, cpu, cpu->bp); /* +1153 */
    cpu->bp = (uint16_t)(cpu->sp); /* +1154 */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x208u, 0x8000u, 1)); /* +1156 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +115A */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +115D */
    cpu->di = (uint16_t)(cpu->ax); /* +115F */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1161 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +1163 */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28616), (uint8_t)(0x0u)); /* +1165 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(0x0u)); /* +116A */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 6), (uint8_t)(0x1u)); /* +116E */

L_1172:
    cpu->ip = 0x1172u;
    if (*remaining == 0u) return PR_U25_STRINGS_LIMIT;
    --*remaining;
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +1172 */
    cmp8(cpu, (uint8_t)cpu->ax, read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8))); /* +1175 */
    if ((cpu->flags & 1u) != 0u) goto L_117d; /* +1178 */
    goto L_128c; /* +117A */

L_117d:
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0x4u); /* +117D */
    if ((cpu->flags & 1u) == 0u) goto L_11a1; /* +1181 */
    write16(ram, cpu->ds, (uint16_t)(18925), (uint16_t)(increment(cpu, read16(ram, cpu->ds, (uint16_t)(18925)), 16u))); /* +1183 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +1187 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 4), (uint8_t)((uint8_t)cpu->ax)); /* +118A */
    cpu->ax = (uint16_t)(read16(ram, cpu->ds, (uint16_t)(18925))); /* +118D */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +1190 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +1194 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +1196 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 3), (uint8_t)((uint8_t)cpu->ax)); /* +119B */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)), 8u))); /* +119E */

L_11a1:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)))); /* +11A1 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x1u); /* +11A4 */
    if ((cpu->flags & 0x40u) == 0u) goto L_11b7; /* +11A6 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +11A8 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11AB */
    cpu->cx = (uint16_t)(0x2u); /* +11AD */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 1)); /* +11B0 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +11B2 */
    goto L_121f; /* +11B5 */

L_11b7:
    cmp8(cpu, (uint8_t)cpu->ax, 0x2u); /* +11B7 */
    if ((cpu->flags & 0x40u) == 0u) goto L_11e5; /* +11B9 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +11BB */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11BE */
    cpu->cx = (uint16_t)(0x2u); /* +11C0 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 1)); /* +11C3 */
    cpu->dx = (uint16_t)(cpu->ax); /* +11C5 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4)))); /* +11C7 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11CA */
    cpu->cx = (uint16_t)(0x6u); /* +11CC */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 0)); /* +11CF */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0)); /* +11D1 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +11D3 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)))); /* +11D6 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11D9 */
    cpu->cx = (uint16_t)(0x2u); /* +11DB */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 1)); /* +11DE */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +11E0 */
    goto L_121f; /* +11E3 */

L_11e5:
    cmp8(cpu, (uint8_t)cpu->ax, 0x3u); /* +11E5 */
    if ((cpu->flags & 0x40u) == 0u) goto L_1213; /* +11E7 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +11E9 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11EC */
    cpu->cx = (uint16_t)(0x4u); /* +11EE */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 1)); /* +11F1 */
    cpu->dx = (uint16_t)(cpu->ax); /* +11F3 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 4)))); /* +11F5 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +11F8 */
    cpu->cx = (uint16_t)(0x4u); /* +11FA */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 0)); /* +11FD */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, cpu->dx, 0x8000u, 0)); /* +11FF */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +1201 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)))); /* +1204 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1207 */
    cpu->cx = (uint16_t)(0x2u); /* +1209 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, (uint8_t)cpu->cx, 1)); /* +120C */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +120E */
    goto L_121f; /* +1211 */

L_1213:
    cmp8(cpu, (uint8_t)cpu->ax, 0x4u); /* +1213 */
    if ((cpu->flags & 0x40u) == 0u) goto L_121f; /* +1215 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +1217 */
    set_al(cpu, (uint8_t)(((uint8_t)cpu->ax) & (0x3fu)));
    logical(cpu, (uint8_t)cpu->ax, 8u); /* +121A */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +121C */

L_121f:
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0x4u); /* +121F */
    if ((cpu->flags & 1u) == 0u) goto L_122a; /* +1223 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 6), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 8u))); /* +1225 */
    goto L_122e; /* +1228 */

L_122a:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 6), (uint8_t)(0x1u)); /* +122A */

L_122e:
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)), 0x0u); /* +122E */
    if ((cpu->flags & 0x40u) != 0u) goto L_1289; /* +1232 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)))); /* +1234 */
    push16(ram, cpu, cpu->ax); /* +1237 */
    push16(ram, cpu, cpu->cs); /* +1238 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, 0x123Cu);
        cpu->ip = 0x06D3u;
        status = body_06d3(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x123Cu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1239 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 7), (uint8_t)((uint8_t)cpu->ax)); /* +123C */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 263)); /* +123F */
    push16(ram, cpu, cpu->ss); /* +1243 */
    push16(ram, cpu, cpu->di); /* +1244 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +1245 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1248 */
    cpu->di = (uint16_t)(cpu->ax); /* +124A */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +124C */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +124E */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +1250 */
    push16(ram, cpu, cpu->ds); /* +1254 */
    push16(ram, cpu, cpu->di); /* +1255 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x125Bu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x125Bu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1256 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 519)); /* +125B */
    push16(ram, cpu, cpu->ss); /* +125F */
    push16(ram, cpu, cpu->di); /* +1260 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 7)))); /* +1261 */
    push16(ram, cpu, cpu->ax); /* +1264 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x126Au);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x126Au)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1265 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x126Fu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x126Fu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +126A */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +126F */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1272 */
    cpu->di = (uint16_t)(cpu->ax); /* +1274 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1276 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +1278 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +127A */
    push16(ram, cpu, cpu->ds); /* +127E */
    push16(ram, cpu, cpu->di); /* +127F */
    cpu->ax = (uint16_t)(0xffu); /* +1280 */
    push16(ram, cpu, cpu->ax); /* +1283 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1289u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1289u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1284 */

L_1289:
    goto L_1172; /* +1289 */

L_128c:
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 6)), 0x4u); /* +128C */
    if ((cpu->flags & 0x40u) == 0u) goto L_12f5; /* +1290 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 3)))); /* +1292 */
    set_al(cpu, (uint8_t)(((uint8_t)cpu->ax) & (0x3fu)));
    logical(cpu, (uint8_t)cpu->ax, 8u); /* +1295 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 5), (uint8_t)((uint8_t)cpu->ax)); /* +1297 */
    cmp8(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)), 0x0u); /* +129A */
    if ((cpu->flags & 0x40u) != 0u) goto L_12f5; /* +129E */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 5)))); /* +12A0 */
    push16(ram, cpu, cpu->ax); /* +12A3 */
    push16(ram, cpu, cpu->cs); /* +12A4 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, 0x12A8u);
        cpu->ip = 0x06D3u;
        status = body_06d3(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x12A8u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +12A5 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 7), (uint8_t)((uint8_t)cpu->ax)); /* +12A8 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 263)); /* +12AB */
    push16(ram, cpu, cpu->ss); /* +12AF */
    push16(ram, cpu, cpu->di); /* +12B0 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +12B1 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +12B4 */
    cpu->di = (uint16_t)(cpu->ax); /* +12B6 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +12B8 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +12BA */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +12BC */
    push16(ram, cpu, cpu->ds); /* +12C0 */
    push16(ram, cpu, cpu->di); /* +12C1 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x12C7u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x12C7u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +12C2 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 519)); /* +12C7 */
    push16(ram, cpu, cpu->ss); /* +12CB */
    push16(ram, cpu, cpu->di); /* +12CC */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 7)))); /* +12CD */
    push16(ram, cpu, cpu->ax); /* +12D0 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x12D6u);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x12D6u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +12D1 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x12DBu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x12DBu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +12D6 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)))); /* +12DB */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +12DE */
    cpu->di = (uint16_t)(cpu->ax); /* +12E0 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +12E2 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +12E4 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +12E6 */
    push16(ram, cpu, cpu->ds); /* +12EA */
    push16(ram, cpu, cpu->di); /* +12EB */
    cpu->ax = (uint16_t)(0xffu); /* +12EC */
    push16(ram, cpu, cpu->ax); /* +12EF */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x12F5u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x12F5u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +12F0 */

L_12f5:
    cpu->sp = (uint16_t)(cpu->bp); /* +12F5 */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +12F7 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_STRINGS_OK; /* +12F8 */
}

static pr_u25_strings_status body_12fb(
    uint8_t *ram, pr_u25_strings_cpu *cpu, uint16_t system_cs, size_t *remaining)
{
    (void)system_cs;
    (void)remaining;
    push16(ram, cpu, cpu->bp); /* +12FB */
    cpu->bp = (uint16_t)(cpu->sp); /* +12FC */
    cpu->sp = (uint16_t)(arithmetic(cpu, cpu->sp, 0x202u, 0x8000u, 1)); /* +12FE */
    push16(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6))); /* +1302 */
    push16(ram, cpu, cpu->cs); /* +1305 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, 0x1309u);
        cpu->ip = 0x06FBu;
        status = body_06fb(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1309u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1306 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 1), (uint8_t)((uint8_t)cpu->ax)); /* +1309 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +130C */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +130F */
    cpu->di = (uint16_t)(cpu->ax); /* +1311 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1313 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +1315 */
    write8(ram, cpu->ds, (uint16_t)(cpu->di + 28616), (uint8_t)(0x0u)); /* +1317 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(0x0u)); /* +131C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 1)))); /* +1320 */
    cmp8(cpu, (uint8_t)cpu->ax, 0x0u); /* +1323 */
    if ((cpu->flags & 0x40u) == 0u) goto L_13a3; /* +1325 */

L_1327:
    cpu->ip = 0x1327u;
    if (*remaining == 0u) return PR_U25_STRINGS_LIMIT;
    --*remaining;
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +1327 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +132A */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +132C */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +132F */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18898), 0); /* +1331 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +1335 */
    cmp16(cpu, read16(ram, cpu->es, (uint16_t)(cpu->di + 28160)), 0x0u); /* +1337 */
    if ((cpu->flags & 0x40u) != 0u) goto L_13a0; /* +133D */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 258)); /* +133F */
    push16(ram, cpu, cpu->ss); /* +1343 */
    push16(ram, cpu, cpu->di); /* +1344 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +1345 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1348 */
    cpu->di = (uint16_t)(cpu->ax); /* +134A */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +134C */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +134E */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +1350 */
    push16(ram, cpu, cpu->ds); /* +1354 */
    push16(ram, cpu, cpu->di); /* +1355 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x135Bu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x135Bu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1356 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 514)); /* +135B */
    push16(ram, cpu, cpu->ss); /* +135F */
    push16(ram, cpu, cpu->di); /* +1360 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +1361 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1364 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +1366 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +1369 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18898), 0); /* +136B */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +136F */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 28160)))); /* +1371 */
    push16(ram, cpu, cpu->ax); /* +1376 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x137Cu);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x137Cu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1377 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1381u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1381u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +137C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +1381 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1384 */
    cpu->di = (uint16_t)(cpu->ax); /* +1386 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1388 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +138A */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +138C */
    push16(ram, cpu, cpu->ds); /* +1390 */
    push16(ram, cpu, cpu->di); /* +1391 */
    cpu->ax = (uint16_t)(0xffu); /* +1392 */
    push16(ram, cpu, cpu->ax); /* +1395 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x139Bu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x139Bu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1396 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +139B */
    goto L_1327; /* +139E */

L_13a0:
    goto L_1547; /* +13A0 */

L_13a3:
    cmp8(cpu, (uint8_t)cpu->ax, 0x1u); /* +13A3 */
    if ((cpu->flags & 0x40u) != 0u) goto L_13aa; /* +13A5 */
    goto L_144f; /* +13A7 */

L_13aa:
    cmp16(cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x6b00u); /* +13AA */
    if ((cpu->flags & 0x40u) == 0u) goto L_13d3; /* +13AF */
    load_far(ram, cpu, cpu->ds, (uint16_t)(23954), 0); /* +13B1 */
    push16(ram, cpu, cpu->es); /* +13B5 */
    push16(ram, cpu, cpu->di); /* +13B6 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +13B7 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +13BA */
    cpu->di = (uint16_t)(cpu->ax); /* +13BC */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +13BE */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +13C0 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +13C2 */
    push16(ram, cpu, cpu->ds); /* +13C6 */
    push16(ram, cpu, cpu->di); /* +13C7 */
    cpu->ax = (uint16_t)(0xffu); /* +13C8 */
    push16(ram, cpu, cpu->ax); /* +13CB */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x13D1u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x13D1u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +13CC */
    goto L_144c; /* +13D1 */

L_13d3:
    cpu->ip = 0x13D3u;
    if (*remaining == 0u) return PR_U25_STRINGS_LIMIT;
    --*remaining;
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +13D3 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +13D6 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +13D8 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +13DB */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18902), 0); /* +13DD */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +13E1 */
    cmp16(cpu, read16(ram, cpu->es, (uint16_t)(cpu->di + 10752)), 0x0u); /* +13E3 */
    if ((cpu->flags & 0x40u) != 0u) goto L_144c; /* +13E9 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 258)); /* +13EB */
    push16(ram, cpu, cpu->ss); /* +13EF */
    push16(ram, cpu, cpu->di); /* +13F0 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +13F1 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +13F4 */
    cpu->di = (uint16_t)(cpu->ax); /* +13F6 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +13F8 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +13FA */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +13FC */
    push16(ram, cpu, cpu->ds); /* +1400 */
    push16(ram, cpu, cpu->di); /* +1401 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1407u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1407u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1402 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 514)); /* +1407 */
    push16(ram, cpu, cpu->ss); /* +140B */
    push16(ram, cpu, cpu->di); /* +140C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +140D */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1410 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +1412 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +1415 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18902), 0); /* +1417 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +141B */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 10752)))); /* +141D */
    push16(ram, cpu, cpu->ax); /* +1422 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1428u);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1428u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1423 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x142Du);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x142Du)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1428 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +142D */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1430 */
    cpu->di = (uint16_t)(cpu->ax); /* +1432 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1434 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +1436 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +1438 */
    push16(ram, cpu, cpu->ds); /* +143C */
    push16(ram, cpu, cpu->di); /* +143D */
    cpu->ax = (uint16_t)(0xffu); /* +143E */
    push16(ram, cpu, cpu->ax); /* +1441 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1447u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1447u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1442 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +1447 */
    goto L_13d3; /* +144A */

L_144c:
    goto L_1547; /* +144C */

L_144f:
    cmp8(cpu, (uint8_t)cpu->ax, 0x2u); /* +144F */
    if ((cpu->flags & 0x40u) == 0u) goto L_14ce; /* +1451 */

L_1453:
    cpu->ip = 0x1453u;
    if (*remaining == 0u) return PR_U25_STRINGS_LIMIT;
    --*remaining;
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +1453 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1456 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +1458 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +145B */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18906), 0); /* +145D */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +1461 */
    cmp16(cpu, read16(ram, cpu->es, (uint16_t)(cpu->di + -11776)), 0x0u); /* +1463 */
    if ((cpu->flags & 0x40u) != 0u) goto L_14cc; /* +1469 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 258)); /* +146B */
    push16(ram, cpu, cpu->ss); /* +146F */
    push16(ram, cpu, cpu->di); /* +1470 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +1471 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1474 */
    cpu->di = (uint16_t)(cpu->ax); /* +1476 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +1478 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +147A */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +147C */
    push16(ram, cpu, cpu->ds); /* +1480 */
    push16(ram, cpu, cpu->di); /* +1481 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1487u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1487u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1482 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 514)); /* +1487 */
    push16(ram, cpu, cpu->ss); /* +148B */
    push16(ram, cpu, cpu->di); /* +148C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +148D */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +1490 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +1492 */
    cpu->ax = (uint16_t)(shift(cpu, cpu->ax, 0x1u, 0)); /* +1495 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18906), 0); /* +1497 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +149B */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + -11776)))); /* +149D */
    push16(ram, cpu, cpu->ax); /* +14A2 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x14A8u);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x14A8u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +14A3 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x14ADu);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x14ADu)
            return PR_U25_STRINGS_TRANSFER;
    } /* +14A8 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +14AD */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +14B0 */
    cpu->di = (uint16_t)(cpu->ax); /* +14B2 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +14B4 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +14B6 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +14B8 */
    push16(ram, cpu, cpu->ds); /* +14BC */
    push16(ram, cpu, cpu->di); /* +14BD */
    cpu->ax = (uint16_t)(0xffu); /* +14BE */
    push16(ram, cpu, cpu->ax); /* +14C1 */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x14C7u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x14C7u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +14C2 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +14C7 */
    goto L_1453; /* +14CA */

L_14cc:
    goto L_1547; /* +14CC */

L_14ce:
    cmp8(cpu, (uint8_t)cpu->ax, 0x3u); /* +14CE */
    if ((cpu->flags & 0x40u) == 0u) goto L_1547; /* +14D0 */

L_14d2:
    cpu->ip = 0x14D2u;
    if (*remaining == 0u) return PR_U25_STRINGS_LIMIT;
    --*remaining;
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +14D2 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +14D5 */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +14D7 */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +14DA */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +14DE */
    cmp8(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)), 0x0u); /* +14E0 */
    if ((cpu->flags & 0x40u) != 0u) goto L_1547; /* +14E6 */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 258)); /* +14E8 */
    push16(ram, cpu, cpu->ss); /* +14EC */
    push16(ram, cpu, cpu->di); /* +14ED */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +14EE */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +14F1 */
    cpu->di = (uint16_t)(cpu->ax); /* +14F3 */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +14F5 */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +14F7 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +14F9 */
    push16(ram, cpu, cpu->ds); /* +14FD */
    push16(ram, cpu, cpu->di); /* +14FE */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1504u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B0Bu;
        status = body_0b0b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1504u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +14FF */
    cpu->di = (uint16_t)((uint16_t)(cpu->bp - 514)); /* +1504 */
    push16(ram, cpu, cpu->ss); /* +1508 */
    push16(ram, cpu, cpu->di); /* +1509 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)))); /* +150A */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +150D */
    cpu->ax = (uint16_t)(arithmetic(cpu, cpu->ax, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x8000u, 0)); /* +150F */
    load_far(ram, cpu, cpu->ds, (uint16_t)(18910), 0); /* +1512 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, cpu->ax, 0x8000u, 0)); /* +1516 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->es, (uint16_t)(cpu->di + 26368)))); /* +1518 */
    push16(ram, cpu, cpu->ax); /* +151D */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1523u);
        cpu->cs = system_cs;
        cpu->ip = 0x0C3Bu;
        status = body_0c3b(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1523u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +151E */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1528u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B98u;
        status = body_0b98(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1528u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +1523 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp + 8)))); /* +1528 */
    cpu->ax &= 0x00FFu;
    logical(cpu, 0u, 8u); /* +152B */
    cpu->di = (uint16_t)(cpu->ax); /* +152D */
    cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | (uint8_t)(0x8u)); /* +152F */
    cpu->di = (uint16_t)(shift(cpu, cpu->di, (uint8_t)cpu->cx, 0)); /* +1531 */
    cpu->di = (uint16_t)(arithmetic(cpu, cpu->di, 0x6fc8u, 0x8000u, 0)); /* +1533 */
    push16(ram, cpu, cpu->ds); /* +1537 */
    push16(ram, cpu, cpu->di); /* +1538 */
    cpu->ax = (uint16_t)(0xffu); /* +1539 */
    push16(ram, cpu, cpu->ax); /* +153C */
    {
        uint16_t caller_cs = cpu->cs;
        pr_u25_strings_status status;
        push16(ram, cpu, caller_cs);
        push16(ram, cpu, 0x1542u);
        cpu->cs = system_cs;
        cpu->ip = 0x0B25u;
        status = body_0b25(ram, cpu, system_cs, remaining);
        if (status != PR_U25_STRINGS_OK) return status;
        if (cpu->cs != caller_cs || cpu->ip != 0x1542u)
            return PR_U25_STRINGS_TRANSFER;
    } /* +153D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 2), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 2)), 8u))); /* +1542 */
    goto L_14d2; /* +1545 */

L_1547:
    cpu->sp = (uint16_t)(cpu->bp); /* +1547 */
    cpu->bp = (uint16_t)(pop16(ram, cpu)); /* +1549 */
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U25_STRINGS_OK; /* +154A */
}

pr_u25_strings_status pr_u25_unpack_text_1153(
    uint8_t *ram, size_t ram_size, pr_u25_strings_cpu *cpu,
    uint16_t system_cs, size_t max_iterations)
{
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_STRINGS_RAM_SIZE ||
        cpu->ip != 0x1153u || max_iterations == 0u ||
        cpu->sp < 0x0218u || cpu->sp > 0xFFF8u)
        return PR_U25_STRINGS_ARGUMENT;
    return body_1153(ram, cpu, system_cs, &max_iterations);
}

pr_u25_strings_status pr_u25_string_by_address_12fb(
    uint8_t *ram, size_t ram_size, pr_u25_strings_cpu *cpu,
    uint16_t system_cs, size_t max_iterations)
{
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_STRINGS_RAM_SIZE ||
        cpu->ip != 0x12FBu || max_iterations == 0u ||
        cpu->sp < 0x0218u || cpu->sp > 0xFFF8u)
        return PR_U25_STRINGS_ARGUMENT;
    return body_12fb(ram, cpu, system_cs, &max_iterations);
}
