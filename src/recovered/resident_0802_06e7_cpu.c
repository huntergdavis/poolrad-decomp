/* START.EXE resident 0802:06E7, close both archive files, raw CPU (docs/re/RES_0802_06E7_CPU.md). */
#include "poolrad/recovered/resident_0802_06e7_cpu.h"
#include "poolrad/recovered/system_iocheck_04a9_cpu.h"

#define MASK (PR_R0802_06E7_RAM_SIZE - 1u)
#define CONTINUE (-1)
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)

typedef pr_r0802_06e7_cpu cpu_t;

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

/* LCALL at `at` (segment word read live from the code) into Close 1491 or IOCheck 04A9; must return to at+5. */
static int far_call(uint8_t *ram, size_t size, cpu_t *c, pr_system_close_service service, void *context, uint16_t at,
                    pr_r0802_06e7_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    int status, ok;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    d->site = at;
    if (target == 0x1491u) {
        pr_system_close_cpu m; COPY_CPU(&m, c); status = pr_system_close_1491(ram, size, &m, service, context); COPY_CPU(c, &m);
        d->callee_status = status;
        if (status == PR_SYSTEM_CLOSE_SERVICE) return PR_R0802_06E7_SERVICE;
        ok = status == PR_SYSTEM_CLOSE_RETURNED;
    } else if (target == 0x04A9u) {
        pr_system_iocheck_cpu m; COPY_CPU(&m, c); status = pr_system_iocheck_04a9(ram, size, &m); COPY_CPU(c, &m);
        d->callee_status = status;
        if (status == PR_SYSTEM_IOCHECK_RUNERROR) return PR_R0802_06E7_RUNERROR;
        ok = status == PR_SYSTEM_IOCHECK_RETURNED;
    } else {
        return PR_R0802_06E7_CALLEE;                                   /* unreachable: the code is immutable */
    }
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return PR_R0802_06E7_CALLEE;
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

static int close_checked(uint8_t *ram, size_t size, cpu_t *c, pr_system_close_service service, void *context, uint16_t disp,
                         uint16_t lcall, pr_r0802_06e7_diagnostic *d)
{
    int s;
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + disp));               /* LES DI,[BP+disp] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + disp + 2u));
    push(ram, c, c->es); push(ram, c, c->di);                         /* PUSH ES; PUSH DI */
    if ((s = far_call(ram, size, c, service, context, lcall, d)) != CONTINUE)              /* LCALL Close */
        return s;
    return far_call(ram, size, c, service, context, (uint16_t)(lcall + 5u), d);            /* LCALL IOCheck */
}

pr_r0802_06e7_status pr_r0802_06e7(uint8_t *ram, size_t ram_size, cpu_t *c, pr_system_close_service service, void *context,
                                   pr_r0802_06e7_diagnostic *diagnostic)
{
    pr_r0802_06e7_diagnostic local, *d = diagnostic ? diagnostic : &local;
    int s;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_R0802_06E7_RAM_SIZE || c->ip != 0x06E7u ||
        c->sp < 0x000Cu || c->sp > 0xFFF4u)
        return PR_R0802_06E7_ARGUMENT;
    d->site = 0; d->callee_status = -1;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 06E7 PUSH BP; MOV BP,SP */
    if ((s = close_checked(ram, ram_size, c, service, context, 0x0Au, 0x06EFu, d)) != CONTINUE)   /* 06EA file B */
        return (pr_r0802_06e7_status)s;
    if ((s = close_checked(ram, ram_size, c, service, context, 0x06u, 0x06FEu, d)) != CONTINUE)   /* 06F9 file A */
        return (pr_r0802_06e7_status)s;
    c->bp = pop(ram, c);                                              /* 0708 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 0709 RETF 8 */
    c->sp = (uint16_t)(c->sp + 8u);
    return PR_R0802_06E7_RETURNED;
}
