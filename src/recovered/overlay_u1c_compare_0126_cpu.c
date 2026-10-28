/* ECL COMPARE caller composition; see docs/re/U1C_COMPARE_0126_CPU.md. */
#include "poolrad/recovered/overlay_u1c_compare_0126_cpu.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include "poolrad/recovered/overlay_u25_compare_strings_1815_cpu.h"
#include "poolrad/recovered/overlay_u25_compare_words_18eb_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK ((1u << 20) - 1u)
typedef struct {
    uint8_t *ram;
    pr_u1c_compare_0126_cpu *c;
    const pr_u1c_compare_0126_limits *limits;
    pr_u1c_compare_0126_diagnostic *diagnostic;
    pr_u1c_compare_0126_observer observe;
    void *context;
} machine;

_Static_assert(sizeof(pr_u1c_compare_0126_cpu) == sizeof(pr_u25_1815_cpu), "compare CPU layout");
_Static_assert(sizeof(pr_u1c_compare_0126_cpu) == sizeof(pr_u25_18eb_cpu), "compare CPU layout");
_Static_assert(sizeof(pr_u1c_compare_0126_cpu) == sizeof(pr_u25_slot_value_cpu), "slot CPU layout");
_Static_assert(sizeof(pr_u1c_compare_0126_cpu) == sizeof(pr_system_0b25_cpu), "string CPU layout");

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint8_t read8(const machine *m, uint16_t segment, uint16_t offset)
{
    return m->ram[physical(segment, offset)];
}
static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    size_t a = physical(segment, offset);
    return (uint16_t)(m->ram[a] | ((uint16_t)m->ram[(a + 1u) & MASK] << 8));
}
static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t a = physical(segment, offset);
    m->ram[a] = (uint8_t)value;
    m->ram[(a + 1u) & MASK] = (uint8_t)(value >> 8);
}
static void push(machine *m, uint16_t value)
{
    m->c->sp = (uint16_t)(m->c->sp - 2u);
    write16(m, m->c->ss, m->c->sp, value);
}
static uint16_t pop(machine *m)
{
    uint16_t value = read16(m, m->c->ss, m->c->sp);
    m->c->sp = (uint16_t)(m->c->sp + 2u);
    return value;
}
static int parity(uint8_t v)
{
    v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1);
    return (v & 1u) == 0u;
}
static uint16_t subtract(pr_u1c_compare_0126_cpu *c, uint16_t a, uint16_t b, unsigned bits)
{
    uint16_t mask = bits == 8u ? 0xFFu : 0xFFFFu;
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t r = (uint16_t)((a - b) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 1u;
    if (parity((uint8_t)r)) c->flags |= 4u;
    if ((a ^ b ^ r) & 0x10u) c->flags |= 0x10u;
    if (r == 0u) c->flags |= 0x40u;
    if (r & sign) c->flags |= 0x80u;
    if ((a ^ b) & (a ^ r) & sign) c->flags |= 0x800u;
    return r;
}
static void step(machine *m, uint16_t ip)
{
    m->c->ip = ip;
    if (m->observe != NULL) m->observe(m->ram, MASK + 1u, m->c, m->context);
}

/* Resolve the actual loaded overlay stub; keep the caller's real far frame. */
static pr_u1c_compare_0126_status call(machine *m, uint16_t site, uint16_t offset,
                                     uint16_t entry, pr_u1c_compare_0126_callee kind)
{
    pr_u1c_compare_0126_cpu *c = m->c;
    uint16_t caller = c->cs, next = (uint16_t)(site + 5u);
    uint16_t target = read16(m, caller, (uint16_t)(site + 3u));
    int status = 0, transfer = 0;
    if (read8(m, caller, site) != 0x9Au || read16(m, caller, (uint16_t)(site + 1u)) != offset)
        return PR_U1C_0126_CODE;
    if (entry != 0xFFFFu) {
        if (read8(m, target, offset) != 0xEAu || read16(m, target, (uint16_t)(offset + 1u)) != entry)
            return PR_U1C_0126_CODE;
        target = read16(m, target, (uint16_t)(offset + 3u));
    } else entry = offset;
    push(m, caller); push(m, next);
    c->cs = target; c->ip = entry;
    switch (kind) {
    case PR_U1C_0126_OPERANDS:
        status = (int)pr_u25_load_operands_0000(m->ram, MASK + 1u, c, m->limits->system_cs,
                    m->limits->max_operands, m->limits->max_string_iterations);
        transfer = status == PR_U25_0000_TRANSFER;
        break;
    case PR_U1C_0126_SLOT_VALUE: {
        pr_u25_slot_value_cpu leaf;
        memcpy(&leaf, c, sizeof leaf);
        status = (int)pr_u25_slot_value_0136(m->ram, MASK + 1u, &leaf);
        memcpy(c, &leaf, sizeof leaf);
        transfer = status == PR_U25_SLOT_VALUE_TRANSFER;
        break;
    }
    case PR_U1C_0126_STRING_COPY: {
        pr_system_0b25_cpu leaf;
        memcpy(&leaf, c, sizeof leaf);
        status = (int)pr_system_strstore_0b25(m->ram, MASK + 1u, &leaf);
        memcpy(c, &leaf, sizeof leaf);
        break;
    }
    case PR_U1C_0126_STRING_COMPARE: {
        pr_u25_1815_cpu leaf;
        size_t budget = m->limits->max_compare_instructions;
        memcpy(&leaf, c, sizeof leaf);
        status = (int)pr_u25_compare_strings_1815_cpu(m->ram, MASK + 1u, &leaf, &budget);
        memcpy(c, &leaf, sizeof leaf);
        transfer = status == PR_U25_1815_TRANSFER;
        break;
    }
    case PR_U1C_0126_WORD_COMPARE: {
        pr_u25_18eb_cpu leaf;
        memcpy(&leaf, c, sizeof leaf);
        status = (int)pr_u25_compare_words_18eb_cpu(m->ram, MASK + 1u, &leaf);
        memcpy(c, &leaf, sizeof leaf);
        transfer = status == PR_U25_18EB_TRANSFER;
        break;
    }
    default: return PR_U1C_0126_ARGUMENT;
    }
    if (m->diagnostic != NULL) {
        m->diagnostic->callee = kind;
        m->diagnostic->status = status;
    }
    if (transfer) return PR_U1C_0126_TRANSFER;
    if (status != 0) return PR_U1C_0126_CALLEE;
    if (c->cs != caller || c->ip != next) return PR_U1C_0126_TRANSFER;
    return PR_U1C_0126_RETURNED;
}

#define STEP(ip) step(&m, (ip))
#define CALL(site, offset, entry, kind) do { \
    pr_u1c_compare_0126_status result; STEP(site); \
    result = call(&m, site, offset, entry, kind); \
    if (result != PR_U1C_0126_RETURNED) return result; \
} while (0)

