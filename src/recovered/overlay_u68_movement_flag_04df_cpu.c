/* GAME.OVR unit 0x68 +04DF (raw CPU, composing the U9E entry 8 dice chain and the raw U68 +0109);
 * docs/re/U68_MOVEMENT_FLAG_04DF_CPU.md. */
#include "poolrad/recovered/overlay_u68_movement_flag_04df_cpu.h"
#include "poolrad/recovered/overlay_u68_map_update_0109_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"

#define MASK (PR_U68_04DF_RAM_SIZE - 1u)
#define STUB_OFFSET 0x0048u
#define ENTRY8_IP 0x0DE3u

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

static void push16(uint8_t *ram, pr_u68_04df_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    wr16(ram, cpu->ss, cpu->sp, value);
}

static void les_local(const uint8_t *ram, pr_u68_04df_cpu *cpu, uint16_t displacement)
{
    size_t at = physical(cpu->ss, (uint16_t)(cpu->bp - displacement));
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

static void szp(pr_u68_04df_cpu *cpu, uint16_t result, uint16_t sign)
{
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((result & (uint16_t)((sign << 1) - 1u)) == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
}

static void sub_flags(pr_u68_04df_cpu *cpu, uint16_t a, uint16_t b, uint16_t sign)
{
    uint16_t result = (uint16_t)((a - b) & (uint16_t)((sign << 1) - 1u));
    cpu->flags &= (uint16_t)~0x08D5u;
    if (a < b) cpu->flags |= 0x0001u;
    if ((a ^ b ^ result) & 0x0010u) cpu->flags |= 0x0010u;
    if ((a ^ b) & (a ^ result) & sign) cpu->flags |= 0x0800u;
    szp(cpu, result, sign);
}

static uint16_t add16(pr_u68_04df_cpu *cpu, uint16_t a, uint16_t b)
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

static void logic_flags(pr_u68_04df_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    szp(cpu, result, sign);
}

static void set_al(pr_u68_04df_cpu *cpu, uint8_t value) { cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value); }
static void cbw(pr_u68_04df_cpu *cpu) { cpu->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)cpu->ax; }

/* MOV AL,DS:[offset]; XOR AH,AH. */
static void load_zero_ah(const uint8_t *ram, pr_u68_04df_cpu *cpu, uint16_t offset)
{
    cpu->ax = rd8(ram, cpu->ds, offset);
    logic_flags(cpu, 0u, 0x80u);
}

/* +0513: LCALL 00B0:0048 through the loaded stub into entry 8 (+0DE3), as the accepted U2F +0131 does. */
static pr_u68_04df_status roll(uint8_t *ram, size_t ram_size, pr_u68_04df_cpu *cpu, int *callee_status)
{
    pr_u9e_entry8_compose_cpu chain;
    pr_u9e_entry8_compose_status status;
    uint16_t caller_cs = cpu->cs, stub_segment;
    cpu->ip = 0x0513u;
    if (rd8(ram, cpu->cs, 0x0513u) != 0x9Au || rd16(ram, cpu->cs, 0x0514u) != STUB_OFFSET)
        return PR_U68_04DF_CODE;
    stub_segment = rd16(ram, cpu->cs, 0x0516u);
    if (rd8(ram, stub_segment, STUB_OFFSET) != 0xEAu || rd16(ram, stub_segment, STUB_OFFSET + 1u) != ENTRY8_IP)
        return PR_U68_04DF_CODE;
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, 0x0518u);
    cpu->ip = ENTRY8_IP;
    cpu->cs = rd16(ram, stub_segment, STUB_OFFSET + 3u);
    COPY_CPU(&chain, cpu);
    status = pr_u9e_entry8_fullstate_compose_0de3(ram, ram_size, &chain, 256u);
    COPY_CPU(cpu, &chain);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_U9E_ENTRY8_COMPOSE_RETURNED)
        return PR_U68_04DF_CALLEE;
    if (cpu->ip != 0x0518u || cpu->cs != caller_cs)
        return PR_U68_04DF_TRANSFER;
    return PR_U68_04DF_OK;
}

