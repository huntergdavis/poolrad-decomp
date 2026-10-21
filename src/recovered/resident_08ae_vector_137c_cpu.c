/* Complete RES_08AE [137C,13B1); docs/re/RES_08AE_VECTOR_137C_CPU.md. */
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#define MASK (PR_R08AE_137C_RAM_SIZE - 1u)
typedef pr_r08ae_137c_cpu cpu_t;
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
static int int21(uint8_t *m, cpu_t *c, uint16_t next, const pr_r08ae_137c_env *e)
{
    uint16_t out = 0;
    push(m, c, c->flags); push(m, c, c->cs); push(m, c, next);
    c->flags = (uint16_t)(c->flags & ~0x0300u);              /* IF and TF cleared on entry */
    if (e == NULL || e->dos == NULL || e->dos(e->context, m, c, &out) != 0) return 1;
    c->ip = pop(m, c); c->cs = pop(m, c); c->sp = (uint16_t)(c->sp + 2u);
    c->flags = out;
    return 0;
}
static int out_port(const pr_r08ae_137c_env *e, uint16_t port, uint8_t value)
{ return e == NULL || e->port_out == NULL || e->port_out(e->context, port, value) != 0; }
pr_r08ae_137c_status pr_r08ae_vector_137c_cpu(
    uint8_t *m, size_t size, cpu_t *c, const pr_r08ae_137c_env *e)
{
    if (m == NULL || c == NULL || size != PR_R08AE_137C_RAM_SIZE || c->ip != 0x137Cu ||
        c->sp < 0x000Cu || c->sp > 0xFFFCu) return PR_R08AE_137C_ARGUMENT;
    for (;;) {
        switch (c->ip) {
        case 0x137Cu: push(m, c, c->ds); c->ip = 0x137Du; break;
        case 0x137Du: push(m, c, c->es); c->ip = 0x137Eu; break;
        case 0x137Eu: c->ax = get(m, c->cs, 0x137Fu); c->ip = 0x1381u; break;   /* relocated segment */
        case 0x1381u: c->ds = c->ax; c->ip = 0x1383u; break;
        case 0x1383u: c->ax = 0x3508u; c->ip = 0x1386u; break;
        case 0x1386u:
            if (int21(m, c, 0x1388u, e)) { c->ip = 0x1386u; return PR_R08AE_137C_ENV_DOS; }
            c->ip = 0x1388u; break;
        case 0x1388u: put(m, c->cs, 0x1378u, c->bx); c->ip = 0x138Du; break;
        case 0x138Du: put(m, c->cs, 0x137Au, c->es); c->ip = 0x1392u; break;
        case 0x1392u: push(m, c, c->ds); c->ip = 0x1393u; break;
        case 0x1393u: c->dx = get(m, c->cs, 0x1394u); c->ip = 0x1396u; break;    /* relocated segment */
        case 0x1396u: c->ds = c->dx; c->ip = 0x1398u; break;
        case 0x1398u: c->dx = 0x13EAu; c->ip = 0x139Bu; break;
        case 0x139Bu: c->ax = 0x2508u; c->ip = 0x139Eu; break;
        case 0x139Eu:
            if (int21(m, c, 0x13A0u, e)) { c->ip = 0x139Eu; return PR_R08AE_137C_ENV_DOS; }
            c->ip = 0x13A0u; break;
        case 0x13A0u: c->ds = pop(m, c); c->ip = 0x13A1u; break;
        case 0x13A1u: c->ax = (uint16_t)((c->ax & 0xFF00u) | 0xB6u); c->ip = 0x13A3u; break;
        case 0x13A3u:
            if (out_port(e, 0x43u, (uint8_t)c->ax)) return PR_R08AE_137C_ENV_PORT;
            c->ip = 0x13A5u; break;
        case 0x13A5u: c->ax = 0x13B1u; c->ip = 0x13A8u; break;
        case 0x13A8u:
            if (out_port(e, 0x40u, (uint8_t)c->ax)) return PR_R08AE_137C_ENV_PORT;
            c->ip = 0x13AAu; break;
        case 0x13AAu: c->ax = (uint16_t)((c->ax & 0xFF00u) | (c->ax >> 8)); c->ip = 0x13ACu; break;
        case 0x13ACu:
            if (out_port(e, 0x40u, (uint8_t)c->ax)) return PR_R08AE_137C_ENV_PORT;
            c->ip = 0x13AEu; break;
        case 0x13AEu: c->es = pop(m, c); c->ip = 0x13AFu; break;
        case 0x13AFu: c->ds = pop(m, c); c->ip = 0x13B0u; break;
        case 0x13B0u: c->ip = pop(m, c); c->cs = pop(m, c); return PR_R08AE_137C_OK;
        default: return PR_R08AE_137C_ARGUMENT;
        }
    }
}