pr_u1c_compare_0126_status pr_u1c_compare_0126(
    uint8_t *ram, size_t size, pr_u1c_compare_0126_cpu *c,
    const pr_u1c_compare_0126_limits *limits, pr_u1c_compare_0126_diagnostic *diagnostic,
    pr_u1c_compare_0126_observer observe, void *context)
{
    machine m;
    uint16_t return_ip, return_cs;
    int strings;
    if (ram == NULL || c == NULL || limits == NULL || size != MASK + 1u || c->ip != 0x0126u ||
        c->sp < 0x0436u || c->sp > 0xFFFBu || limits->max_operands == 0u ||
        limits->max_string_iterations == 0u || limits->max_compare_instructions == 0u)
        return PR_U1C_0126_ARGUMENT;
    m = (machine){ram, c, limits, diagnostic, observe, context};
    if (diagnostic != NULL) { diagnostic->callee = PR_U1C_0126_NONE; diagnostic->status = -1; }
    return_ip = read16(&m, c->ss, c->sp);
    return_cs = read16(&m, c->ss, (uint16_t)(c->sp + 2u));
    STEP(0x0126u); push(&m, c->bp);
    STEP(0x0127u); c->bp = c->sp;
    STEP(0x0129u); c->sp = subtract(c, c->sp, 0x0204u, 16u);
    STEP(0x012Du); c->ax = (uint16_t)((c->ax & 0xFF00u) | 2u);
    STEP(0x012Fu); push(&m, c->ax);
    CALL(0x0130u, 0x002Au, 0x0000u, PR_U1C_0126_OPERANDS);
    STEP(0x0135u); (void)subtract(c, read8(&m, c->ds, 0x7006u), 0x80u, 8u);
    STEP(0x013Au); strings = (c->flags & 1u) == 0u;
    if (!strings) {
        STEP(0x013Cu); (void)subtract(c, read8(&m, c->ds, 0x7007u), 0x80u, 8u);
        STEP(0x0141u); strings = (c->flags & 1u) == 0u;
    }
    if (strings) {
        STEP(0x0143u); c->di = 0x70C8u;
        STEP(0x0146u); push(&m, c->ds);
        STEP(0x0147u); push(&m, c->di);
        STEP(0x0148u); c->di = (uint16_t)(c->bp - 0x0104u);
        STEP(0x014Cu); push(&m, c->ss);
        STEP(0x014Du); push(&m, c->di);
        STEP(0x014Eu); c->ax = 0xFFu;
        STEP(0x0151u); push(&m, c->ax);
        CALL(0x0152u, 0x0B25u, 0xFFFFu, PR_U1C_0126_STRING_COPY);
        STEP(0x0157u); c->di = 0x71C8u;
        STEP(0x015Au); push(&m, c->ds);
        STEP(0x015Bu); push(&m, c->di);
        STEP(0x015Cu); c->di = (uint16_t)(c->bp - 0x0204u);
        STEP(0x0160u); push(&m, c->ss);
        STEP(0x0161u); push(&m, c->di);
        STEP(0x0162u); c->ax = 0xFFu;
        STEP(0x0165u); push(&m, c->ax);
        CALL(0x0166u, 0x0B25u, 0xFFFFu, PR_U1C_0126_STRING_COPY);
        STEP(0x016Bu); c->di = (uint16_t)(c->bp - 0x0104u);
        STEP(0x016Fu); push(&m, c->ss);
        STEP(0x0170u); push(&m, c->di);
        STEP(0x0171u); c->di = (uint16_t)(c->bp - 0x0204u);
        STEP(0x0175u); push(&m, c->ss);
        STEP(0x0176u); push(&m, c->di);
        CALL(0x0177u, 0x006Bu, 0x1815u, PR_U1C_0126_STRING_COMPARE);
        STEP(0x017Cu);
    } else {
        STEP(0x017Eu); c->ax = (uint16_t)((c->ax & 0xFF00u) | 1u);
        STEP(0x0180u); push(&m, c->ax);
        CALL(0x0181u, 0x0025u, 0x0136u, PR_U1C_0126_SLOT_VALUE);
        STEP(0x0186u); write16(&m, c->ss, (uint16_t)(c->bp - 2u), c->ax);
        STEP(0x0189u); c->ax = (uint16_t)((c->ax & 0xFF00u) | 2u);
        STEP(0x018Bu); push(&m, c->ax);
        CALL(0x018Cu, 0x0025u, 0x0136u, PR_U1C_0126_SLOT_VALUE);
        STEP(0x0191u); write16(&m, c->ss, (uint16_t)(c->bp - 4u), c->ax);
        STEP(0x0194u); push(&m, read16(&m, c->ss, (uint16_t)(c->bp - 2u)));
        STEP(0x0197u); push(&m, read16(&m, c->ss, (uint16_t)(c->bp - 4u)));
        CALL(0x019Au, 0x0070u, 0x18EBu, PR_U1C_0126_WORD_COMPARE);
    }
    STEP(0x019Fu); c->sp = c->bp;
    STEP(0x01A1u); c->bp = pop(&m);
    STEP(0x01A2u); c->ip = pop(&m); c->cs = pop(&m);
    return c->ip == return_ip && c->cs == return_cs ? PR_U1C_0126_RETURNED : PR_U1C_0126_TRANSFER;
}
