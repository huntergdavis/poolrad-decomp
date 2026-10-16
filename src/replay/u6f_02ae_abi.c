#include "poolrad/replay/u6f_02ae_abi.h"
#include "poolrad/replay/slot_a_string_0356_abi.h"

enum reg { AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS };
enum op { PUSH, POP, MOV, IMM, LEA_DI, SUB_SP, LOAD_AL_DS, XOR_AH,
    DEC_AX, FAR_CALL, STORE_RESULT, CMP_RESULT, JE, JNE, FAR_RET,
    NEAR_CALL, NEAR_RET, MOV_AH_39, LDS_SI, CLD, LODS, CMP_AL_127,
    JB, MOV_AL_127, CBW, REP_MOVS, XOR_AL, STOS, LEA_DX, JAE,
    STORE_ERROR, XOR_AX, XCHG_ERROR };
typedef struct {
    uint16_t ip, next;
    unsigned op, a, b;
    uint16_t operand;
} instruction;

#define I(ip,next,op,a,b,n) {ip,next,op,a,b,n}
static const instruction caller[] = {
    I(0x02ae,0x02af,PUSH,BP,0,0), I(0x02af,0x02b1,MOV,BP,SP,0),
    I(0x02b1,0x02b5,SUB_SP,0,0,0x202),
    I(0x02b5,0x02b9,LEA_DI,0,0,0xfefe),
    I(0x02b9,0x02ba,PUSH,SS,0,0), I(0x02ba,0x02bb,PUSH,DI,0,0),
    I(0x02bb,0x02be,IMM,DI,0,0x537a), I(0x02be,0x02bf,PUSH,DS,0,0),
    I(0x02bf,0x02c0,PUSH,DI,0,0), I(0x02c0,0x02c3,IMM,AX,0,1),
    I(0x02c3,0x02c4,PUSH,AX,0,0),
    I(0x02c4,0x02c7,LOAD_AL_DS,0,0,0x537a),
    I(0x02c7,0x02c9,XOR_AH,0,0,0), I(0x02c9,0x02ca,DEC_AX,0,0,0),
    I(0x02ca,0x02cb,PUSH,AX,0,0),
    I(0x02cb,0x02d0,FAR_CALL,0,0,0x0b57),
    I(0x02d0,0x02d5,FAR_CALL,0,0,0x17d6),
    I(0x02d5,0x02da,FAR_CALL,0,0,0x04a2),
    I(0x02da,0x02dd,STORE_RESULT,0,0,0),
    I(0x02dd,0x02e1,CMP_RESULT,0,0,0), I(0x02e1,0x02e3,JE,0,0,0x031f),
    I(0x02e3,0x02e7,CMP_RESULT,0,0,5), I(0x02e7,0x02e9,JE,0,0,0x031f),
    I(0x031f,0x0323,CMP_RESULT,0,0,0), I(0x0323,0x0325,JE,0,0,0x032b),
    I(0x0325,0x0329,CMP_RESULT,0,0,5), I(0x0329,0x032b,JNE,0,0,0x02b5),
    I(0x032b,0x032d,MOV,SP,BP,0), I(0x032d,0x032e,POP,BP,0,0),
    I(0x032e,0,FAR_RET,0,0,0)
};
static const instruction system_ops[] = {
    I(0x17d6,0x17d7,PUSH,BP,0,0), I(0x17d7,0x17d9,MOV,BP,SP,0),
    I(0x17d9,0x17dd,SUB_SP,0,0,0x80),
    I(0x17dd,0x17e0,NEAR_CALL,0,0,0x1800),
    I(0x17e0,0x17e2,MOV_AH_39,0,0,0),
    I(0x17e2,0x17e5,NEAR_CALL,0,0,0x181b),
    I(0x17e5,0x17e7,MOV,SP,BP,0), I(0x17e7,0x17e8,POP,BP,0,0),
    I(0x17e8,0,FAR_RET,0,0,4),
    I(0x1800,0x1801,PUSH,DS,0,0), I(0x1801,0x1804,LDS_SI,0,0,6),
    I(0x1804,0x1807,LEA_DI,0,0,0xff80),
    I(0x1807,0x1808,PUSH,SS,0,0), I(0x1808,0x1809,POP,ES,0,0),
    I(0x1809,0x180a,CLD,0,0,0), I(0x180a,0x180b,LODS,0,0,0),
    I(0x180b,0x180d,CMP_AL_127,0,0,0), I(0x180d,0x180f,JB,0,0,0x1811),
    I(0x180f,0x1811,MOV_AL_127,0,0,0), I(0x1811,0x1812,CBW,0,0,0),
    I(0x1812,0x1814,MOV,CX,AX,0), I(0x1814,0x1816,REP_MOVS,0,0,0),
    I(0x1816,0x1818,XOR_AL,0,0,0), I(0x1818,0x1819,STOS,0,0,0),
    I(0x1819,0x181a,POP,DS,0,0), I(0x181a,0,NEAR_RET,0,0,0),
    I(0x181b,0x181c,PUSH,DS,0,0), I(0x181c,0x181f,LEA_DX,0,0,0xff80),
    I(0x181f,0x1820,PUSH,SS,0,0), I(0x1820,0x1821,POP,DS,0,0),
    I(0x1823,0x1824,POP,DS,0,0), I(0x1824,0x1826,JAE,0,0,0x1829),
    I(0x1826,0x1829,STORE_ERROR,0,0,0), I(0x1829,0,NEAR_RET,0,0,0),
    I(0x04a2,0x04a4,XOR_AX,0,0,0), I(0x04a4,0x04a8,XCHG_ERROR,0,0,0),
    I(0x04a8,0,FAR_RET,0,0,0)
};
#undef I

