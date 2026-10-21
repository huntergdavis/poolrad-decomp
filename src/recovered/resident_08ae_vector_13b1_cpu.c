/* Complete RES_08AE [13B1,13EA); docs/re/RES_08AE_VECTOR_13B1_CPU.md. */
#include "poolrad/recovered/resident_08ae_vector_13b1_cpu.h"
#define MASK (PR_R08AE_13B1_RAM_SIZE - 1u)
typedef pr_r08ae_13b1_cpu cpu_t;
static size_t phys(uint16_t s, uint16_t o) { return (((size_t)s << 4) + o) & MASK; }
static uint16_t get(const uint8_t *m, uint16_t s, uint16_t o)
{
    size_t p = phys(s, o);
    return (uint16_t)(m[p] | ((unsigned)m[(p + 1u) & MASK] << 8));
}
static void put(uint8_t *m, uint16_t s, uint16_t o, uint16_t v)
{
    size_t p = phys(s, o); m[p] = (uint8_t)v; m[(p + 1u) & MASK] = (uint8_t)(v >> 8);
}
static void push(uint8_t *m, cpu_t *c, uint16_t v)
{ c->sp = (uint16_t)(c->sp - 2u); put(m, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *m, cpu_t *c)
{ uint16_t v = get(m, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
/* INT 21h: hardware frame, DOS service, return with RETF 2 and the handler's flags. */
static int int21(uint8_t *m, cpu_t *c, uint16_t next, const pr_r08ae_13b1_env *e)
{
    uint16_t out = 0;
    push(m, c, c->flags); push(m, c, c->cs); push(m, c, next);
    c->flags = (uint16_t)(c->flags & ~0x0300u);              /* IF and TF cleared on entry */
    if (e == NULL || e->dos == NULL || e->dos(e->context, m, c, &out) != 0) return 1;
    c->ip = pop(m, c); c->cs = pop(m, c); c->sp = (uint16_t)(c->sp + 2u);
    c->flags = out;
    return 0;
}
static int out_port(const pr_r08ae_13b1_env *e, uint16_t port, uint8_t value)
{ return e == NULL || e->port_out == NULL || e->port_out(e->context, port, value) != 0; }
/* AND r8,imm8: CF=OF=0, SF/ZF/PF from the result, AF cleared. */
static void and_flags(cpu_t *c, uint8_t r)
{
    unsigned p = r; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    c->flags = (uint16_t)(c->flags & ~0x08D5u);
    if (!(p & 1u)) c->flags |= 0x0004u;
    if (!r) c->flags |= 0x0040u;
    if (r & 0x80u) c->flags |= 0x0080u;
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
pr_r08ae_13b1_status pr_r08ae_vector_13b1_cpu(
    uint8_t *m, size_t size, cpu_t *c, const pr_r08ae_13b1_env *e)
{
    uint8_t v;
    if (m == NULL || c == NULL || size != PR_R08AE_13B1_RAM_SIZE || c->ip != 0x13B1u ||
        c->sp < 0x000Au || c->sp > 0xFFFCu) return PR_R08AE_13B1_ARGUMENT;
    for (;;) {
        switch (c->ip) {
        case 0x13B1u: push(m, c, c->ds); c->ip = 0x13B2u; break;
        case 0x13B2u: c->ax = get(m, c->cs, 0x13B3u); c->ip = 0x13B5u; break;   /* relocated segment */
        case 0x13B5u: c->ds = c->ax; c->ip = 0x13B7u; break;
        case 0x13B7u: push(m, c, c->ds); c->ip = 0x13B8u; break;
        case 0x13B8u: c->dx = get(m, c->cs, 0x1378u); c->ip = 0x13BDu; break;   /* saved old vector, before 137C */
        case 0x13BDu: c->ax = get(m, c->cs, 0x137Au); c->ip = 0x13C1u; break;
        case 0x13C1u: c->ds = c->ax; c->ip = 0x13C3u; break;
        case 0x13C3u: c->ax = 0x2508u; c->ip = 0x13C6u; break;
        case 0x13C6u:
            if (int21(m, c, 0x13C8u, e)) { c->ip = 0x13C6u; return PR_R08AE_13B1_ENV_DOS; }
            c->ip = 0x13C8u; break;
        case 0x13C8u: c->ds = pop(m, c); c->ip = 0x13C9u; break;
        case 0x13C9u: c->ax = 0xFFFFu; c->ip = 0x13CCu; break;
        case 0x13CCu:
            if (out_port(e, 0x40u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT;
            c->ip = 0x13CEu; break;
        case 0x13CEu: set_al(c, (uint8_t)(c->ax >> 8)); c->ip = 0x13D0u; break;
        case 0x13D0u:
            if (out_port(e, 0x40u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT;
            c->ip = 0x13D2u; break;
        case 0x13D2u:
            if (e == NULL || e->port_in == NULL || e->port_in(e->context, 0x61u, &v) != 0)
                return PR_R08AE_13B1_ENV_PORT;
            set_al(c, v); c->ip = 0x13D4u; break;
        case 0x13D4u: set_al(c, (uint8_t)((uint8_t)c->ax & 0xFCu)); and_flags(c, (uint8_t)c->ax); c->ip = 0x13D6u; break;
        case 0x13D6u:
            if (out_port(e, 0x61u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT;
            c->ip = 0x13D8u; break;
        case 0x13D8u: set_al(c, 0x9Fu); c->ip = 0x13DAu; break;
        case 0x13DAu: if (out_port(e, 0xC0u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT; c->ip = 0x13DCu; break;
        case 0x13DCu: set_al(c, 0xBFu); c->ip = 0x13DEu; break;
        case 0x13DEu: if (out_port(e, 0xC0u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT; c->ip = 0x13E0u; break;
        case 0x13E0u: set_al(c, 0xDFu); c->ip = 0x13E2u; break;
        case 0x13E2u: if (out_port(e, 0xC0u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT; c->ip = 0x13E4u; break;
        case 0x13E4u: set_al(c, 0xFFu); c->ip = 0x13E6u; break;
        case 0x13E6u: if (out_port(e, 0xC0u, (uint8_t)c->ax)) return PR_R08AE_13B1_ENV_PORT; c->ip = 0x13E8u; break;
        case 0x13E8u: c->ds = pop(m, c); c->ip = 0x13E9u; break;
        case 0x13E9u: c->ip = pop(m, c); c->cs = pop(m, c); return PR_R08AE_13B1_OK;
        default: return PR_R08AE_13B1_ARGUMENT;
        }
    }
}
