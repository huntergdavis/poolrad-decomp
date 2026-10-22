/* START.EXE resident 08AE:0000 time in hundredths, raw CPU (docs/re/RES_08AE_TIME_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R08AE_TIME_CPU_##s
typedef pr_r08ae_time_cpu_status pr_status_t;
typedef pr_r08ae_time_cpu pr_cpu_t;
typedef struct { pr_r08ae_time_cpu_diagnostic *d; pr_dos_gettime_service service; void *context; } ctx_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(pr_bus *b, size_t at)
{
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static uint16_t rd16(pr_bus *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static void wr16(pr_bus *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    pr_bus_write8(b, at, (uint8_t)v); pr_bus_write8(b, (at + 1u) & MASK, (uint8_t)(v >> 8u));
}
static void push(pr_bus *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void szp(pr_cpu_t *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t arith(pr_cpu_t *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = sub ? (uint32_t)l - r - carry : (uint32_t)l + r + carry;
    uint16_t res = (uint16_t)(wide & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (sub ? (uint32_t)l < (uint32_t)r + carry : wide > mask) c->flags |= 1u;
    if ((l ^ r ^ res) & 0x10u) c->flags |= 0x10u;
    if ((sub ? (l ^ r) & (l ^ res) : (uint16_t)~(l ^ r) & (l ^ res)) & sign) c->flags |= 0x800u;
    szp(c, res, bits); return res;
}
static uint16_t logic(pr_cpu_t *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1).
 * AF is architecturally undefined; it follows pinned DOSBox-X normal/386 (AF := 1 for a nonzero count),
 * because in this body SHR +2555's AF survives FillChar to the RETURN. Unicorn sets AF := 0: a declared split. */
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    /* Pinned MULW: SF/AF kept, ZF from AX, PF is the even parity of all four bytes of DX:AX. */
    {
        uint32_t fold = p;
        fold ^= fold >> 16; fold ^= fold >> 8; fold ^= fold >> 4; fold ^= fold >> 2; fold ^= fold >> 1;
        c->flags = (uint16_t)((c->flags & ~0x0845u) | (c->dx != 0u ? 0x0801u : 0u) |
                                (c->ax == 0u ? 0x0040u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
}
/* Dos.GetTime 0A32:00A2 (runtime coverage), entered with the far frame pushed; the INT 21h service is passed through. */
static int call_gettime(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_dos_gettime_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_dos_gettime_00a2(b->ram, PR_BUS_RAM_SIZE, &inner, dg->service, dg->context);
    memcpy(c, &inner, sizeof inner);
    dg->d->gettime_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
/* SYSTEM LongMul 0AF8:0A5B on the flat RAM (its accepted raw model), entered with the far frame pushed. */
static int call_longmul(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_longmul_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_longmul_0a5b(b->ram, PR_BUS_RAM_SIZE, &inner);
    memcpy(c, &inner, sizeof inner);
    dg->d->longmul_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
pr_r08ae_time_cpu_status pr_r08ae_time_0000_cpu(pr_bus *b, size_t ram_size, pr_r08ae_time_cpu *c,
                                                pr_dos_gettime_service service, void *service_context,
                                                pr_r08ae_time_cpu_diagnostic *d)
{
    pr_r08ae_time_cpu_diagnostic fallback = {-1, -1};
    ctx_t context; ctx_t *dg = &context;
    uint16_t caller; size_t loops = 0, max_loopbacks = 0;
    if (b == NULL || b->ram == NULL || b->ega != NULL || c == NULL || service == NULL || ram_size != PR_BUS_RAM_SIZE ||
        c->ip != 0x0000u)
        return STATUS(ARGUMENT);
    context.d = d != NULL ? d : &fallback; context.service = service; context.context = service_context;
    *context.d = fallback;
    caller = c->cs;
    (void)loops; (void)max_loopbacks;
    for (;;) {
        switch (c->ip) {
        case 0x0000: push(b,c,c->bp);c->ip=0x0001u;break; /* push bp */
        case 0x0001: c->bp=(uint16_t)(c->sp);c->ip=0x0003u;break; /* mov bp, sp */
        case 0x0003: c->sp=(uint16_t)(arith(c,c->sp,0x000Cu,0u,16u,1u));c->ip=0x0006u;break; /* sub sp, 0xc */
        case 0x0006: c->di=(uint16_t)(c->bp+0xFFFAu);c->ip=0x0009u;break; /* lea di, [bp - 6] */
        case 0x0009: push(b,c,c->ss);c->ip=0x000Au;break; /* push ss */
        case 0x000A: push(b,c,c->di);c->ip=0x000Bu;break; /* push di */
        case 0x000B: c->di=(uint16_t)(c->bp+0xFFF8u);c->ip=0x000Eu;break; /* lea di, [bp - 8] */
        case 0x000E: push(b,c,c->ss);c->ip=0x000Fu;break; /* push ss */
        case 0x000F: push(b,c,c->di);c->ip=0x0010u;break; /* push di */
        case 0x0010: c->di=(uint16_t)(c->bp+0xFFF6u);c->ip=0x0013u;break; /* lea di, [bp - 0xa] */
        case 0x0013: push(b,c,c->ss);c->ip=0x0014u;break; /* push ss */
        case 0x0014: push(b,c,c->di);c->ip=0x0015u;break; /* push di */
        case 0x0015: c->di=(uint16_t)(c->bp+0xFFF4u);c->ip=0x0018u;break; /* lea di, [bp - 0xc] */
        case 0x0018: push(b,c,c->ss);c->ip=0x0019u;break; /* push ss */
        case 0x0019: push(b,c,c->di);c->ip=0x001Au;break; /* push di */
        case 0x001A: push(b,c,c->cs);push(b,c,0x001Fu);c->cs=rd16(b,c->cs,0x001Du);c->ip=0x00A2u;{int s=call_gettime(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x001Fu)return STATUS(TRANSFER);break; /* lcall 0xa32, 0xa2 */
        case 0x001F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0022u;break; /* mov ax, word ptr [bp - 6] */
        case 0x0022: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x0024u;break; /* xor dx, dx */
        case 0x0024: c->cx=(uint16_t)(0x7E40u);c->ip=0x0027u;break; /* mov cx, 0x7e40 */
        case 0x0027: c->bx=(uint16_t)(0x0005u);c->ip=0x002Au;break; /* mov bx, 5 */
        case 0x002A: push(b,c,c->cs);push(b,c,0x002Fu);c->cs=rd16(b,c->cs,0x002Du);c->ip=0x0A5Bu;{int s=call_longmul(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x002Fu)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0xa5b */
        case 0x002F: c->cx=(uint16_t)(c->ax);c->ip=0x0031u;break; /* mov cx, ax */
        case 0x0031: c->bx=(uint16_t)(c->dx);c->ip=0x0033u;break; /* mov bx, dx */
        case 0x0033: c->ax=(uint16_t)(0x1770u);c->ip=0x0036u;break; /* mov ax, 0x1770 */
        case 0x0036: mul16(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0039u;break; /* mul word ptr [bp - 8] */
        case 0x0039: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x003Bu;break; /* xor dx, dx */
        case 0x003B: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,0u));c->ip=0x003Du;break; /* add ax, cx */
        case 0x003D: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,0u));c->ip=0x003Fu;break; /* adc dx, bx */
        case 0x003F: c->cx=(uint16_t)(c->ax);c->ip=0x0041u;break; /* mov cx, ax */
        case 0x0041: c->bx=(uint16_t)(c->dx);c->ip=0x0043u;break; /* mov bx, dx */
        case 0x0043: c->ax=(uint16_t)(0x0064u);c->ip=0x0046u;break; /* mov ax, 0x64 */
        case 0x0046: mul16(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0049u;break; /* mul word ptr [bp - 0xa] */
        case 0x0049: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x004Bu;break; /* xor dx, dx */
        case 0x004B: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,0u));c->ip=0x004Du;break; /* add ax, cx */
        case 0x004D: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,0u));c->ip=0x004Fu;break; /* adc dx, bx */
        case 0x004F: c->cx=(uint16_t)(c->ax);c->ip=0x0051u;break; /* mov cx, ax */
        case 0x0051: c->bx=(uint16_t)(c->dx);c->ip=0x0053u;break; /* mov bx, dx */
        case 0x0053: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x0056u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x0056: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x0058u;break; /* xor dx, dx */
        case 0x0058: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,0u));c->ip=0x005Au;break; /* add ax, cx */
        case 0x005A: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,0u));c->ip=0x005Cu;break; /* adc dx, bx */
        case 0x005C: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x005Fu;break; /* mov word ptr [bp - 4], ax */
        case 0x005F: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->dx));c->ip=0x0062u;break; /* mov word ptr [bp - 2], dx */
        case 0x0062: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0065u;break; /* mov ax, word ptr [bp - 4] */
        case 0x0065: c->dx=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0068u;break; /* mov dx, word ptr [bp - 2] */
        case 0x0068: c->sp=(uint16_t)(c->bp);c->ip=0x006Au;break; /* mov sp, bp */
        case 0x006A: c->bp=(uint16_t)(pop(b,c));c->ip=0x006Bu;break; /* pop bp */
        case 0x006B: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+0u);return STATUS(OK);break; /* retf  */
        default: return STATUS(TRANSFER);
        }
    }
}
