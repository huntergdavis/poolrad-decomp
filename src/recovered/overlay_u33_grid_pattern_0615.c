/* Complete U33 [0615,0810); docs/re/U33_GRID_PATTERN_0615.md. */
#include "poolrad/recovered/overlay_u33_grid_pattern_0615.h"
#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"

#define MASK (PR_U33_01AD_RAM_SIZE - 1u)
#define ARITH 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}
static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t p = physical(segment, offset);
    return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8));
}
static void write8(uint8_t *ram, uint16_t segment, uint16_t offset, uint8_t value)
{
    ram[physical(segment, offset)] = value;
}
static void write16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t p = physical(segment, offset);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & MASK] = (uint8_t)(value >> 8);
}
static void push(uint8_t *ram, pr_u33_0615_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}
static uint16_t pop(const uint8_t *ram, pr_u33_0615_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}
static void set_al(pr_u33_0615_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}
static void cbw(pr_u33_0615_cpu *cpu)
{
    uint16_t value = cpu->ax & 0xFFu;
    cpu->ax = (uint16_t)((value & 0x80u) ? value | 0xFF00u : value);
}
static uint16_t arithmetic(pr_u33_0615_cpu *cpu, uint16_t a, uint16_t b,
                           unsigned bits, int subtract)
{
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t result = (uint16_t)(wide & mask);
    uint8_t parity = (uint8_t)result;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    cpu->flags &= (uint16_t)~ARITH;
    if (subtract ? a < b : wide > mask) cpu->flags |= 1u;
    if ((parity & 1u) == 0u) cpu->flags |= 4u;
    if (((a ^ b ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;
    if (result == 0u) cpu->flags |= 0x40u;
    if ((result & sign) != 0u) cpu->flags |= 0x80u;
    if (((subtract ? a ^ b : (uint16_t)~(a ^ b)) &
         (a ^ result) & sign) != 0u) cpu->flags |= 0x800u;
    return result;
}
static uint16_t increment(pr_u33_0615_cpu *cpu, uint16_t value,
                          unsigned bits, int decrement)
{
    uint16_t carry = cpu->flags & 1u;
    uint16_t result = arithmetic(cpu, value, 1u, bits, decrement);
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~1u) | carry);
    return result;
}
#define COPY_CPU(dst, src) do { \
    (dst)->ax = (src)->ax; \
    (dst)->bx = (src)->bx; \
    (dst)->cx = (src)->cx; \
    (dst)->dx = (src)->dx; \
    (dst)->si = (src)->si; \
    (dst)->di = (src)->di; \
    (dst)->bp = (src)->bp; \
    (dst)->sp = (src)->sp; \
    (dst)->cs = (src)->cs; \
    (dst)->ip = (src)->ip; \
    (dst)->ds = (src)->ds; \
    (dst)->es = (src)->es; \
    (dst)->ss = (src)->ss; \
    (dst)->flags = (src)->flags; \
} while (0)

static pr_u33_0615_status near_call(uint8_t *ram, size_t size,
    pr_u33_0615_cpu *cpu, uint16_t target, uint16_t continuation)
{
    uint16_t caller_cs = cpu->cs;
    push(ram, cpu, continuation); /* Caller has already pushed CS. */
    cpu->ip = target;
    if (target == 0x01ADu) {
        pr_u33_01ad_status status = pr_u33_cell_pair_01ad(ram, size, cpu);
        if (status == PR_U33_01AD_TRANSFER) return PR_U33_0615_TRANSFER;
        if (status != PR_U33_01AD_OK) return PR_U33_0615_CALLEE;
    } else {
        pr_u33_0221_cpu inner;
        pr_u33_0221_status status;
        COPY_CPU(&inner, cpu);
        status = pr_u33_clipped_writer_0221(ram, size, &inner);
        COPY_CPU(cpu, &inner);
        if (status != PR_U33_0221_OK) return PR_U33_0615_CALLEE;
    }
    return cpu->cs == caller_cs && cpu->ip == continuation ?
        PR_U33_0615_OK : PR_U33_0615_TRANSFER;
}

