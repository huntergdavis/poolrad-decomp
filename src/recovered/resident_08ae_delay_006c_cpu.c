/* Complete RES_08AE [006C,00DC); docs/re/RES_08AE_DELAY_006C_CPU.md. */
#include "poolrad/recovered/resident_08ae_delay_006c_cpu.h"
#define MASK (PR_R08AE_006C_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
typedef pr_r08ae_006c_cpu cpu_t;
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
/* ADD/ADC/SUB/SBB/CMP: all six arithmetic flags, carry-in for ADC/SBB. */
static uint16_t alu(cpu_t *c, uint16_t a, uint16_t b, int sub, unsigned carry)
{
    unsigned r = sub ? ((unsigned)a - b - carry) & 0xFFFFu : ((unsigned)a + b + carry) & 0xFFFFu;
    uint16_t f = (uint16_t)(c->flags & ~(CF | PF | AF | ZF | SF | OF));
    unsigned p = r & 255u; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    if (sub ? (unsigned)a < (unsigned)b + carry : (unsigned)a + b + carry > 0xFFFFu) f |= CF;
    if (!(p & 1u)) f |= PF;
    if ((a ^ b ^ r) & 16u) f |= AF;
    if (!r) f |= ZF;
    if (r & 0x8000u) f |= SF;
    if ((sub ? (unsigned)(a ^ b) : ~(unsigned)(a ^ b)) & (a ^ r) & 0x8000u) f |= OF;
    c->flags = f; return (uint16_t)r;
}
static uint16_t local(const uint8_t *m, const cpu_t *c, int disp)
{ return get(m, c->ss, (uint16_t)(c->bp + (unsigned)disp)); }
static void set_local(uint8_t *m, const cpu_t *c, int disp, uint16_t v)
{ put(m, c->ss, (uint16_t)(c->bp + (unsigned)disp), v); }
static int sf_ne_of(const cpu_t *c) { return !!(c->flags & SF) != !!(c->flags & OF); }
/* XOR r,r: CF=OF=0, SF/ZF/PF from the zero result; AF left cleared. */
static void xor_zero(cpu_t *c)
{ c->flags = (uint16_t)((c->flags & ~(CF | PF | AF | ZF | SF | OF)) | PF | ZF); }
static int call_time(uint8_t *m, cpu_t *c, uint16_t site, pr_r08ae_006c_time_fn t, void *x)
{
    /* `site` is the CALL: PUSH CS (0E) precedes it, E8 rel16 with target CS:0000. */
    unsigned rel = (unsigned)get(m, c->cs, (uint16_t)(site + 1u));
    if (m[phys(c->cs, (uint16_t)(site - 1u))] != 0x0Eu || m[phys(c->cs, site)] != 0xE8u ||
        ((site + 3u + rel) & 0xFFFFu) != 0u) return 2;
    push(m, c, c->cs);
    push(m, c, (uint16_t)(site + 3u));
    c->ip = 0;
    if (t == NULL || t(x, m, c) != 0) return 3;
    c->ip = pop(m, c); c->cs = pop(m, c);
    return 0;
}
pr_r08ae_006c_status pr_r08ae_delay_006c_cpu(
    uint8_t *m, size_t size, cpu_t *c, pr_r08ae_006c_time_fn time, void *x,
    unsigned max_steps)
{
    unsigned steps = 0; int r;
    if (m == NULL || c == NULL || size != PR_R08AE_006C_RAM_SIZE || c->ip != 0x006Cu ||
        c->sp < 0x000Eu || c->sp > 0xFFFAu) return PR_R08AE_006C_ARGUMENT;
    for (;;) {
        if (steps == max_steps) return PR_R08AE_006C_LIMIT;
        ++steps;
        switch (c->ip) {
        case 0x006Cu: push(m, c, c->bp); c->ip = 0x006Du; break;
        case 0x006Du: c->bp = c->sp; c->ip = 0x006Fu; break;
        case 0x006Fu: c->sp = alu(c, c->sp, 8u, 1, 0); c->ip = 0x0072u; break;
        case 0x0072u: /* PUSH CS; CALL 0 are checked and executed together */
        case 0x0089u:
            r = call_time(m, c, (uint16_t)(c->ip + 1u), time, x);
            if (r == 2) return PR_R08AE_006C_CODE;
            if (r == 3) return PR_R08AE_006C_TIME;
            break;
        case 0x0076u: c->cx = c->ax; c->ip = 0x0078u; break;
        case 0x0078u: c->bx = c->dx; c->ip = 0x007Au; break;
        case 0x007Au: c->ax = local(m, c, 6); c->ip = 0x007Du; break;
        case 0x007Du: c->dx = 0; xor_zero(c); c->ip = 0x007Fu; break;
        case 0x007Fu: c->ax = alu(c, c->ax, c->cx, 0, 0); c->ip = 0x0081u; break;
        case 0x0081u: c->dx = alu(c, c->dx, c->bx, 0, c->flags & CF); c->ip = 0x0083u; break;
        case 0x0083u: set_local(m, c, -4, c->ax); c->ip = 0x0086u; break;
        case 0x0086u: set_local(m, c, -2, c->dx); c->ip = 0x0089u; break;
        case 0x008Du: set_local(m, c, -8, c->ax); c->ip = 0x0090u; break;
        case 0x0090u: set_local(m, c, -6, c->dx); c->ip = 0x0093u; break;
        case 0x0093u: c->ax = local(m, c, 6); c->ip = 0x0096u; break;
        case 0x0096u: c->dx = 0; xor_zero(c); c->ip = 0x0098u; break;
        case 0x0098u: c->cx = c->ax; c->ip = 0x009Au; break;
        case 0x009Au: c->bx = c->dx; c->ip = 0x009Cu; break;
        case 0x009Cu: c->ax = local(m, c, -4); c->ip = 0x009Fu; break;
        case 0x009Fu: c->dx = local(m, c, -2); c->ip = 0x00A2u; break;
        case 0x00A2u: c->ax = alu(c, c->ax, local(m, c, -8), 1, 0); c->ip = 0x00A5u; break;
        case 0x00A5u: c->dx = alu(c, c->dx, local(m, c, -6), 1, c->flags & CF); c->ip = 0x00A8u; break;
        case 0x00A8u: alu(c, c->dx, c->bx, 1, 0); c->ip = 0x00AAu; break;
        case 0x00AAu: /* JG */
            c->ip = (!(c->flags & ZF) && !sf_ne_of(c)) ? 0x00B2u : 0x00ACu; break;
        case 0x00ACu: /* JL */ c->ip = sf_ne_of(c) ? 0x00C4u : 0x00AEu; break;
        case 0x00AEu: alu(c, c->ax, c->cx, 1, 0); c->ip = 0x00B0u; break;
        case 0x00B0u: /* JBE */ c->ip = (c->flags & (CF | ZF)) ? 0x00C4u : 0x00B2u; break;
        case 0x00B2u: c->ax = local(m, c, -4); c->ip = 0x00B5u; break;
        case 0x00B5u: c->dx = local(m, c, -2); c->ip = 0x00B8u; break;
        case 0x00B8u: c->ax = alu(c, c->ax, 1u, 0, 0); c->ip = 0x00BBu; break;
        case 0x00BBu: c->dx = alu(c, c->dx, 0u, 0, c->flags & CF); c->ip = 0x00BEu; break;
        case 0x00BEu: set_local(m, c, -8, c->ax); c->ip = 0x00C1u; break;
        case 0x00C1u: set_local(m, c, -6, c->dx); c->ip = 0x00C4u; break;
        case 0x00C4u: c->ax = local(m, c, -8); c->ip = 0x00C7u; break;
        case 0x00C7u: c->dx = local(m, c, -6); c->ip = 0x00CAu; break;
        case 0x00CAu: alu(c, c->dx, local(m, c, -2), 1, 0); c->ip = 0x00CDu; break;
        case 0x00CDu: /* JG */
            c->ip = (!(c->flags & ZF) && !sf_ne_of(c)) ? 0x00D6u : 0x00CFu; break;
        case 0x00CFu: /* JL */ c->ip = sf_ne_of(c) ? 0x0089u : 0x00D1u; break;
        case 0x00D1u: alu(c, c->ax, local(m, c, -4), 1, 0); c->ip = 0x00D4u; break;
        case 0x00D4u: /* JB */ c->ip = (c->flags & CF) ? 0x0089u : 0x00D6u; break;
        case 0x00D6u: c->sp = c->bp; c->ip = 0x00D8u; break;
        case 0x00D8u: c->bp = pop(m, c); c->ip = 0x00D9u; break;
        case 0x00D9u: /* RETF 2 */
            c->ip = pop(m, c); c->cs = pop(m, c); c->sp = (uint16_t)(c->sp + 2u);
            return PR_R08AE_006C_OK;
        default: return PR_R08AE_006C_CODE;
        }
    }
}
