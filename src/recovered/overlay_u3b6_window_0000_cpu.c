/* GAME.OVR unit 3B6h +0000 map window, raw CPU (docs/re/OVL_03B6_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u3b6_window_0000_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL03B6_0000_##s
typedef pr_ovl03b6_0000_status pr_status_t;
typedef pr_ovl03b6_0000_regs pr_cpu_t;
typedef struct {
    const pr_ovl03b6_0000_env *env;
    pr_ovl03b6_0000_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_ovl03ce_00f1_regs), "CPU layout");
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
static uint16_t incdec(pr_cpu_t *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t cf = c->flags & 1u; v = arith(c, v, 1u, 0u, bits, dec);
    c->flags = (uint16_t)((c->flags & ~1u) | cf); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1,
 * Unicorn's rule otherwise); AF := 1 (pinned DOSBox-X; Unicorn clears it). The emitter's report shows neither is observed. */
static uint16_t shift(pr_cpu_t *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, pre, res;
    n &= 0x1Fu;
    if (n == 0u) return v;
    v = (uint16_t)(v & mask);
    pre = right ? ((uint32_t)v >> (n - 1u)) : (((uint32_t)v << (n - 1u)) & mask);
    res = right ? (pre >> 1u) : ((pre << 1u) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    c->flags |= 0x10u; /* pinned: AF = (count & 1Fh) != 0 */
    if (right ? (pre & 1u) : (pre & sign)) c->flags |= 1u;
    if ((pre ^ res) & sign) c->flags |= 0x800u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
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
/* Overlay stub 03E9:002A, entered with the far frame pushed. A loaded entry is JMP FAR (EAh) to the unit's segment:00F1;
 * it is executed, then OVL_03CE_00F1 runs to its RETF 8, which returns straight to this body. */
static int call_tile_stub(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_ovl03ce_00f1_regs in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u), ip, cs;
    r->d->calls += 1u;
    if (rd8(b, c->cs, c->ip) != 0xEAu) { r->d->site = site; return STATUS(OVERLAY); }
    ip = rd16(b, c->cs, (uint16_t)(c->ip + 1u)); cs = rd16(b, c->cs, (uint16_t)(c->ip + 3u));
    if (ip != 0x00F1u) { r->d->site = site; return STATUS(OVERLAY); }
    c->ip = ip; c->cs = cs;
    memcpy(&in, c, sizeof in);
    s = (int)pr_ovl03ce_00f1_cpu(b, PR_BUS_RAM_SIZE, &in, &r->env->tile, &r->d->tile);
    memcpy(c, &in, sizeof in);
    if (s == PR_OVL03CE_00F1_OK) return 0;
    r->d->site = site; r->d->status = s; return STATUS(CALLEE);
}

pr_ovl03b6_0000_status pr_ovl03b6_0000_cpu(pr_bus *b, size_t ram_size, pr_ovl03b6_0000_regs *c,
                                           const pr_ovl03b6_0000_env *env, pr_ovl03b6_0000_diagnostic *d)
{
    pr_ovl03b6_0000_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller; size_t loops = 0, max_loopbacks;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0000u)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1;
    max_loopbacks = env->max_loopbacks;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x0000: push(b,c,c->bp);c->ip=0x0001u;break; /* push bp */
        case 0x0001: c->bp=(uint16_t)(c->sp);c->ip=0x0003u;break; /* mov bp, sp */
        case 0x0003: c->sp=(uint16_t)(arith(c,c->sp,0x0008u,0u,16u,1u));c->ip=0x0006u;break; /* sub sp, 8 */
        case 0x0006: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint8_t)(0x00u));c->ip=0x000Au;break; /* mov byte ptr [bp - 2], 0 */
        case 0x000A: wr8(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint8_t)(0x00u));c->ip=0x000Eu;break; /* mov byte ptr [bp - 8], 0 */
        case 0x000E: c->ip=0x0013u;break; /* jmp 0x13 */
        case 0x0010: wr8(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),8u,0u)));c->ip=0x0013u;break; /* inc byte ptr [bp - 8] */
        case 0x0013: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x00u));c->ip=0x0017u;break; /* mov byte ptr [bp - 1], 0 */
        case 0x0017: wr8(b,c->ss,(uint16_t)(c->bp+0xFFF9u),(uint8_t)(0x00u));c->ip=0x001Bu;break; /* mov byte ptr [bp - 7], 0 */
        case 0x001B: c->ip=0x0020u;break; /* jmp 0x20 */
        case 0x001D: wr8(b,c->ss,(uint16_t)(c->bp+0xFFF9u),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFF9u)),8u,0u)));c->ip=0x0020u;break; /* inc byte ptr [bp - 7] */
        case 0x0020: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFF9u))));c->ip=0x0023u;break; /* mov al, byte ptr [bp - 7] */
        case 0x0023: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0025u;break; /* xor ah, ah */
        case 0x0025: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0x0008u)),0u,16u,0u));c->ip=0x0028u;break; /* add ax, word ptr [bp + 8] */
        case 0x0028: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint8_t)((uint8_t)c->ax));c->ip=0x002Bu;break; /* mov byte ptr [bp - 4], al */
        case 0x002B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFF8u))));c->ip=0x002Eu;break; /* mov al, byte ptr [bp - 8] */
        case 0x002E: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0030u;break; /* xor ah, ah */
        case 0x0030: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0x0006u)),0u,16u,0u));c->ip=0x0033u;break; /* add ax, word ptr [bp + 6] */
        case 0x0033: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFBu),(uint8_t)((uint8_t)c->ax));c->ip=0x0036u;break; /* mov byte ptr [bp - 5], al */
        case 0x0036: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu))));c->ip=0x0039u;break; /* mov al, byte ptr [bp - 4] */
        case 0x0039: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x003Au;break; /* cwde  */
        case 0x003A: c->cx=(uint16_t)(c->ax);c->ip=0x003Cu;break; /* mov cx, ax */
        case 0x003C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFBu))));c->ip=0x003Fu;break; /* mov al, byte ptr [bp - 5] */
        case 0x003F: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0040u;break; /* cwde  */
        case 0x0040: c->dx=(uint16_t)(0x002Cu);c->ip=0x0043u;break; /* mov dx, 0x2c */
        case 0x0043: mul16(c,c->dx);c->ip=0x0045u;break; /* mul dx */
        case 0x0045: c->di=(uint16_t)(c->ax);c->ip=0x0047u;break; /* mov di, ax */
        case 0x0047: c->di=(uint16_t)(arith(c,c->di,c->cx,0u,16u,0u));c->ip=0x0049u;break; /* add di, cx */
        case 0x0049: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(c->di+0x3648u))));c->ip=0x004Du;break; /* mov al, byte ptr [di + 0x3648] */
        case 0x004D: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x004Fu;break; /* xor ah, ah */
        case 0x004F: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0050u;break; /* dec ax */
        case 0x0050: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x0053u;break; /* mov byte ptr [bp - 6], al */
        case 0x0053: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFDu),(uint8_t)(0x01u));c->ip=0x0057u;break; /* mov byte ptr [bp - 3], 1 */
        case 0x0057: c->ip=0x005Cu;break; /* jmp 0x5c */
        case 0x0059: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFDu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu)),8u,0u)));c->ip=0x005Cu;break; /* inc byte ptr [bp - 3] */
        case 0x005C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x005Fu;break; /* mov al, byte ptr [bp - 3] */
        case 0x005F: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0060u;break; /* cwde  */
        case 0x0060: c->di=(uint16_t)(c->ax);c->ip=0x0062u;break; /* mov di, ax */
        case 0x0062: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0064u;break; /* shl di, 1 */
        case 0x0064: c->si=(uint16_t)(c->di);c->ip=0x0066u;break; /* mov si, di */
        case 0x0066: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0068u;break; /* shl di, 1 */
        case 0x0068: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x006Au;break; /* add di, si */
        case 0x006A: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C30u)));c->ip=0x006Eu;break; /* mov ax, word ptr [di + 0xc30] */
        case 0x006E: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x0070u;break; /* xor dx, dx */
        case 0x0070: c->cx=(uint16_t)(c->ax);c->ip=0x0072u;break; /* mov cx, ax */
        case 0x0072: c->bx=(uint16_t)(c->dx);c->ip=0x0074u;break; /* mov bx, dx */
        case 0x0074: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu))));c->ip=0x0077u;break; /* mov al, byte ptr [bp - 4] */
        case 0x0077: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0078u;break; /* cwde  */
        case 0x0078: c->dx=(c->ax&0x8000u)?0xFFFFu:0u;c->ip=0x0079u;break; /* cdq  */
        case 0x0079: (void)arith(c,c->dx,c->bx,0u,16u,1u);c->ip=0x007Bu;break; /* cmp dx, bx */
        case 0x007B: if((c->flags&0x40u)){c->ip=0x0080u;}else c->ip=0x007Du;break; /* je 0x80 */
        case 0x007D: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x0080: (void)arith(c,c->ax,c->cx,0u,16u,1u);c->ip=0x0082u;break; /* cmp ax, cx */
        case 0x0082: if((c->flags&0x40u)){c->ip=0x0087u;}else c->ip=0x0084u;break; /* je 0x87 */
        case 0x0084: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x0087: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x008Au;break; /* mov al, byte ptr [bp - 3] */
        case 0x008A: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x008Bu;break; /* cwde  */
        case 0x008B: c->di=(uint16_t)(c->ax);c->ip=0x008Du;break; /* mov di, ax */
        case 0x008D: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x008Fu;break; /* shl di, 1 */
        case 0x008F: c->si=(uint16_t)(c->di);c->ip=0x0091u;break; /* mov si, di */
        case 0x0091: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0093u;break; /* shl di, 1 */
        case 0x0093: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x0095u;break; /* add di, si */
        case 0x0095: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C32u)));c->ip=0x0099u;break; /* mov ax, word ptr [di + 0xc32] */
        case 0x0099: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x009Bu;break; /* xor dx, dx */
        case 0x009B: c->cx=(uint16_t)(c->ax);c->ip=0x009Du;break; /* mov cx, ax */
        case 0x009D: c->bx=(uint16_t)(c->dx);c->ip=0x009Fu;break; /* mov bx, dx */
        case 0x009F: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFBu))));c->ip=0x00A2u;break; /* mov al, byte ptr [bp - 5] */
        case 0x00A2: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x00A3u;break; /* cwde  */
        case 0x00A3: c->dx=(c->ax&0x8000u)?0xFFFFu:0u;c->ip=0x00A4u;break; /* cdq  */
        case 0x00A4: (void)arith(c,c->dx,c->bx,0u,16u,1u);c->ip=0x00A6u;break; /* cmp dx, bx */
        case 0x00A6: if((c->flags&0x40u)){c->ip=0x00ABu;}else c->ip=0x00A8u;break; /* je 0xab */
        case 0x00A8: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x00AB: (void)arith(c,c->ax,c->cx,0u,16u,1u);c->ip=0x00ADu;break; /* cmp ax, cx */
        case 0x00AD: if((c->flags&0x40u)){c->ip=0x00B2u;}else c->ip=0x00AFu;break; /* je 0xb2 */
        case 0x00AF: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x00B2: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x00B5u;break; /* mov al, byte ptr [bp - 3] */
        case 0x00B5: (void)arith(c,(uint8_t)c->ax,0x03u,0u,8u,1u);c->ip=0x00B7u;break; /* cmp al, 3 */
        case 0x00B7: if(!(c->flags&0x40u)){c->ip=0x00DFu;}else c->ip=0x00B9u;break; /* jne 0xdf */
        case 0x00B9: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x00BDu;break; /* les di, ptr [0x49d2] */
        case 0x00BD: (void)arith(c,rd16(b,c->es,(uint16_t)(c->di+0x0318u)),0x00FFu,0u,16u,1u);c->ip=0x00C4u;break; /* cmp word ptr es:[di + 0x318], 0xff */
        case 0x00C4: if((c->flags&0x40u)){c->ip=0x00DCu;}else c->ip=0x00C6u;break; /* je 0xdc */
        case 0x00C6: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x00C9u;break; /* mov al, byte ptr [bp - 3] */
        case 0x00C9: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x00CAu;break; /* cwde  */
        case 0x00CA: c->di=(uint16_t)(c->ax);c->ip=0x00CCu;break; /* mov di, ax */
        case 0x00CC: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x00CEu;break; /* shl di, 1 */
        case 0x00CE: c->si=(uint16_t)(c->di);c->ip=0x00D0u;break; /* mov si, di */
        case 0x00D0: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x00D2u;break; /* shl di, 1 */
        case 0x00D2: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x00D4u;break; /* add di, si */
        case 0x00D4: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x00D8u;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x00D8: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x00D9u;break; /* dec ax */
        case 0x00D9: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x00DCu;break; /* mov byte ptr [bp - 6], al */
        case 0x00DC: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x00DF: (void)arith(c,(uint8_t)c->ax,0x04u,0u,8u,1u);c->ip=0x00E1u;break; /* cmp al, 4 */
        case 0x00E1: if(!(c->flags&0x40u)){c->ip=0x0124u;}else c->ip=0x00E3u;break; /* jne 0x124 */
        case 0x00E3: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x00E7u;break; /* les di, ptr [0x49d2] */
        case 0x00E7: (void)arith(c,rd16(b,c->es,(uint16_t)(c->di+0x03A4u)),0x0000u,0u,16u,1u);c->ip=0x00EDu;break; /* cmp word ptr es:[di + 0x3a4], 0 */
        case 0x00ED: if(!(c->flags&0x40u)){c->ip=0x010Bu;}else c->ip=0x00EFu;break; /* jne 0x10b */
        case 0x00EF: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x00F3u;break; /* les di, ptr [0x49d2] */
        case 0x00F3: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x02F8u)));c->ip=0x00F8u;break; /* mov ax, word ptr es:[di + 0x2f8] */
        case 0x00F8: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax&0x0008u),16u));c->ip=0x00FBu;break; /* and ax, 8 */
        case 0x00FB: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|c->ax),16u));c->ip=0x00FDu;break; /* or ax, ax */
        case 0x00FD: if(!(c->flags&0x40u)){c->ip=0x010Bu;}else c->ip=0x00FFu;break; /* jne 0x10b */
        case 0x00FF: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0103u;break; /* les di, ptr [0x49d2] */
        case 0x0103: (void)arith(c,rd16(b,c->es,(uint16_t)(c->di+0x033Eu)),0x0000u,0u,16u,1u);c->ip=0x0109u;break; /* cmp word ptr es:[di + 0x33e], 0 */
        case 0x0109: if(!(c->flags&0x40u)){c->ip=0x0121u;}else c->ip=0x010Bu;break; /* jne 0x121 */
        case 0x010B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x010Eu;break; /* mov al, byte ptr [bp - 3] */
        case 0x010E: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x010Fu;break; /* cwde  */
        case 0x010F: c->di=(uint16_t)(c->ax);c->ip=0x0111u;break; /* mov di, ax */
        case 0x0111: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0113u;break; /* shl di, 1 */
        case 0x0113: c->si=(uint16_t)(c->di);c->ip=0x0115u;break; /* mov si, di */
        case 0x0115: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0117u;break; /* shl di, 1 */
        case 0x0117: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x0119u;break; /* add di, si */
        case 0x0119: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x011Du;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x011D: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x011Eu;break; /* dec ax */
        case 0x011E: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x0121u;break; /* mov byte ptr [bp - 6], al */
        case 0x0121: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x0124: (void)arith(c,(uint8_t)c->ax,0x05u,0u,8u,1u);c->ip=0x0126u;break; /* cmp al, 5 */
        case 0x0126: if(!(c->flags&0x40u)){c->ip=0x0150u;}else c->ip=0x0128u;break; /* jne 0x150 */
        case 0x0128: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x012Cu;break; /* les di, ptr [0x49d2] */
        case 0x012C: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0340u)));c->ip=0x0131u;break; /* mov ax, word ptr es:[di + 0x340] */
        case 0x0131: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax&0x0002u),16u));c->ip=0x0134u;break; /* and ax, 2 */
        case 0x0134: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|c->ax),16u));c->ip=0x0136u;break; /* or ax, ax */
        case 0x0136: if(!(c->flags&0x40u)){c->ip=0x014Eu;}else c->ip=0x0138u;break; /* jne 0x14e */
        case 0x0138: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x013Bu;break; /* mov al, byte ptr [bp - 3] */
        case 0x013B: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x013Cu;break; /* cwde  */
        case 0x013C: c->di=(uint16_t)(c->ax);c->ip=0x013Eu;break; /* mov di, ax */
        case 0x013E: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0140u;break; /* shl di, 1 */
        case 0x0140: c->si=(uint16_t)(c->di);c->ip=0x0142u;break; /* mov si, di */
        case 0x0142: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0144u;break; /* shl di, 1 */
        case 0x0144: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x0146u;break; /* add di, si */
        case 0x0146: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x014Au;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x014A: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x014Bu;break; /* dec ax */
        case 0x014B: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x014Eu;break; /* mov byte ptr [bp - 6], al */
        case 0x014E: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x0150: (void)arith(c,(uint8_t)c->ax,0x06u,0u,8u,1u);c->ip=0x0152u;break; /* cmp al, 6 */
        case 0x0152: if(!(c->flags&0x40u)){c->ip=0x016Cu;}else c->ip=0x0154u;break; /* jne 0x16c */
        case 0x0154: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x0157u;break; /* mov al, byte ptr [bp - 3] */
        case 0x0157: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0158u;break; /* cwde  */
        case 0x0158: c->di=(uint16_t)(c->ax);c->ip=0x015Au;break; /* mov di, ax */
        case 0x015A: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x015Cu;break; /* shl di, 1 */
        case 0x015C: c->si=(uint16_t)(c->di);c->ip=0x015Eu;break; /* mov si, di */
        case 0x015E: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0160u;break; /* shl di, 1 */
        case 0x0160: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x0162u;break; /* add di, si */
        case 0x0162: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x0166u;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x0166: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0167u;break; /* dec ax */
        case 0x0167: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x016Au;break; /* mov byte ptr [bp - 6], al */
        case 0x016A: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x016C: (void)arith(c,(uint8_t)c->ax,0x07u,0u,8u,1u);c->ip=0x016Eu;break; /* cmp al, 7 */
        case 0x016E: if(!(c->flags&0x40u)){c->ip=0x0198u;}else c->ip=0x0170u;break; /* jne 0x198 */
        case 0x0170: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0174u;break; /* les di, ptr [0x49d2] */
        case 0x0174: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0340u)));c->ip=0x0179u;break; /* mov ax, word ptr es:[di + 0x340] */
        case 0x0179: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax&0x0001u),16u));c->ip=0x017Cu;break; /* and ax, 1 */
        case 0x017C: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|c->ax),16u));c->ip=0x017Eu;break; /* or ax, ax */
        case 0x017E: if(!(c->flags&0x40u)){c->ip=0x0196u;}else c->ip=0x0180u;break; /* jne 0x196 */
        case 0x0180: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x0183u;break; /* mov al, byte ptr [bp - 3] */
        case 0x0183: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0184u;break; /* cwde  */
        case 0x0184: c->di=(uint16_t)(c->ax);c->ip=0x0186u;break; /* mov di, ax */
        case 0x0186: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0188u;break; /* shl di, 1 */
        case 0x0188: c->si=(uint16_t)(c->di);c->ip=0x018Au;break; /* mov si, di */
        case 0x018A: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x018Cu;break; /* shl di, 1 */
        case 0x018C: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x018Eu;break; /* add di, si */
        case 0x018E: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x0192u;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x0192: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0193u;break; /* dec ax */
        case 0x0193: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x0196u;break; /* mov byte ptr [bp - 6], al */
        case 0x0196: c->ip=0x01C3u;break; /* jmp 0x1c3 */
        case 0x0198: (void)arith(c,(uint8_t)c->ax,0x08u,0u,8u,1u);c->ip=0x019Au;break; /* cmp al, 8 */
        case 0x019A: if((c->flags&0x40u)){c->ip=0x01A0u;}else c->ip=0x019Cu;break; /* je 0x1a0 */
        case 0x019C: (void)arith(c,(uint8_t)c->ax,0x09u,0u,8u,1u);c->ip=0x019Eu;break; /* cmp al, 9 */
        case 0x019E: if(!(c->flags&0x40u)){c->ip=0x01C3u;}else c->ip=0x01A0u;break; /* jne 0x1c3 */
        case 0x01A0: {uint16_t o=(uint16_t)(0x49D2u);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x01A4u;break; /* les di, ptr [0x49d2] */
        case 0x01A4: (void)arith(c,rd16(b,c->es,(uint16_t)(c->di+0x0366u)),0x00FDu,0u,16u,1u);c->ip=0x01ABu;break; /* cmp word ptr es:[di + 0x366], 0xfd */
        case 0x01AB: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x01C3u;}else c->ip=0x01ADu;break; /* jbe 0x1c3 */
        case 0x01AD: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu))));c->ip=0x01B0u;break; /* mov al, byte ptr [bp - 3] */
        case 0x01B0: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x01B1u;break; /* cwde  */
        case 0x01B1: c->di=(uint16_t)(c->ax);c->ip=0x01B3u;break; /* mov di, ax */
        case 0x01B3: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x01B5u;break; /* shl di, 1 */
        case 0x01B5: c->si=(uint16_t)(c->di);c->ip=0x01B7u;break; /* mov si, di */
        case 0x01B7: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x01B9u;break; /* shl di, 1 */
        case 0x01B9: c->di=(uint16_t)(arith(c,c->di,c->si,0u,16u,0u));c->ip=0x01BBu;break; /* add di, si */
        case 0x01BB: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x0C34u)));c->ip=0x01BFu;break; /* mov ax, word ptr [di + 0xc34] */
        case 0x01BF: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x01C0u;break; /* dec ax */
        case 0x01C0: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint8_t)((uint8_t)c->ax));c->ip=0x01C3u;break; /* mov byte ptr [bp - 6], al */
        case 0x01C3: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFDu)),0x09u,0u,8u,1u);c->ip=0x01C7u;break; /* cmp byte ptr [bp - 3], 9 */
        case 0x01C7: if((c->flags&0x40u)){c->ip=0x01CCu;}else c->ip=0x01C9u;break; /* je 0x1cc */
        case 0x01C9: if(loops==max_loopbacks){c->ip=0x01C9u;return STATUS(LIMIT);}++loops;c->ip=0x0059u;break; /* jmp 0x59 */
        case 0x01CC: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x01CFu;break; /* mov al, byte ptr [bp - 1] */
        case 0x01CF: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x01D0u;break; /* cwde  */
        case 0x01D0: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x01D1u;break; /* inc ax */
        case 0x01D1: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x01D2u;break; /* dec ax */
        case 0x01D2: push(b,c,c->ax);c->ip=0x01D3u;break; /* push ax */
        case 0x01D3: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFEu))));c->ip=0x01D6u;break; /* mov al, byte ptr [bp - 2] */
        case 0x01D6: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x01D7u;break; /* cwde  */
        case 0x01D7: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x01D8u;break; /* inc ax */
        case 0x01D8: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x01D9u;break; /* dec ax */
        case 0x01D9: push(b,c,c->ax);c->ip=0x01DAu;break; /* push ax */
        case 0x01DA: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFAu))));c->ip=0x01DDu;break; /* mov al, byte ptr [bp - 6] */
        case 0x01DD: push(b,c,c->ax);c->ip=0x01DEu;break; /* push ax */
        case 0x01DE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x01E0u;break; /* mov al, 1 */
        case 0x01E0: push(b,c,c->ax);c->ip=0x01E1u;break; /* push ax */
        case 0x01E1: push(b,c,c->cs);push(b,c,0x01E6u);c->cs=rd16(b,c->cs,0x01E4u);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01E6u)return STATUS(TRANSFER);break; /* lcall 0x3e9, 0x2a */
        case 0x01E6: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu)),0x03u,0u,8u,0u)));c->ip=0x01EAu;break; /* add byte ptr [bp - 1], 3 */
        case 0x01EA: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFF9u)),0x04u,0u,8u,1u);c->ip=0x01EEu;break; /* cmp byte ptr [bp - 7], 4 */
        case 0x01EE: if((c->flags&0x40u)){c->ip=0x01F3u;}else c->ip=0x01F0u;break; /* je 0x1f3 */
        case 0x01F0: if(loops==max_loopbacks){c->ip=0x01F0u;return STATUS(LIMIT);}++loops;c->ip=0x001Du;break; /* jmp 0x1d */
        case 0x01F3: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint8_t)(arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFEu)),0x03u,0u,8u,0u)));c->ip=0x01F7u;break; /* add byte ptr [bp - 2], 3 */
        case 0x01F7: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),0x04u,0u,8u,1u);c->ip=0x01FBu;break; /* cmp byte ptr [bp - 8], 4 */
        case 0x01FB: if((c->flags&0x40u)){c->ip=0x0200u;}else c->ip=0x01FDu;break; /* je 0x200 */
        case 0x01FD: if(loops==max_loopbacks){c->ip=0x01FDu;return STATUS(LIMIT);}++loops;c->ip=0x0010u;break; /* jmp 0x10 */
        case 0x0200: c->sp=(uint16_t)(c->bp);c->ip=0x0202u;break; /* mov sp, bp */
        case 0x0202: c->bp=(uint16_t)(pop(b,c));c->ip=0x0203u;break; /* pop bp */
        case 0x0203: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+4u);return STATUS(OK);break; /* retf 4 */
        default: return STATUS(TRANSFER);
        }
    }
}
