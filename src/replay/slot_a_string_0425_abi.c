#include "poolrad/replay/slot_a_string_0425_abi.h"

enum reg { AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS, FS, GS,
           AL = AX, AH = AX + 16, DL = DX };
enum operation { PUSH, POP, MOV, MOV8, IMM, IMM8, LOAD8, STORE8,
    FAR, LEA, XOR8, SUBI, INC_MEM, CMP_MEM, JUMP, CALL, RET, CLD,
    STOS, STORE_IMM, CMP_IMM_MEM, LOAD_TABLE, LOAD_INDEX, STORE_INDEX,
    NEAR, LOAD_SS_BX, CMP_IMM8, SUB_IMM8 };
enum condition { ALWAYS, ABOVE, BELOW, BELOW_EQUAL, EQUAL, NOT_EQUAL,
                 GREATER, GREATER_EQUAL };
typedef struct {
    uint16_t ip, next;
    uint8_t op, a, b;
    int16_t operand;
} instruction;

#define I(ip, next, op, a, b, n) {ip, next, op, a, b, n}
#include "slot_a_string_0425_instructions.inc"
#undef I

static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t width)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + width > 0x10000u || start > s->ram_size ||
        width > s->ram_size - start) return NULL;
    return s->ram + start;
}

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint8_t byte_reg(uint16_t **r, unsigned reg)
{
    return (uint8_t)(*r[reg & 15u] >> (reg & 16u ? 8u : 0u));
}

static void set_byte(uint16_t **r, unsigned reg, uint8_t value)
{
    uint16_t *p = r[reg & 15u];
    if (reg & 16u) *p = (uint16_t)((*p & 0x00FFu) | ((uint16_t)value << 8));
    else *p = (uint16_t)((*p & 0xFF00u) | value);
}

static uint16_t result_flags(uint16_t old, unsigned result, unsigned bits)
{
    unsigned p = result & 255u, mask = bits == 8u ? 255u : 65535u;
    uint16_t flags = (uint16_t)(old & (uint16_t)~0x08D5u);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    if (!(p & 1u)) flags |= 4u;
    if (!(result & mask)) flags |= 0x40u;
    if (result & (1u << (bits - 1u))) flags |= 0x80u;
    return flags;
}

static uint16_t arithmetic_flags(uint16_t old, unsigned a, unsigned b,
                                  unsigned bits, int subtract)
{
    unsigned mask = bits == 8u ? 255u : 65535u;
    unsigned sign = 1u << (bits - 1u);
    unsigned value = subtract ? (a - b) & mask : (a + b) & mask;
    uint16_t f = result_flags(old, value, bits);
    if (subtract ? a < b : a + b > mask) f |= 1u;
    if ((a ^ b ^ value) & 0x10u) f |= 0x10u;
    if ((subtract ? (a ^ b) : ~(a ^ b)) & (a ^ value) & sign) f |= 0x800u;
    return f;
}

static int branch(uint16_t flags, unsigned condition)
{
    int cf = !!(flags & 1u), zf = !!(flags & 0x40u);
    int sf = !!(flags & 0x80u), of = !!(flags & 0x800u);
    switch (condition) {
    case ALWAYS: return 1;
    case ABOVE: return !cf && !zf;
    case BELOW: return cf;
    case BELOW_EQUAL: return cf || zf;
    case EQUAL: return zf;
    case NOT_EQUAL: return !zf;
    case GREATER: return !zf && sf == of;
    case GREATER_EQUAL: return sf == of;
    default: return 0;
    }
}

