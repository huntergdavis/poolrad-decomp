/* START.EXE resident 0464:248E bitmap allocator, raw CPU (docs/re/RES_0464_ALLOC_248E_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_0464_alloc_248e_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R0464_ALLOC_CPU_##s
typedef pr_r0464_alloc_cpu_status pr_status_t;
typedef pr_r0464_alloc_cpu pr_cpu_t;
typedef struct { pr_r0464_alloc_cpu_diagnostic *d; unsigned getmem_steps; } ctx_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(pr_bus *b, size_t at)
{
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static uint8_t rd8(pr_bus *b, uint16_t seg, uint16_t off) { return pr_bus_read8(b, physical(seg, off)); }
static uint16_t rd16(pr_bus *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static void wr8(pr_bus *b, uint16_t seg, uint16_t off, uint8_t v) { pr_bus_write8(b, physical(seg, off), v); }
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
static uint16_t shift(pr_cpu_t *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, pre, res;
    n &= 0x1Fu;
    if (n == 0u) return v;
    v = (uint16_t)(v & mask);
    pre = right ? ((uint32_t)v >> (n - 1u)) : (((uint32_t)v << (n - 1u)) & mask);
    res = right ? (pre >> 1u) : ((pre << 1u) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (right ? (pre & 1u) : (pre & sign)) c->flags |= 1u;
    if ((pre ^ res) & sign) c->flags |= 0x800u;
    c->flags |= 0x10u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    c->flags &= (uint16_t)~0x08D5u;
    if (c->dx) c->flags |= 0x801u;
    szp(c, c->ax, 16u);
}
/* SYSTEM GetMem 0AF8:023F on the flat RAM (its accepted raw model), entered with the far frame pushed. */
static int call_getmem(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_getmem_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_getmem_023f(b->ram, PR_BUS_RAM_SIZE, &inner, dg->getmem_steps);
    memcpy(c, &inner, sizeof inner);
    dg->d->getmem_calls += 1; dg->d->getmem_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
/* SYSTEM FillChar 0AF8:16A8 on the bus (its accepted raw model), entered with the far frame pushed. */
static int call_fill(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_leaf_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_fillchar_bus_16a8(b, &inner);
    memcpy(c, &inner, sizeof inner);
    dg->d->fill_calls += 1; dg->d->fill_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
static int ordinary_segment(uint16_t s)
{
    size_t start = (size_t)s << 4u;
    return start > 0xAFFFFu || start + 0xFFFFu < 0xA0000u;
}

pr_r0464_alloc_cpu_status pr_r0464_alloc_248e_cpu(pr_bus *b, size_t ram_size, pr_r0464_alloc_cpu *c,
                                                   size_t max_loopbacks, unsigned getmem_steps,
                                                   pr_r0464_alloc_cpu_diagnostic *d)
{
    pr_r0464_alloc_cpu_diagnostic fallback = {0, -1, 0, -1};
    ctx_t context; ctx_t *dg = &context;
    uint16_t caller; size_t loops = 0;
    if (b == NULL || b->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x248Eu ||
        (b->ega != NULL && !ordinary_segment(c->ds)))
        return STATUS(ARGUMENT);
    context.d = d != NULL ? d : &fallback; context.getmem_steps = getmem_steps;
    *context.d = fallback;
    caller = c->cs;
    (void)loops; (void)max_loopbacks;
    for (;;) {
        switch (c->ip) {
        case 0x248E: push(b,c,c->bp);c->ip=0x248Fu;break; /* push bp */
        case 0x248F: c->bp=(uint16_t)(c->sp);c->ip=0x2491u;break; /* mov bp, sp */
        case 0x2491: c->sp=(uint16_t)(arith(c,c->sp,0x0004u,0u,16u,1u));c->ip=0x2494u;break; /* sub sp, 4 */
        case 0x2494: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0010u)));c->ip=0x2497u;break; /* mov ax, word ptr [bp + 0x10] */
        case 0x2497: mul16(c,rd16(b,c->ss,(uint16_t)(c->bp+0x000Eu)));c->ip=0x249Au;break; /* mul word ptr [bp + 0xe] */
        case 0x249A: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x249Cu;break; /* shl ax, 1 */
        case 0x249C: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x249Fu;break; /* mov word ptr [bp - 2], ax */
        case 0x249F: (void)arith(c,rd8(b,c->ds,(uint16_t)(0x4A1Eu)),0x00u,0u,8u,1u);c->ip=0x24A4u;break; /* cmp byte ptr [0x4a1e], 0 */
        case 0x24A4: if((c->flags&0x40u)){c->ip=0x24AEu;}else c->ip=0x24A6u;break; /* je 0x24ae */
        case 0x24A6: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x24A9u;break; /* mov ax, word ptr [bp - 2] */
        case 0x24A9: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x24ABu;break; /* shl ax, 1 */
        case 0x24AB: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x24AEu;break; /* mov word ptr [bp - 2], ax */
        case 0x24AE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x24B1u;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x24B1: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x24B3u;break; /* xor ah, ah */
        case 0x24B3: mul16(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x24B6u;break; /* mul word ptr [bp - 2] */
        case 0x24B6: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x24B9u;break; /* mov word ptr [bp - 4], ax */
        case 0x24B9: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x24BCu;break; /* mov ax, word ptr [bp - 4] */
        case 0x24BC: c->ax=(uint16_t)(arith(c,c->ax,0x0017u,0u,16u,0u));c->ip=0x24BFu;break; /* add ax, 0x17 */
        case 0x24BF: push(b,c,c->ax);c->ip=0x24C0u;break; /* push ax */
        case 0x24C0: push(b,c,c->cs);push(b,c,0x24C5u);c->cs=rd16(b,c->cs,0x24C3u);c->ip=0x023Fu;{int s=call_getmem(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x24C5u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x23f */
        case 0x24C5: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24C8u;break; /* les di, ptr [bp + 6] */
        case 0x24C8: wr16(b,c->es,(uint16_t)(c->di),(uint16_t)(c->ax));c->ip=0x24CBu;break; /* mov word ptr es:[di], ax */
        case 0x24CB: wr16(b,c->es,(uint16_t)(c->di+0x0002u),(uint16_t)(c->dx));c->ip=0x24CFu;break; /* mov word ptr es:[di + 2], dx */
        case 0x24CF: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24D2u;break; /* les di, ptr [bp + 6] */
        case 0x24D2: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24D5u;break; /* les di, ptr es:[di] */
        case 0x24D5: push(b,c,c->es);c->ip=0x24D6u;break; /* push es */
        case 0x24D6: push(b,c,c->di);c->ip=0x24D7u;break; /* push di */
        case 0x24D7: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x24DAu;break; /* mov ax, word ptr [bp - 4] */
        case 0x24DA: c->ax=(uint16_t)(arith(c,c->ax,0x0017u,0u,16u,0u));c->ip=0x24DDu;break; /* add ax, 0x17 */
        case 0x24DD: push(b,c,c->ax);c->ip=0x24DEu;break; /* push ax */
        case 0x24DE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x24E0u;break; /* mov al, 0 */
        case 0x24E0: push(b,c,c->ax);c->ip=0x24E1u;break; /* push ax */
        case 0x24E1: push(b,c,c->cs);push(b,c,0x24E6u);c->cs=rd16(b,c->cs,0x24E4u);c->ip=0x16A8u;{int s=call_fill(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x24E6u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x16a8 */
        case 0x24E6: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0010u)));c->ip=0x24E9u;break; /* mov ax, word ptr [bp + 0x10] */
        case 0x24E9: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24ECu;break; /* les di, ptr [bp + 6] */
        case 0x24EC: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24EFu;break; /* les di, ptr es:[di] */
        case 0x24EF: wr16(b,c->es,(uint16_t)(c->di),(uint16_t)(c->ax));c->ip=0x24F2u;break; /* mov word ptr es:[di], ax */
        case 0x24F2: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Eu)));c->ip=0x24F5u;break; /* mov ax, word ptr [bp + 0xe] */
        case 0x24F5: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24F8u;break; /* les di, ptr [bp + 6] */
        case 0x24F8: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x24FBu;break; /* les di, ptr es:[di] */
        case 0x24FB: wr16(b,c->es,(uint16_t)(c->di+0x0002u),(uint16_t)(c->ax));c->ip=0x24FFu;break; /* mov word ptr es:[di + 2], ax */
        case 0x24FF: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x2502u;break; /* mov ax, word ptr [bp - 2] */
        case 0x2502: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2505u;break; /* les di, ptr [bp + 6] */
        case 0x2505: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2508u;break; /* les di, ptr es:[di] */
        case 0x2508: wr16(b,c->es,(uint16_t)(c->di+0x0011u),(uint16_t)(c->ax));c->ip=0x250Cu;break; /* mov word ptr es:[di + 0x11], ax */
        case 0x250C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x250Fu;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x250F: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2512u;break; /* les di, ptr [bp + 6] */
        case 0x2512: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2515u;break; /* les di, ptr es:[di] */
        case 0x2515: wr8(b,c->es,(uint16_t)(c->di+0x0008u),(uint8_t)((uint8_t)c->ax));c->ip=0x2519u;break; /* mov byte ptr es:[di + 8], al */
        case 0x2519: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x251Cu;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x251C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x01u),8u)));c->ip=0x251Eu;break; /* and al, 1 */
        case 0x251E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x2520u;break; /* or al, al */
        case 0x2520: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x258Fu;}else c->ip=0x2522u;break; /* jbe 0x258f */
        case 0x2522: (void)arith(c,rd8(b,c->ds,(uint16_t)(0x4A1Eu)),0x01u,0u,8u,1u);c->ip=0x2527u;break; /* cmp byte ptr [0x4a1e], 1 */
        case 0x2527: if(!(c->flags&0x40u)){c->ip=0x2562u;}else c->ip=0x2529u;break; /* jne 0x2562 */
        case 0x2529: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x252Cu;break; /* mov ax, word ptr [bp - 4] */
        case 0x252C: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x252Eu;break; /* shr ax, 1 */
        case 0x252E: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x2530u;break; /* shr ax, 1 */
        case 0x2530: push(b,c,c->ax);c->ip=0x2531u;break; /* push ax */
        case 0x2531: push(b,c,c->cs);push(b,c,0x2536u);c->cs=rd16(b,c->cs,0x2534u);c->ip=0x023Fu;{int s=call_getmem(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x2536u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x23f */
        case 0x2536: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2539u;break; /* les di, ptr [bp + 6] */
        case 0x2539: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x253Cu;break; /* les di, ptr es:[di] */
        case 0x253C: wr16(b,c->es,(uint16_t)(c->di+0x0013u),(uint16_t)(c->ax));c->ip=0x2540u;break; /* mov word ptr es:[di + 0x13], ax */
        case 0x2540: wr16(b,c->es,(uint16_t)(c->di+0x0015u),(uint16_t)(c->dx));c->ip=0x2544u;break; /* mov word ptr es:[di + 0x15], dx */
        case 0x2544: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2547u;break; /* les di, ptr [bp + 6] */
        case 0x2547: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x254Au;break; /* les di, ptr es:[di] */
        case 0x254A: {uint16_t o=(uint16_t)(c->di+0x0013u);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x254Eu;break; /* les di, ptr es:[di + 0x13] */
        case 0x254E: push(b,c,c->es);c->ip=0x254Fu;break; /* push es */
        case 0x254F: push(b,c,c->di);c->ip=0x2550u;break; /* push di */
        case 0x2550: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x2553u;break; /* mov ax, word ptr [bp - 4] */
        case 0x2553: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x2555u;break; /* shr ax, 1 */
        case 0x2555: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x2557u;break; /* shr ax, 1 */
        case 0x2557: push(b,c,c->ax);c->ip=0x2558u;break; /* push ax */
        case 0x2558: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x255Au;break; /* mov al, 0 */
        case 0x255A: push(b,c,c->ax);c->ip=0x255Bu;break; /* push ax */
        case 0x255B: push(b,c,c->cs);push(b,c,0x2560u);c->cs=rd16(b,c->cs,0x255Eu);c->ip=0x16A8u;{int s=call_fill(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x2560u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x16a8 */
        case 0x2560: c->ip=0x258Fu;break; /* jmp 0x258f */
        case 0x2562: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x2565u;break; /* push word ptr [bp - 4] */
        case 0x2565: push(b,c,c->cs);push(b,c,0x256Au);c->cs=rd16(b,c->cs,0x2568u);c->ip=0x023Fu;{int s=call_getmem(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x256Au)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x23f */
        case 0x256A: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x256Du;break; /* les di, ptr [bp + 6] */
        case 0x256D: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2570u;break; /* les di, ptr es:[di] */
        case 0x2570: wr16(b,c->es,(uint16_t)(c->di+0x0013u),(uint16_t)(c->ax));c->ip=0x2574u;break; /* mov word ptr es:[di + 0x13], ax */
        case 0x2574: wr16(b,c->es,(uint16_t)(c->di+0x0015u),(uint16_t)(c->dx));c->ip=0x2578u;break; /* mov word ptr es:[di + 0x15], dx */
        case 0x2578: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x257Bu;break; /* les di, ptr [bp + 6] */
        case 0x257B: {uint16_t o=(uint16_t)(c->di);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x257Eu;break; /* les di, ptr es:[di] */
        case 0x257E: {uint16_t o=(uint16_t)(c->di+0x0013u);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x2582u;break; /* les di, ptr es:[di + 0x13] */
        case 0x2582: push(b,c,c->es);c->ip=0x2583u;break; /* push es */
        case 0x2583: push(b,c,c->di);c->ip=0x2584u;break; /* push di */
        case 0x2584: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x2587u;break; /* push word ptr [bp - 4] */
        case 0x2587: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x2589u;break; /* mov al, 0 */
        case 0x2589: push(b,c,c->ax);c->ip=0x258Au;break; /* push ax */
        case 0x258A: push(b,c,c->cs);push(b,c,0x258Fu);c->cs=rd16(b,c->cs,0x258Du);c->ip=0x16A8u;{int s=call_fill(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x258Fu)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x16a8 */
        case 0x258F: c->sp=(uint16_t)(c->bp);c->ip=0x2591u;break; /* mov sp, bp */
        case 0x2591: c->bp=(uint16_t)(pop(b,c));c->ip=0x2592u;break; /* pop bp */
        case 0x2592: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+12u);return STATUS(OK);break; /* retf 0xc */
        default: return STATUS(TRANSFER);
        }
    }
}