/* Same bounded-span and defined-flag conventions as string_0356_abi.c. */
static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t length)
{
    size_t start = ((size_t)seg << 4) + off;
    if ((size_t)off + length > 0x10000u || start > s->ram_size ||
        length > s->ram_size - start) return NULL;
    return s->ram + start;
}

static uint16_t word(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint16_t logic_flags(uint16_t old, unsigned value, unsigned bits)
{
    unsigned parity = value & 255u;
    uint16_t f = (uint16_t)(old & (uint16_t)~0x08d5u);
    parity ^= parity >> 4; parity ^= parity >> 2; parity ^= parity >> 1;
    if (!(parity & 1u)) f |= 4u;
    if (!(value & (bits == 8u ? 255u : 65535u))) f |= 0x40u;
    if (value & (1u << (bits - 1u))) f |= 0x80u;
    return f;
}

static uint16_t sub_flags(uint16_t old, unsigned a, unsigned b,
                          unsigned bits)
{
    unsigned mask = bits == 8u ? 255u : 65535u;
    unsigned sign = 1u << (bits - 1u), value = (a - b) & mask;
    uint16_t f = logic_flags(old, value, bits);
    if (a < b) f |= 1u;
    if ((a ^ b ^ value) & 0x10u) f |= 0x10u;
    if ((a ^ b) & (a ^ value) & sign) f |= 0x800u;
    return f;
}

pr_u6f_02ae_status pr_u6f_02ae_step(
    pr_slot_a_directory_state *s,
    pr_slot_a_directory_service service, void *context,
    pr_slot_a_directory_write *journal, size_t capacity)
{
    const instruction *table, *in = NULL;
    size_t count, i;
    pr_slot_a_directory_cpu *c;
    uint8_t *p, *source, *destination;
    uint16_t value, next, saved_cf;
    int outer_return;
    if (!s || (capacity && !journal)) return PR_U6F_02AE_ARGUMENT;
    if (!s->ram || s->ram_size > 0x100000u)
        return PR_U6F_02AE_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs == 0x2a8eu && c->ip == 0x02e9u)
        return PR_U6F_02AE_BLOCKED_MESSAGE;
    if (c->cs == 0x131cu && c->ip >= 0x0b57u && c->ip < 0x0b98u) {
        pr_slot_a_string_0356_report report;
        pr_slot_a_string_0356_status result =
            pr_slot_a_string_0356_prefix_step(s, &report);
        if (result == PR_SLOT_A_STRING_0356_STEPPED) return PR_U6F_02AE_STEPPED;
        if (result == PR_SLOT_A_STRING_0356_BLOCKED_MEMORY)
            return PR_U6F_02AE_BLOCKED_MEMORY;
        return PR_U6F_02AE_BLOCKED_PC;
    }
    if (c->cs == 0x131cu && c->ip == 0x1821u) {
        pr_slot_a_directory_status result = pr_slot_a_directory_step(
            s, service, context, journal, capacity);
        if (result == PR_SLOT_A_DIRECTORY_OK) return PR_U6F_02AE_STEPPED;
        if (result == PR_SLOT_A_DIRECTORY_BLOCKED_DOS_SERVICE)
            return PR_U6F_02AE_BLOCKED_DOS_SERVICE;
        if (result == PR_SLOT_A_DIRECTORY_BAD_EVENT) return PR_U6F_02AE_BAD_EVENT;
        return PR_U6F_02AE_BLOCKED_PC;
    }
    if (c->cs == 0x2a8eu) {
        table = caller; count = sizeof caller / sizeof caller[0];
    } else if (c->cs == 0x131cu) {
        table = system_ops; count = sizeof system_ops / sizeof system_ops[0];
    } else return PR_U6F_02AE_BLOCKED_PC;
    for (i = 0; i < count; ++i)
        if (table[i].ip == c->ip) { in = table + i; break; }
    if (!in) return PR_U6F_02AE_BLOCKED_PC;
    next = in->next;
    outer_return = c->cs == 0x2a8eu && in->op == FAR_RET;
    {
        uint16_t *r[] = {&c->ax, &c->bx, &c->cx, &c->dx, &c->si, &c->di,
                         &c->bp, &c->sp, &c->cs, &c->ds, &c->es, &c->ss};
        switch (in->op) {
        case PUSH:
            p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            put_word(p, *r[in->a]); c->sp = (uint16_t)(c->sp - 2u); break;
        case POP:
            p = span(s, c->ss, c->sp, 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            value = word(p); c->sp = (uint16_t)(c->sp + 2u);
            *r[in->a] = value; break;
        case MOV: *r[in->a] = *r[in->b]; break;
        case IMM: *r[in->a] = in->operand; break;
        case LEA_DI: c->di = (uint16_t)(c->bp + in->operand); break;
        case LEA_DX: c->dx = (uint16_t)(c->bp + in->operand); break;
        case SUB_SP:
            c->flags = sub_flags(c->flags, c->sp, in->operand, 16u);
            c->sp = (uint16_t)(c->sp - in->operand); break;
        case LOAD_AL_DS:
            p = span(s, c->ds, in->operand, 1u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            c->ax = (uint16_t)((c->ax & 0xff00u) | *p); break;
        case XOR_AH:
            c->ax &= 0x00ffu; c->flags = logic_flags(c->flags, 0u, 8u); break;
        case DEC_AX:
            saved_cf = c->flags & 1u;
            c->flags = (uint16_t)((sub_flags(c->flags, c->ax, 1u, 16u) &
                                  (uint16_t)~1u) | saved_cf);
            --c->ax; break;
        case FAR_CALL:
            p = span(s, c->ss, (uint16_t)(c->sp - 4u), 4u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            put_word(p + 2u, c->cs); put_word(p, next);
            c->sp = (uint16_t)(c->sp - 4u); c->cs = 0x131cu;
            next = in->operand; break;
        case NEAR_CALL:
            p = span(s, c->ss, (uint16_t)(c->sp - 2u), 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            put_word(p, next); c->sp = (uint16_t)(c->sp - 2u);
            next = in->operand; break;
        case FAR_RET: case NEAR_RET:
            p = span(s, c->ss, c->sp, in->op == FAR_RET ? 4u : 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            next = word(p);
            if (in->op == FAR_RET) c->cs = word(p + 2u);
            c->sp = (uint16_t)(c->sp + (in->op == FAR_RET ? 4u : 2u) +
                               in->operand); break;
        case STORE_RESULT: case CMP_RESULT:
            p = span(s, c->ss, (uint16_t)(c->bp - 2u), 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            if (in->op == STORE_RESULT) put_word(p, c->ax);
            else c->flags = sub_flags(c->flags, word(p), in->operand, 16u);
            break;
        case JE: if (c->flags & 0x40u) next = in->operand; break;
        case JNE: if (!(c->flags & 0x40u)) next = in->operand; break;
        case JB: if (c->flags & 1u) next = in->operand; break;
        case JAE: if (!(c->flags & 1u)) next = in->operand; break;
        case MOV_AH_39: c->ax = (uint16_t)((c->ax & 0xffu) | 0x3900u); break;
        case LDS_SI:
            p = span(s, c->ss, (uint16_t)(c->bp + in->operand), 4u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            c->si = word(p); c->ds = word(p + 2u); break;
        case CLD: c->flags &= (uint16_t)~0x0400u; break;
        case CMP_AL_127:
            c->flags = sub_flags(c->flags, c->ax & 255u, 127u, 8u); break;
        case MOV_AL_127: c->ax = (uint16_t)((c->ax & 0xff00u) | 127u); break;
        case CBW:
            c->ax = (uint16_t)((c->ax & 255u) |
                               (c->ax & 128u ? 0xff00u : 0u)); break;
        case LODS: case STOS:
            if (c->flags & 0x0400u) return PR_U6F_02AE_BLOCKED_PC;
            p = span(s, in->op == LODS ? c->ds : c->es,
                     in->op == LODS ? c->si : c->di, 1u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            if (in->op == LODS) {
                c->ax = (uint16_t)((c->ax & 0xff00u) | *p); ++c->si;
            } else { *p = (uint8_t)c->ax; ++c->di; }
            break;
        case REP_MOVS:
            if (c->flags & 0x0400u) return PR_U6F_02AE_BLOCKED_PC;
            if (c->cx) {
                size_t length = c->cx;
                source = span(s, c->ds, c->si, length);
                destination = span(s, c->es, c->di, length);
                if (!source || !destination) return PR_U6F_02AE_BLOCKED_MEMORY;
                for (i = 0; i < length; ++i) {
                    destination[i] = source[i];
                    ++c->si; ++c->di; --c->cx;
                }
            }
            break;
        case XOR_AL:
            c->ax &= 0xff00u; c->flags = logic_flags(c->flags, 0u, 8u); break;
        case STORE_ERROR: case XCHG_ERROR:
            p = span(s, c->ds, 0x4426u, 2u);
            if (!p) return PR_U6F_02AE_BLOCKED_MEMORY;
            value = word(p); put_word(p, c->ax);
            if (in->op == XCHG_ERROR) c->ax = value;
            break;
        case XOR_AX:
            c->ax = 0; c->flags = logic_flags(c->flags, 0u, 16u); break;
        default: return PR_U6F_02AE_BLOCKED_PC;
        }
    }
    c->ip = next;
    return outer_return ? PR_U6F_02AE_RETURNED : PR_U6F_02AE_STEPPED;
}