pr_u33_0615_status pr_u33_grid_pattern_0615(
    uint8_t *ram, size_t ram_size, pr_u33_0615_cpu *cpu)
{
    pr_u33_0615_status status;
    unsigned i;
    if (ram == NULL || cpu == NULL || ram_size != PR_U33_01AD_RAM_SIZE ||
        cpu->ip != 0x0615u || cpu->sp < 0x0080u || cpu->sp > 0xFFFAu)
        return PR_U33_0615_ARGUMENT;
    for (i = 1u; i <= 128u; ++i) {
        size_t p = physical(cpu->ss, (uint16_t)(cpu->sp - i));
        size_t relative = (p - physical(cpu->cs, 0u)) & MASK;
        if ((relative >= 0x012Bu && relative < 0x02BAu) ||
            (relative >= 0x0615u && relative < 0x0810u))
            return PR_U33_0615_CODE_ALIAS;
    }
    push(ram, cpu, cpu->bp); /* +0615 */
    cpu->bp = cpu->sp; /* +0616 */
    cpu->sp = arithmetic(cpu, cpu->sp, 0x8u, 16u, 1); /* +0618 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +061B */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x3u)))); /* +061E */
    push(ram, cpu, cpu->ax); /* +0622 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0623 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x4u)))); /* +0626 */
    cbw(cpu); /* +062A */
    cpu->ax = increment(cpu, cpu->ax, 16u, 1); /* +062B */
    push(ram, cpu, cpu->ax); /* +062C */
    set_al(cpu, (uint8_t)(0x2u)); /* +062D */
    push(ram, cpu, cpu->ax); /* +062F */
    push(ram, cpu, cpu->cs); /* +0630 */
    status = near_call(ram, ram_size, cpu, 0x01ADu, 0x0634u); /* +0631 */
    if (status != PR_U33_0615_OK) return status;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u), (uint8_t)((uint8_t)cpu->ax)); /* +0634 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0637 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x3u)))); /* +063A */
    cbw(cpu); /* +063E */
    cpu->ax = increment(cpu, cpu->ax, 16u, 0); /* +063F */
    push(ram, cpu, cpu->ax); /* +0640 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0641 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x4u)))); /* +0644 */
    push(ram, cpu, cpu->ax); /* +0648 */
    set_al(cpu, (uint8_t)(0x0u)); /* +0649 */
    push(ram, cpu, cpu->ax); /* +064B */
    push(ram, cpu, cpu->cs); /* +064C */
    status = near_call(ram, ram_size, cpu, 0x01ADu, 0x0650u); /* +064D */
    if (status != PR_U33_0615_OK) return status;
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u), (uint8_t)((uint8_t)cpu->ax)); /* +0650 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)), 0x0u, 8u, 1); /* +0653 */
    if ((cpu->flags & 0x40u) == 0u) goto L0665; /* +0657 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +0659 */
    if ((cpu->flags & 0x40u) == 0u) goto L0665; /* +065D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u), (uint8_t)(0x1u)); /* +065F */
    goto L0669; /* +0663 */
L0665:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u), (uint8_t)(0x0u)); /* +0665 */
L0669:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), (uint8_t)(0x1u)); /* +0669 */
    goto L0672; /* +066D */
L066F:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u), (uint8_t)(increment(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)), 8u, 0))); /* +066F */
L0672:
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)))); /* +0672 */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +0675 */
    if ((cpu->flags & 0x40u) == 0u) goto L06AB; /* +0677 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0679 */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +067C */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +0680 */
    if ((cpu->flags & 0x40u) == 0u) goto L0696; /* +0682 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)), 0x1u, 8u, 1); /* +0684 */
    if ((cpu->flags & 0x40u) == 0u) goto L0690; /* +0688 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x4u)); /* +068A */
    goto L0694; /* +068E */
L0690:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x16u)); /* +0690 */
L0694:
    goto L06A8; /* +0694 */
L0696:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0696 */
    if ((cpu->flags & 0x40u) == 0u) goto L06A0; /* +0698 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0xFu)); /* +069A */
    goto L06A8; /* +069E */
L06A0:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +06A0 */
    if ((cpu->flags & 0x40u) == 0u) goto L06A8; /* +06A2 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u), (uint8_t)(0x5u)); /* +06A4 */
L06A8:
    goto L07BD; /* +06A8 */
L06AB:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x2u, 8u, 1); /* +06AB */
    if ((cpu->flags & 0x40u) != 0u) goto L06B2; /* +06AD */
    goto L0738; /* +06AF */
