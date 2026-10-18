/* GAME.OVR unit 0x7F +0B93 (raw CPU, composing SYSTEM string store and UpCase); docs/re/U7F_COIN_TYPE_0B93.md. */
#include "poolrad/recovered/overlay_u7f_coin_type_0b93_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#include <string.h>

#define MASK (PR_U7F_0B93_RAM_SIZE - 1u)
#define ENTRY_IP 0x0B93u
#define STORE_ENTRY 0x0B25u
#define UPCASE_ENTRY 0x182Au

_Static_assert(sizeof(pr_u7f_0b93_cpu) == sizeof(pr_system_0b25_cpu), "SYSTEM 0B25 CPU layout differs");
_Static_assert(sizeof(pr_u7f_0b93_cpu) == sizeof(pr_system_leaf_cpu), "SYSTEM leaf CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u7f_0b93_cpu *cpu;
    uint16_t entry;
    int *callee_status;
    pr_u7f_0b93_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void push16(machine *m, uint16_t value)
{
    size_t at;
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    at = physical(m->cpu->ss, m->cpu->sp);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint8_t frame8(const machine *m, uint16_t disp)
{
    return m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp + disp))];
}

static void set_frame8(machine *m, uint16_t disp, uint8_t value)
{
    m->ram[physical(m->cpu->ss, (uint16_t)(m->cpu->bp + disp))] = value;
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

static void sub_flags(pr_u7f_0b93_cpu *cpu, uint16_t left, uint16_t right, unsigned bits)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t result = ((uint32_t)left - right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= 0x0001u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & sign) cpu->flags |= 0x0800u;
}

