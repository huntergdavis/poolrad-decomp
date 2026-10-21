/* START.EXE resident 0802:0000, the FreeMem wrapper, raw CPU (docs/re/RES_0802_0000_CPU.md). */
#include "poolrad/recovered/resident_0802_0000_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"

#define MASK (PR_R0802_0000_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)

typedef pr_r0802_0000_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static unsigned parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}

pr_r0802_0000_status pr_r0802_0000(uint8_t *ram, size_t ram_size, cpu_t *c, unsigned max_steps,
                                   pr_r0802_0000_diagnostic *diagnostic)
{
    pr_r0802_0000_diagnostic local, *d = diagnostic ? diagnostic : &local;
    pr_system_freemem_cpu m;
    uint16_t home, target, seg;
    int status;
    if (ram == NULL || c == NULL || ram_size != PR_R0802_0000_RAM_SIZE || c->ip != 0x0000u || c->sp < 0x002Cu || c->sp > 0xFFF6u)
        return PR_R0802_0000_ARGUMENT;
    d->callee_status = -1;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 0000 PUSH BP; MOV BP,SP */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 8u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 10u));   /* 0003 LES DI,[BP+8] */
    push(ram, c, rd16(ram, c->es, (uint16_t)(c->di + 2u)));           /* 0006 PUSH ES:[DI+2] */
    push(ram, c, rd16(ram, c->es, c->di));                            /* 000A PUSH ES:[DI] */
    {   unsigned a = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)), full = a + 7u, r = full & 0xFFFFu;   /* 000D MOV AX,[BP+6]; ADD AX,7 */
        c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) |
                              (full > 0xFFFFu ? CF : 0u) | ((a ^ 7u ^ r) & AF) | ((~(a ^ 7u) & (a ^ r) & 0x8000u) ? OF : 0u));
        r &= 0xFFF8u;                                                 /* 0013 AND AX,0FFF8h: CF = OF = AF = 0 */
        c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r));
        c->ax = (uint16_t)r; }
    push(ram, c, c->ax);                                              /* 0016 PUSH AX */
    home = c->cs; target = rd16(ram, c->cs, 0x0018u); seg = rd16(ram, c->cs, 0x001Au);   /* 0017 LCALL 0AF8:0254 */
    push(ram, c, c->cs); push(ram, c, 0x001Cu);
    c->cs = seg; c->ip = target;
    COPY_CPU(&m, c);
    status = target == 0x0254u ? (int)pr_system_freemem_0254(ram, ram_size, &m, max_steps) : -1;
    COPY_CPU(c, &m);
    if (status == PR_SYSTEM_FREEMEM_RUNERROR) { d->callee_status = status; return PR_R0802_0000_RUNERROR; }
    if (status == PR_SYSTEM_FREEMEM_LIMIT) { d->callee_status = status; return PR_R0802_0000_LIMIT; }
    if (status != PR_SYSTEM_FREEMEM_RETURNED || c->cs != home || c->ip != 0x001Cu) { d->callee_status = status; return PR_R0802_0000_CALLEE; }
    c->bp = pop(ram, c);                                              /* 001C POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 001D RETF 6 */
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_R0802_0000_RETURNED;
}