L06B2:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +06B2 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)), 0x0u, 8u, 1); /* +06B5 */
    if ((cpu->flags & 0x40u) == 0u) goto L0709; /* +06BA */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)))); /* +06BC */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +06BF */
    if ((cpu->flags & 0x40u) == 0u) goto L06C9; /* +06C1 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x16u)); /* +06C3 */
    goto L0707; /* +06C7 */
L06C9:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +06C9 */
    if ((cpu->flags & 0x40u) == 0u) goto L06E9; /* +06CB */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +06CD */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x9u)), 0x0u, 8u, 1); /* +06D0 */
    if ((cpu->flags & 0x40u) == 0u) goto L06E3; /* +06D5 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +06D7 */
    if ((cpu->flags & 0x40u) != 0u) goto L06E3; /* +06DB */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x18u)); /* +06DD */
    goto L06E7; /* +06E1 */
L06E3:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x1u)); /* +06E3 */
L06E7:
    goto L0707; /* +06E7 */
L06E9:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +06E9 */
    if ((cpu->flags & 0x40u) == 0u) goto L0707; /* +06EB */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +06ED */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x9u)), 0x0u, 8u, 1); /* +06F0 */
    if ((cpu->flags & 0x40u) == 0u) goto L0703; /* +06F5 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +06F7 */
    if ((cpu->flags & 0x40u) != 0u) goto L0703; /* +06FB */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0xBu)); /* +06FD */
    goto L0707; /* +0701 */
L0703:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x3u)); /* +0703 */
L0707:
    goto L0735; /* +0707 */
L0709:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0709 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x9u)), 0x0u, 8u, 1); /* +070C */
    if ((cpu->flags & 0x40u) != 0u) goto L0719; /* +0711 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x9u)); /* +0713 */
    goto L0735; /* +0717 */
L0719:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +0719 */
    if ((cpu->flags & 0x40u) != 0u) goto L0725; /* +071D */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x5u)); /* +071F */
    goto L0735; /* +0723 */
L0725:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x6u)), 0x0u, 8u, 1); /* +0725 */
    if ((cpu->flags & 0x40u) != 0u) goto L0731; /* +0729 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x11u)); /* +072B */
    goto L0735; /* +072F */
L0731:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u), (uint8_t)(0x13u)); /* +0731 */
L0735:
    goto L07BD; /* +0735 */
L0738:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +0738 */
    if ((cpu->flags & 0x40u) == 0u) goto L0761; /* +073A */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +073C */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)))); /* +073F */
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x0u, 8u, 1); /* +0743 */
    if ((cpu->flags & 0x40u) == 0u) goto L074D; /* +0745 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x16u)); /* +0747 */
    goto L075F; /* +074B */
L074D:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x3u, 8u, 1); /* +074D */
    if ((cpu->flags & 0x40u) == 0u) goto L0757; /* +074F */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0x10u)); /* +0751 */
    goto L075F; /* +0755 */
L0757:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x1u, 8u, 1); /* +0757 */
    if ((cpu->flags & 0x40u) == 0u) goto L075F; /* +0759 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u), (uint8_t)(0xAu)); /* +075B */
L075F:
    goto L07BD; /* +075F */
L0761:
    (void)arithmetic(cpu, (uint8_t)cpu->ax, 0x4u, 8u, 1); /* +0761 */
    if ((cpu->flags & 0x40u) == 0u) goto L07BD; /* +0763 */
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +0765 */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x7u)), 0x0u, 8u, 1); /* +0768 */
    if ((cpu->flags & 0x40u) == 0u) goto L079D; /* +076D */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x7u)), 0x0u, 8u, 1); /* +076F */
    if ((cpu->flags & 0x40u) == 0u) goto L077B; /* +0773 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x16u)); /* +0775 */
    goto L079B; /* +0779 */
L077B:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +077B */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x9u)), 0x0u, 8u, 1); /* +077E */
    if ((cpu->flags & 0x40u) != 0u) goto L078B; /* +0783 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x4u)); /* +0785 */
    goto L079B; /* +0789 */
L078B:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +078B */
    if ((cpu->flags & 0x40u) == 0u) goto L0797; /* +078F */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x8u)); /* +0791 */
    goto L079B; /* +0795 */