/* INC r/m8: CF kept. */
static void inc8_flags(pr_u7f_0b93_cpu *cpu, uint8_t old)
{
    uint8_t result = (uint8_t)(old + 1u);
    cpu->flags &= (uint16_t)~0x08D4u;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if ((old & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (result == 0x80u) cpu->flags |= 0x0800u;
}

/* XOR AH,AH then INC AX: CF = 0 from the XOR; live when the UpCase call site is rewritten. */
static void zero_ah_inc_ax(pr_u7f_0b93_cpu *cpu)
{
    uint16_t old = (uint16_t)(cpu->ax & 0x00FFu), result = (uint16_t)(old + 1u);
    cpu->ax = result;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((old & 0x0Fu) == 0x0Fu) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (result == 0x8000u) cpu->flags |= 0x0800u;
}

static void set_al(pr_u7f_0b93_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* LCALL at original offset `site` to `entry_ip` (arguments already pushed), run by `kind` 0 (store) or 1 (UpCase). */
static int far_call(machine *m, uint16_t site, int kind)
{
    pr_u7f_0b93_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs, entry_ip = kind ? UPCASE_ENTRY : STORE_ENTRY;
    int status;
    cpu->ip = at;
    if (m->ram[physical(cpu->cs, at)] != 0x9Au || read16_at(m->ram, physical(cpu->cs, (uint16_t)(at + 1u))) != entry_ip) {
        m->stop = PR_U7F_0B93_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16_at(m->ram, physical(caller_cs, (uint16_t)(at + 3u)));
    cpu->ip = entry_ip;
    if (kind) {
        pr_system_leaf_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_upcase_182a(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    } else {
        pr_system_0b25_cpu leaf;
        memcpy(&leaf, cpu, sizeof leaf);
        status = (int)pr_system_strstore_0b25(m->ram, m->size, &leaf);
        memcpy(cpu, &leaf, sizeof leaf);
    }
    if (m->callee_status != NULL)
        *m->callee_status = status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U7F_0B93_TRANSFER;
        return 0;
    }
    return 1;
}

/* MOV DI,literal; PUSH CS; PUSH DI; LES DI,[BP+6]; PUSH ES; PUSH DI; MOV AX,0FFh; PUSH AX; LCALL store at `site`. */
static int store_literal(machine *m, uint16_t literal, uint16_t site)
{
    pr_u7f_0b93_cpu *cpu = m->cpu;
    size_t at;
    cpu->di = literal;
    push16(m, cpu->cs);
    push16(m, cpu->di);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = read16_at(m->ram, at);
    cpu->es = read16_at(m->ram, (at + 2u) & MASK);
    push16(m, cpu->es);
    push16(m, cpu->di);
    cpu->ax = 0x00FFu;
    push16(m, cpu->ax);
    return far_call(m, site, 0);
}

pr_u7f_0b93_status pr_u7f_coin_type_0b93_cpu(uint8_t *ram, size_t ram_size, pr_u7f_0b93_cpu *cpu, unsigned max_loopbacks,
                                             int *callee_status)
{
    static const struct { uint8_t letter; uint16_t literal, site, after; uint8_t result; } cases[5] = {
        {0x50u, 0x0B66u, 0x0C34u, 0x0C39u, 4u}, {0x45u, 0x0B70u, 0x0C51u, 0x0C56u, 2u}, {0x53u, 0x0B7Au, 0x0C6Eu, 0x0C73u, 1u},
        {0x43u, 0x0B82u, 0x0C8Bu, 0x0C90u, 0u}, {0x4Au, 0x0B8Au, 0x0CA8u, 0x0CADu, 6u}
    };
    machine m;
    unsigned loopbacks = 0u;
    size_t at;
    uint8_t letter;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U7F_0B93_RAM_SIZE)
        return PR_U7F_0B93_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu; m.entry = cpu->ip; m.callee_status = callee_status; m.stop = PR_U7F_0B93_OK;
    push16(&m, cpu->bp);                                                  /* +0B93 PUSH BP; MOV BP,SP; SUB SP,102h */
    cpu->bp = cpu->sp;
    sub_flags(cpu, cpu->sp, 0x0102u, 16u);
    cpu->sp = (uint16_t)(cpu->sp - 0x0102u);
    at = physical(cpu->ss, (uint16_t)(cpu->bp + 0x0Au));                 /* +0B9A LES DI,[BP+0Ah]; PUSH ES; PUSH DI */
    cpu->di = read16_at(ram, at);
    cpu->es = read16_at(ram, (at + 2u) & MASK);
    push16(&m, cpu->es);
    push16(&m, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x0101u);                              /* +0B9F LEA DI,[BP-101h]; PUSH SS; PUSH DI */
    push16(&m, cpu->ss);
    push16(&m, cpu->di);
    cpu->ax = 0x00FFu;                                                    /* +0BA5 MOV AX,0FFh; PUSH AX; LCALL store */
    push16(&m, cpu->ax);
    if (!far_call(&m, 0x0BA9u, 0))
        return m.stop;
    set_frame8(&m, (uint16_t)-0x0102, 1u);                                /* +0BAE MOV BYTE [BP-102h],1 */
    for (;;) {
        uint8_t value;
        cpu->ax = frame8(&m, (uint16_t)-0x0102);                          /* +0BB3 MOV AL,[BP-102h]; XOR AH,AH; MOV DI,AX */
        cpu->di = cpu->ax;
        value = frame8(&m, (uint16_t)(cpu->di - 0x0101u));                /* +0BBB CMP BYTE [BP+DI-101h],20h; JNE +0BC8 */
        sub_flags(cpu, value, 0x20u, 8u);
        if (value != 0x20u)
            break;
        value = frame8(&m, (uint16_t)-0x0102);                            /* +0BC2 INC BYTE [BP-102h]: live at a LIMIT stop */
        inc8_flags(cpu, value);
        set_frame8(&m, (uint16_t)-0x0102, (uint8_t)(value + 1u));
        if (loopbacks++ == max_loopbacks) {                               /* +0BC6 JMP +0BB3 */
            cpu->ip = here(&m, 0x0BB3u);
            return PR_U7F_0B93_LIMIT;
        }
    }
    cpu->ax = frame8(&m, (uint16_t)-0x0102);                              /* +0BC8 MOV AL,[BP-102h]; XOR AH,AH; MOV DI,AX */
    cpu->di = cpu->ax;
    set_al(cpu, frame8(&m, (uint16_t)(cpu->di - 0x0101u)));               /* +0BD0 MOV AL,[BP+DI-101h]; CMP AL,47h; JNE +0C22 */
    letter = (uint8_t)cpu->ax;
    sub_flags(cpu, letter, 0x47u, 8u);
    if (letter == 0x47u) {
        set_al(cpu, frame8(&m, (uint16_t)-0x0102));                       /* +0BD8 MOV AL,[BP-102h]; XOR AH,AH; INC AX; MOV DI,AX */
        zero_ah_inc_ax(cpu);
        cpu->di = cpu->ax;
        set_al(cpu, frame8(&m, (uint16_t)(cpu->di - 0x0101u)));           /* +0BE1 MOV AL,[BP+DI-101h]; PUSH AX; LCALL UpCase */
        push16(&m, cpu->ax);
        if (!far_call(&m, 0x0BE6u, 1))
            return m.stop;
        sub_flags(cpu, (uint8_t)cpu->ax, 0x45u, 8u);                      /* +0BEB CMP AL,45h; JNE +0C08 */
        if ((uint8_t)cpu->ax == 0x45u) {
            set_frame8(&m, (uint16_t)-1, 5u);                             /* +0BEF MOV BYTE [BP-1],5 */
            if (!store_literal(&m, 0x0B5Au, 0x0C01u))
                return m.stop;
        } else {
            if (!store_literal(&m, 0x0B60u, 0x0C16u))                     /* +0C08 */
                return m.stop;
            set_frame8(&m, (uint16_t)-1, 3u);                             /* +0C1B MOV BYTE [BP-1],3 */
        }
    } else {
        for (unsigned k = 0u; k < 5u; ++k) {                              /* +0C22.. CMP AL,letter; JNE next */
            sub_flags(cpu, letter, cases[k].letter, 8u);
            if (letter == cases[k].letter) {
                if (!store_literal(&m, cases[k].literal, cases[k].site))
                    return m.stop;
                set_frame8(&m, (uint16_t)-1, cases[k].result);
                break;
            }
        }
    }
    set_al(cpu, frame8(&m, (uint16_t)-1));                                /* +0CB1 MOV AL,[BP-1]; MOV SP,BP; POP BP; RETF 8 */
    cpu->sp = cpu->bp;
    cpu->bp = read16_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    at = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(ram, at);
    cpu->cs = read16_at(ram, (at + 2u) & MASK);
    cpu->sp = (uint16_t)(cpu->sp + 4u + 8u);
    return PR_U7F_0B93_OK;
}
