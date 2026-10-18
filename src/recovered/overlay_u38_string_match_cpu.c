/* GAME.OVR unit 0x38 +0679 and its twin +0947 (raw CPU, composing SYSTEM StrCmp 0AF8:0C10); docs/re/U38_STRING_MATCH_CPU.md. */
#include "poolrad/recovered/overlay_u38_string_match_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#define MASK (PR_U38_STRING_MATCH_RAM_SIZE - 1u)
#define STRCMP_IP 0x0C10u

#define COPY_CPU(to, from)                                                                                   \
    do {                                                                                                     \
        (to)->ax = (from)->ax; (to)->bx = (from)->bx; (to)->cx = (from)->cx; (to)->dx = (from)->dx;          \
        (to)->si = (from)->si; (to)->di = (from)->di; (to)->bp = (from)->bp; (to)->sp = (from)->sp;          \
        (to)->cs = (from)->cs; (to)->ip = (from)->ip; (to)->ds = (from)->ds; (to)->es = (from)->es;          \
        (to)->ss = (from)->ss; (to)->flags = (from)->flags;                                                  \
    } while (0)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint8_t rd8(const uint8_t *ram, uint16_t segment, uint16_t offset) { return ram[physical(segment, offset)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t segment, uint16_t offset) { return read16_at(ram, physical(segment, offset)); }

static void wr16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u38_string_match_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(const uint8_t *ram, pr_u38_string_match_cpu *cpu, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void szp(pr_u38_string_match_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u38_string_match_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

static uint16_t add16(pr_u38_string_match_cpu *cpu, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b;
    uint16_t result = (uint16_t)wide;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (wide > 0xFFFFu) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if (~(a ^ b) & (a ^ result) & 0x8000u) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x8000u);
    return result;
}

/* INC BYTE at a physical address: CF kept. */
static void inc8(uint8_t *ram, pr_u38_string_match_cpu *cpu, size_t at)
{
    uint8_t before = ram[at], result = (uint8_t)(before + 1u);
    ram[at] = result;
    cpu->flags &= (uint16_t)~0x08D4u;
    if ((before & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
    if (before == 0x7Fu) cpu->flags |= 0x0800u;
    szp(cpu, result, 0x80u);
}

/* LCALL 0AF8:0C10 at `site`: checks 9A and the offset, reads the segment from RAM, pushes CS and site + 5, and runs the
 * merged raw-CPU SYSTEM string compare. */
static pr_u38_string_match_status compare(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu, uint16_t site,
                                          int *callee_status)
{
    pr_system_leaf_cpu state;
    pr_system_leaf_status status;
    uint16_t caller_cs = cpu->cs, back = (uint16_t)(site + 5u);
    cpu->ip = site;
    if (rd8(ram, cpu->cs, site) != 0x9Au || rd16(ram, cpu->cs, (uint16_t)(site + 1u)) != STRCMP_IP)
        return PR_U38_STRING_MATCH_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, back);
    cpu->cs = rd16(ram, caller_cs, (uint16_t)(site + 3u));
    cpu->ip = STRCMP_IP;
    COPY_CPU(&state, cpu);
    status = pr_system_strcmp_0c10(ram, ram_size, &state);
    COPY_CPU(cpu, &state);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_SYSTEM_LEAF_RETURNED)
        return PR_U38_STRING_MATCH_CALLEE;
    if (cpu->ip != back || cpu->cs != caller_cs)
        return PR_U38_STRING_MATCH_TRANSFER;
    return PR_U38_STRING_MATCH_OK;
}

/* The routine at `entry`, comparing with the 16-byte table slots at `table` + 16i for i = 1..last. Offsets in the
 * comments are +0679's. */
static pr_u38_string_match_status match(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu, uint16_t entry,
                                        uint16_t table, uint8_t last, unsigned max_loopbacks, int *callee_status)
{
    pr_u38_string_match_status status;
    unsigned loopbacks = 0;
    uint8_t byte;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U38_STRING_MATCH_RAM_SIZE || cpu->ip != entry)
        return PR_U38_STRING_MATCH_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 0679 PUSH BP; MOV BP,SP; SUB SP,2 */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 2u, 0x8000u);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));                  /* 067F P := [[BP+0Ch]+108h]+0Ah */
    les(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
    cpu->ax = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Au));
    cpu->dx = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Cu));
    wr16(ram, cpu->ds, 0x6826u, cpu->ax);
    wr16(ram, cpu->ds, 0x6828u, cpu->dx);
    les(ram, cpu, cpu->ds, 0x6826u);                                      /* 0696 CMP BYTE P[9Fh],1; JNE 06DE */
    byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x9Fu));
    sub_flags(cpu, byte, 1u, 0x80u);
    if (byte == 1u) {
        les(ram, cpu, cpu->ds, 0x6826u);                                  /* 06A2 CMP BYTE P[6Ch],1; JNE 06DE */
        byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x6Cu));
        sub_flags(cpu, byte, 1u, 0x80u);
        if (byte == 1u) {
            ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = 1u;        /* 06AD MOV BYTE [BP-1],1; JMP 06B6 */
            for (;;) {
                les(ram, cpu, cpu->ds, 0x6826u);                          /* 06B6 PUSH P; PUSH DS:(table + 16i) */
                push16(ram, cpu, cpu->es);
                push16(ram, cpu, cpu->di);
                cpu->ax = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));    /* MOV AL,[BP-1]; XOR AH,AH */
                cpu->flags &= (uint16_t)~0x08D5u;
                szp(cpu, 0u, 0x80u);
                cpu->di = cpu->ax;                                        /* MOV DI,AX; MOV CL,4; SHL DI,CL */
                cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
                cpu->di = (uint16_t)(cpu->di << 4);
                cpu->di = add16(cpu, cpu->di, table);                     /* ADD DI,0332h */
                push16(ram, cpu, cpu->ds);
                push16(ram, cpu, cpu->di);
                status = compare(ram, ram_size, cpu, (uint16_t)(entry + 0x54u), callee_status); /* 06CD LCALL 0AF8:0C10 */
                if (status != PR_U38_STRING_MATCH_OK)
                    return status;
                if (cpu->flags & 0x0040u)                                 /* 06D2 JNE 06D8; INC BYTE [6822h] */
                    inc8(ram, cpu, physical(cpu->ds, 0x6822u));
                byte = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));       /* 06D8 CMP BYTE [BP-1],4; JNE 06B3 */
                sub_flags(cpu, byte, last, 0x80u);
                if (byte == last)
                    break;
                if (loopbacks == max_loopbacks) {
                    cpu->ip = (uint16_t)(entry + 0x3Au);
                    return PR_U38_STRING_MATCH_LIMIT;
                }
                ++loopbacks;
                inc8(ram, cpu, physical(cpu->ss, (uint16_t)(cpu->bp - 1u))); /* 06B3 INC BYTE [BP-1] */
            }
        }
    }
    cpu->sp = cpu->bp;                                                    /* 06DE MOV SP,BP; POP BP; RETF 0Ah */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_U38_STRING_MATCH_OK;
}

pr_u38_string_match_status pr_u38_string_match_0679_cpu(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu,
                                                        unsigned max_loopbacks, int *callee_status)
{
    return match(ram, ram_size, cpu, 0x0679u, 0x0332u, 4u, max_loopbacks, callee_status);
}

pr_u38_string_match_status pr_u38_string_match_0947_cpu(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu,
                                                        unsigned max_loopbacks, int *callee_status)
{
    return match(ram, ram_size, cpu, 0x0947u, 0x0372u, 8u, max_loopbacks, callee_status);
}