L0797:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0xCu)); /* +0797 */
L079B:
    goto L07BD; /* +079B */
L079D:
    cpu->di = read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u)); /* +079D */
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->di - 0x9u)), 0x0u, 8u, 1); /* +07A0 */
    if ((cpu->flags & 0x40u) != 0u) goto L07AD; /* +07A5 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0xEu)); /* +07A7 */
    goto L07BD; /* +07AB */
L07AD:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x8u)), 0x0u, 8u, 1); /* +07AD */
    if ((cpu->flags & 0x40u) == 0u) goto L07B9; /* +07B1 */
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0x17u)); /* +07B3 */
    goto L07BD; /* +07B7 */
L07B9:
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u), (uint8_t)(0xAu)); /* +07B9 */
L07BD:
    (void)arithmetic(cpu, read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x1u)), 0x4u, 8u, 1); /* +07BD */
    if ((cpu->flags & 0x40u) != 0u) goto L07C6; /* +07C1 */
    goto L066F; /* +07C3 */
L07C6:
    set_al(cpu, (uint8_t)(0x5u)); /* +07C6 */
    push(ram, cpu, cpu->ax); /* +07C8 */
    set_al(cpu, (uint8_t)(0x0u)); /* +07C9 */
    push(ram, cpu, cpu->ax); /* +07CB */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x2u)))); /* +07CC */
    push(ram, cpu, cpu->ax); /* +07CF */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +07D0 */
    push(ram, cpu, cpu->cs); /* +07D3 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x07D7u); /* +07D4 */
    if (status != PR_U33_0615_OK) return status;
    set_al(cpu, (uint8_t)(0x6u)); /* +07D7 */
    push(ram, cpu, cpu->ax); /* +07D9 */
    set_al(cpu, (uint8_t)(0x0u)); /* +07DA */
    push(ram, cpu, cpu->ax); /* +07DC */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x4u)))); /* +07DD */
    push(ram, cpu, cpu->ax); /* +07E0 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +07E1 */
    push(ram, cpu, cpu->cs); /* +07E4 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x07E8u); /* +07E5 */
    if (status != PR_U33_0615_OK) return status;
    set_al(cpu, (uint8_t)(0x5u)); /* +07E8 */
    push(ram, cpu, cpu->ax); /* +07EA */
    set_al(cpu, (uint8_t)(0x1u)); /* +07EB */
    push(ram, cpu, cpu->ax); /* +07ED */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x3u)))); /* +07EE */
    push(ram, cpu, cpu->ax); /* +07F1 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +07F2 */
    push(ram, cpu, cpu->cs); /* +07F5 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x07F9u); /* +07F6 */
    if (status != PR_U33_0615_OK) return status;
    set_al(cpu, (uint8_t)(0x6u)); /* +07F9 */
    push(ram, cpu, cpu->ax); /* +07FB */
    set_al(cpu, (uint8_t)(0x1u)); /* +07FC */
    push(ram, cpu, cpu->ax); /* +07FE */
    set_al(cpu, (uint8_t)(read8(ram, cpu->ss, (uint16_t)(cpu->bp - 0x5u)))); /* +07FF */
    push(ram, cpu, cpu->ax); /* +0802 */
    push(ram, cpu, read16(ram, cpu->ss, (uint16_t)(cpu->bp + 0x6u))); /* +0803 */
    push(ram, cpu, cpu->cs); /* +0806 */
    status = near_call(ram, ram_size, cpu, 0x0221u, 0x080Au); /* +0807 */
    if (status != PR_U33_0615_OK) return status;
    cpu->sp = cpu->bp; /* +080A */
    cpu->bp = pop(ram, cpu); /* +080C */
    {
        /* RETF reads four physically contiguous bytes even if a grid alias
         * changed saved BP and moved the outer frame across offset FFFF. */
        size_t p = physical(cpu->ss, cpu->sp);
        cpu->ip = read16(ram, cpu->ss, cpu->sp);
        cpu->cs = (uint16_t)(ram[(p + 2u) & MASK] |
                            ((uint16_t)ram[(p + 3u) & MASK] << 8));
        cpu->sp = (uint16_t)(cpu->sp + 6u);
    } /* +080D */
    return PR_U33_0615_OK;
}
