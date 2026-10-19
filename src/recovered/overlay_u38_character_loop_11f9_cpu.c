/* Complete U38 [11F9,1268); raw SYSTEM StrCmp composed.
 * Memory/flags/composition reuse U38 +0679/+0947. */
#include "poolrad/recovered/overlay_u38_character_loop_11f9_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#define MASK (PR_U38_11F9_RAM_SIZE - 1u)
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

static void push16(uint8_t *ram, pr_u38_11f9_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(const uint8_t *ram, pr_u38_11f9_cpu *cpu, uint16_t segment, uint16_t offset)
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

static void szp(pr_u38_11f9_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u38_11f9_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

static uint16_t add16(pr_u38_11f9_cpu *cpu, uint16_t a, uint16_t b)
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
static void inc8(uint8_t *ram, pr_u38_11f9_cpu *cpu, size_t at)
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
static pr_u38_11f9_status compare(uint8_t *ram, size_t ram_size, pr_u38_11f9_cpu *cpu, uint16_t site,
                                          int *callee_status)
{
    pr_system_leaf_cpu state;
    pr_system_leaf_status status;
    uint16_t caller_cs = cpu->cs, back = (uint16_t)(site + 5u);
    cpu->ip = site;
    if (rd8(ram, cpu->cs, site) != 0x9Au || rd16(ram, cpu->cs, (uint16_t)(site + 1u)) != STRCMP_IP)
        return PR_U38_11F9_CODE;
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
        return PR_U38_11F9_CALLEE;
    if (cpu->ip != back || cpu->cs != caller_cs)
        return PR_U38_11F9_TRANSFER;
    return PR_U38_11F9_OK;
}

pr_u38_11f9_status pr_u38_character_loop_11f9_cpu(uint8_t *ram, size_t ram_size,
    pr_u38_11f9_cpu *cpu, unsigned max_loopbacks, int *callee_status)
{
    pr_u38_11f9_status status;
    unsigned loopbacks = 0;
    uint8_t byte;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U38_11F9_RAM_SIZE || cpu->ip != 0x11F9u || cpu->sp < 0x10u || cpu->sp > 0xFFF2u)
        return PR_U38_11F9_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 11F9 PUSH BP; MOV BP,SP; SUB SP,2 */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 2u, 0x8000u);
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    les(ram, cpu, cpu->ss, (uint16_t)(cpu->bp + 0x0Cu));                  /* 11FF P := [[BP+0Ch]+108h]+0Ah */
    les(ram, cpu, cpu->es, (uint16_t)(cpu->di + 0x108u));
    cpu->ax = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Au));
    cpu->dx = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x0Cu));
    wr16(ram, cpu->ds, 0x6826u, cpu->ax);
    wr16(ram, cpu->ds, 0x6828u, cpu->dx);
    les(ram, cpu, cpu->ds, 0x5D92u);                                      /* 1216 CMP BYTE active[9Fh],1; JNE 1262 */
    byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x9Fu));
    sub_flags(cpu, byte, 1u, 0x80u);
    if (byte == 1u) {
        les(ram, cpu, cpu->ds, 0x5D92u);                                  /* 1222 MOV AL,active[6Ch]; AND AL,7Fh; CMP AL,2; JNE 1262 */
        byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x6Cu));
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | (byte & 0x7Fu));
        byte = (uint8_t)cpu->ax;                                      /* AND AL,7Fh; CMP AL,2 */
        sub_flags(cpu, byte, 2u, 0x80u);
        if (byte == 2u) {
            ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = 1u;        /* 1230 MOV BYTE [BP-1],1; JMP 1239 */
            for (;;) {
                les(ram, cpu, cpu->ds, 0x6826u);                          /* 1239 PUSH P; PUSH DS:(03F2h + 16i) */
                push16(ram, cpu, cpu->es);
                push16(ram, cpu, cpu->di);
                cpu->ax = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));    /* MOV AL,[BP-1]; XOR AH,AH */
                cpu->flags &= (uint16_t)~0x08D5u;
                szp(cpu, 0u, 0x80u);
                cpu->di = cpu->ax;                                        /* MOV DI,AX; MOV CL,4; SHL DI,CL */
                cpu->cx = (uint16_t)((cpu->cx & 0xFF00u) | 4u);
                cpu->di = (uint16_t)(cpu->di << 4);
                cpu->di = add16(cpu, cpu->di, 0x03F2u);                     /* ADD DI,03F2h */
                push16(ram, cpu, cpu->ds);
                push16(ram, cpu, cpu->di);
                status = compare(ram, ram_size, cpu, 0x1250u, callee_status); /* 1250 LCALL 0AF8:0C10 */
                if (status != PR_U38_11F9_OK)
                    return status;
                if (cpu->flags & 0x0040u) {                               /* +1255 JNE; SUB BYTE [6822h],4 */
                    byte = rd8(ram, cpu->ds, 0x6822u);
                    sub_flags(cpu, byte, 4u, 0x80u);
                    ram[physical(cpu->ds, 0x6822u)] = (uint8_t)(byte - 4u);
                }
                byte = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u));       /* 125C CMP BYTE [BP-1],8; JNE 1236 */
                sub_flags(cpu, byte, 8u, 0x80u);
                if (byte == 8u)
                    break;
                if (loopbacks == max_loopbacks) {
                    cpu->ip = 0x1236u;
                    return PR_U38_11F9_LIMIT;
                }
                ++loopbacks;
                inc8(ram, cpu, physical(cpu->ss, (uint16_t)(cpu->bp - 1u))); /* 1236 INC BYTE [BP-1] */
            }
        }
    }
    cpu->sp = cpu->bp;                                                    /* 1262 MOV SP,BP; POP BP; RETF 0Ah */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 0x0Au);
    return PR_U38_11F9_OK;
}