pr_slot_a_string_0425_status pr_slot_a_string_0425_step(
    pr_slot_a_directory_state *s, pr_slot_a_string_0425_report *report)
{
    const instruction *table, *in = NULL;
    size_t count, i;
    uint8_t *p;
    uint16_t value, next, saved_cf, offset;
    pr_slot_a_directory_cpu *c;
    if (!s || !report) return PR_SLOT_A_STRING_0425_ARGUMENT;
    *report = (pr_slot_a_string_0425_report){0};
    if (!s->ram || s->ram_size > 0x100000u)
        return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x2A8Eu) {
        if (c->ip == 0x1974u) {
            *report = (pr_slot_a_string_0425_report){
                1u, 0x6Fu, 0x197Eu, 0x0AF8u, 0x0B25u, 0x1983u, 1u};
            return PR_SLOT_A_STRING_0425_BLOCKED_CALLER_CONTINUATION;
        }
        if (c->ip != 0x196Fu) return PR_SLOT_A_STRING_0425_BLOCKED_PC;
        p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
        if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
        put_word(p + 2u, c->cs); put_word(p, 0x1974u);
        c->sp = (uint16_t)(c->sp - 4u);
        c->cs = 0x1026u; c->ip = 0x0425u;
        return PR_SLOT_A_STRING_0425_STEPPED;
    }
    if (c->cs == 0x1026u) {
        table = resident; count = sizeof resident / sizeof resident[0];
    } else if (c->cs == 0x131Cu) {
        table = system_ops; count = sizeof system_ops / sizeof system_ops[0];
    } else return PR_SLOT_A_STRING_0425_BLOCKED_PC;
    for (i = 0u; i < count; ++i) if (table[i].ip == c->ip) { in = table + i; break; }
    if (!in) {
        /* Existing original +0356 / B25 / B57 / C10 instruction semantics.
         * The shared report preserves Delete's original dependency addresses. */
        pr_slot_a_string_0356_status status = pr_slot_a_string_0356_step(s, report);
        switch (status) {
        case PR_SLOT_A_STRING_0356_STEPPED: return PR_SLOT_A_STRING_0425_STEPPED;
        case PR_SLOT_A_STRING_0356_BLOCKED_DELETE: return PR_SLOT_A_STRING_0425_BLOCKED_DELETE;
        case PR_SLOT_A_STRING_0356_BLOCKED_MEMORY: return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
        default: return PR_SLOT_A_STRING_0425_BLOCKED_PC;
        }
    }
    {
        uint16_t *r[] = {&c->ax, &c->bx, &c->cx, &c->dx, &c->si, &c->di,
                         &c->bp, &c->sp, &c->cs, &c->ds, &c->es, &c->ss,
                         &c->fs, &c->gs};
        next = in->next;
        switch (in->op) {
        case PUSH:
            p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            put_word(p, *r[in->a]); c->sp = (uint16_t)(c->sp - 2u); break;
        case POP:
            p = span(s, c->ss, c->sp, 2u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            value = word(p); c->sp = (uint16_t)(c->sp + 2u); *r[in->a] = value; break;
        case MOV: *r[in->a] = *r[in->b]; break;
        case MOV8: set_byte(r, in->a, byte_reg(r, in->b)); break;
        case IMM: *r[in->a] = (uint16_t)in->operand; break;
        case IMM8: set_byte(r, in->a, (uint8_t)in->operand); break;
        case LEA: c->di = (uint16_t)(c->bp + in->operand); break;
        case LOAD8: case STORE8: case STORE_IMM: case INC_MEM:
        case CMP_MEM: case CMP_IMM_MEM: case LOAD_INDEX: case STORE_INDEX:
            offset = (uint16_t)(c->bp + in->operand +
                ((in->op == LOAD_INDEX || in->op == STORE_INDEX) ? (int)c->di : 0));
            p = span(s, c->ss, offset, 1u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            if (in->op == LOAD8 || in->op == LOAD_INDEX) set_byte(r, in->a, *p);
            else if (in->op == STORE8 || in->op == STORE_INDEX) *p = byte_reg(r, in->a);
            else if (in->op == STORE_IMM) *p = in->a;
            else if (in->op == CMP_MEM)
                c->flags = arithmetic_flags(c->flags, byte_reg(r, in->a), *p, 8u, 1);
            else if (in->op == CMP_IMM_MEM)
                c->flags = arithmetic_flags(c->flags, *p, in->a, 8u, 1);
            else {
                saved_cf = c->flags & 1u;
                c->flags = (uint16_t)((arithmetic_flags(c->flags, *p, 1u, 8u, 0) & ~1u) | saved_cf);
                ++*p;
            }
            break;
        case LOAD_TABLE: case LOAD_SS_BX:
            p = span(s, in->op == LOAD_TABLE ? c->ds : c->ss,
                (uint16_t)((in->op == LOAD_TABLE ? c->di : c->bx) + in->operand), 1u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            set_byte(r, in->a, *p); break;
        case FAR:
            p = span(s, c->ss, (uint16_t)(*r[in->b] + in->operand), 4u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            *r[in->a] = word(p); c->es = word(p + 2u); break;
        case XOR8:
            set_byte(r, in->a, 0u); c->flags = result_flags(c->flags, 0u, 8u); break;
        case SUBI:
            c->flags = arithmetic_flags(c->flags, *r[in->a], (uint16_t)in->operand, 16u, 1);
            *r[in->a] = (uint16_t)(*r[in->a] - in->operand); break;
        case CMP_IMM8: case SUB_IMM8:
            value = byte_reg(r, in->a);
            c->flags = arithmetic_flags(c->flags, value, (uint8_t)in->operand, 8u, 1);
            if (in->op == SUB_IMM8) set_byte(r, in->a, (uint8_t)(value - in->operand));
            break;
        case JUMP: if (branch(c->flags, in->a)) next = (uint16_t)in->operand; break;
        case CALL: case NEAR:
            value = in->op == CALL ? 4u : 2u;
            p = span(s, c->ss, (uint16_t)(c->sp - value), value);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            if (in->op == CALL) { put_word(p + 2u, c->cs); c->cs = 0x131Cu; }
            put_word(p, next); c->sp = (uint16_t)(c->sp - value);
            next = (uint16_t)in->operand; break;
        case RET:
            p = span(s, c->ss, c->sp, 4u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            next = word(p); c->cs = word(p + 2u);
            c->sp = (uint16_t)(c->sp + 4u + (uint16_t)in->operand); break;
        case CLD: c->flags &= (uint16_t)~0x0400u; break;
        case STOS:
            if (c->flags & 0x400u) return PR_SLOT_A_STRING_0425_BLOCKED_PC;
            p = span(s, c->es, c->di, 1u);
            if (!p) return PR_SLOT_A_STRING_0425_BLOCKED_MEMORY;
            *p = byte_reg(r, AL); ++c->di; break;
        default: return PR_SLOT_A_STRING_0425_BLOCKED_PC;
        }
        c->ip = next;
    }
    return PR_SLOT_A_STRING_0425_STEPPED;
}