/* PUSH CS; CALL 0109 from the CALL at `site`: the raw +0109. */
static pr_u68_04df_status mark(uint8_t *ram, size_t ram_size, pr_u68_04df_cpu *cpu, uint16_t site, int *callee_status)
{
    pr_u68_0109_cpu state;
    pr_u68_0109_status status;
    uint16_t caller_cs = cpu->cs, back = (uint16_t)(site + 3u);
    push16(ram, cpu, cpu->cs);
    push16(ram, cpu, back);
    cpu->ip = 0x0109u;
    COPY_CPU(&state, cpu);
    status = pr_u68_map_update_0109_cpu(ram, ram_size, &state);
    COPY_CPU(cpu, &state);
    if (callee_status != NULL)
        *callee_status = (int)status;
    if (status != PR_U68_0109_OK)
        return PR_U68_04DF_CALLEE;
    if (cpu->ip != back || cpu->cs != caller_cs)
        return PR_U68_04DF_TRANSFER;
    return PR_U68_04DF_OK;
}

pr_u68_04df_status pr_u68_movement_flag_04df_cpu(uint8_t *ram, size_t ram_size, pr_u68_04df_cpu *cpu, unsigned max_loopbacks,
                                              int *callee_status)
{
    pr_u68_04df_status status;
    unsigned loopbacks = 0;
    uint8_t byte;
    int16_t dividend;
    size_t at;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U68_04DF_RAM_SIZE || cpu->ip != 0x04DFu)
        return PR_U68_04DF_ARGUMENT;
    push16(ram, cpu, cpu->bp);                                            /* 04DF PUSH BP; MOV BP,SP; SUB SP,0Ah */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x0Au, 0x8000u);
    cpu->sp = (uint16_t)(cpu->sp - 0x0Au);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6u))] = 0u;               /* 04E5 success := 0; member := DS:[5D96h] */
    cpu->ax = rd16(ram, cpu->ds, 0x5D96u);
    cpu->dx = rd16(ram, cpu->ds, 0x5D98u);
    wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
    wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);
    for (;;) {
        cpu->ax = (uint16_t)(rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u)) | rd16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u)));
        logic_flags(cpu, cpu->ax, 0x8000u);                               /* 04F6 member nil? JE 0545 */
        if (cpu->ax == 0u)
            break;
        byte = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));               /* 04FE CMP BYTE [BP-6],0; JNE 0545 */
        sub_flags(cpu, byte, 0u, 0x80u);
        if (byte != 0u)
            break;
        les_local(ram, cpu, 5u);                                          /* 0504 [BP-0Ah] := the member */
        wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x0Au), cpu->di);
        wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->es);
        set_al(cpu, 1u);                                                  /* 050D MOV AL,1; PUSH AX; MOV AL,64h; PUSH AX */
        push16(ram, cpu, cpu->ax);
        set_al(cpu, 0x64u);
        push16(ram, cpu, cpu->ax);
        status = roll(ram, ram_size, cpu, callee_status);                 /* 0513 LCALL 00B0:0048 */
        if (status != PR_U68_04DF_OK)
            return status;
        les_local(ram, cpu, 0x0Au);                                       /* 0518 CMP AL,ES:[DI+78h]; JA 0530 */
        byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x78u));
        sub_flags(cpu, (uint8_t)cpu->ax, byte, 0x80u);
        if ((uint8_t)cpu->ax <= byte) {
            les_local(ram, cpu, 0x0Au);                                   /* 0521 CMP BYTE ES:[DI+10Ch],0; JNE 0530 */
            byte = rd8(ram, cpu->es, (uint16_t)(cpu->di + 0x10Cu));
            sub_flags(cpu, byte, 0u, 0x80u);
            if (byte == 0u)
                ram[physical(cpu->ss, (uint16_t)(cpu->bp - 6u))] = 1u;   /* 052C success := 1 */
        }
        les_local(ram, cpu, 5u);                                          /* 0530 member := member->link; JMP 04F6 */
        cpu->ax = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x104u));
        cpu->dx = rd16(ram, cpu->es, (uint16_t)(cpu->di + 0x106u));
        wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 5u), cpu->ax);
        wr16(ram, cpu->ss, (uint16_t)(cpu->bp - 3u), cpu->dx);
        if (loopbacks == max_loopbacks) {
            cpu->ip = 0x04F6u;
            return PR_U68_04DF_LIMIT;
        }
        ++loopbacks;
    }
    ram[physical(cpu->ds, 0x6DDFu)] = 0u;                                 /* 0545 MOV BYTE [6DDFh],0 */
    byte = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));                   /* 054A CMP BYTE [BP-6],0; JE 059E */
    sub_flags(cpu, byte, 0u, 0x80u);
    if (byte != 0u) {
        set_al(cpu, rd8(ram, cpu->ds, 0x6AADu));                          /* 0550 +0109(x, y, d) */
        push16(ram, cpu, cpu->ax);
        set_al(cpu, rd8(ram, cpu->ds, 0x6AAEu));
        push16(ram, cpu, cpu->ax);
        set_al(cpu, rd8(ram, cpu->ds, 0x6AAFu));
        push16(ram, cpu, cpu->ax);
        status = mark(ram, ram_size, cpu, 0x055Du, callee_status);
        if (status != PR_U68_04DF_OK)
            return status;
        load_zero_ah(ram, cpu, 0x6AAFu);                                  /* 0560 x + table1[d] */
        cpu->di = cpu->ax;
        set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(cpu->di + 0x2736u)));
        cbw(cpu);
        cpu->dx = cpu->ax;
        set_al(cpu, rd8(ram, cpu->ds, 0x6AADu));
        cbw(cpu);
        cpu->ax = add16(cpu, cpu->ax, cpu->dx);
        push16(ram, cpu, cpu->ax);
        load_zero_ah(ram, cpu, 0x6AAFu);                                  /* 0575 y + table2[d] */
        cpu->di = cpu->ax;
        set_al(cpu, rd8(ram, cpu->ds, (uint16_t)(cpu->di + 0x273Fu)));
        cbw(cpu);
        cpu->dx = cpu->ax;
        set_al(cpu, rd8(ram, cpu->ds, 0x6AAEu));
        cbw(cpu);
        cpu->ax = add16(cpu, cpu->ax, cpu->dx);
        push16(ram, cpu, cpu->ax);
        load_zero_ah(ram, cpu, 0x6AAFu);                                  /* 058A (d + 4) mod 8: ADD; CWD; IDIV CX; XCHG */
        cpu->ax = add16(cpu, cpu->ax, 4u);
        cpu->dx = (cpu->ax & 0x8000u) ? 0xFFFFu : 0u;
        cpu->cx = 8u;
        dividend = (int16_t)cpu->ax;                                      /* DX:AX is AX sign-extended, so a 16-bit IDIV is exact */
        cpu->ax = (uint16_t)(int16_t)(dividend % 8);
        cpu->dx = (uint16_t)(int16_t)(dividend / 8);
        push16(ram, cpu, cpu->ax);
        status = mark(ram, ram_size, cpu, 0x059Bu, callee_status);
        if (status != PR_U68_04DF_OK)
            return status;
    }
    byte = rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));                   /* 059E AL := success via [BP-1] */
    set_al(cpu, byte);
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = byte;
    set_al(cpu, rd8(ram, cpu->ss, (uint16_t)(cpu->bp - 1u)));
    cpu->sp = cpu->bp;                                                    /* 05A7 MOV SP,BP; POP BP; RETF */
    at = physical(cpu->ss, cpu->sp);
    cpu->bp = read16_at(ram, at);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U68_04DF_OK;
}
