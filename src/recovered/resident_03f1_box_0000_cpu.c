/* START.EXE resident 03F1:0000 tile box, raw CPU (docs/re/RES_03F1_BOX_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_03f1_box_0000_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R03F1_BOX_##s
typedef pr_r03f1_box_status pr_status_t;
typedef pr_r03f1_box_regs pr_cpu_t;
typedef struct {
    const pr_r03f1_box_env *env;
    pr_r03f1_box_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_ovl040c_015d_regs), "CPU layout");
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
 * Unicorn's rule otherwise); AF := 0 (Unicorn). The report in the doc shows where these are observed. */
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
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
/* Overlay stub 0428:002A, entered with the far frame pushed. A loaded entry is JMP FAR (EAh) to the unit's segment:015D;
 * it is executed, then OVL_040C_015D runs to its RETF 0Ah, which returns straight to this body. */
static int call_tile_stub(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_ovl040c_015d_regs in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u), ip, cs;
    r->d->calls += 1u;
    if (rd8(b, c->cs, c->ip) != 0xEAu) { r->d->site = site; return STATUS(OVERLAY); }
    ip = rd16(b, c->cs, (uint16_t)(c->ip + 1u)); cs = rd16(b, c->cs, (uint16_t)(c->ip + 3u));
    if (ip != 0x015Du) { r->d->site = site; return STATUS(OVERLAY); }
    c->ip = ip; c->cs = cs;
    memcpy(&in, c, sizeof in);
    s = (int)pr_ovl040c_015d_cpu(b, PR_BUS_RAM_SIZE, &in, &r->env->tile, &r->d->tile);
    memcpy(c, &in, sizeof in);
    if (s == PR_OVL040C_015D_OK) return 0;
    r->d->site = site; r->d->status = s;
    return s == PR_OVL040C_015D_EXIT ? STATUS(EXIT) : s == PR_OVL040C_015D_CONTRACT ? STATUS(CONTRACT) : STATUS(CALLEE);
}

pr_r03f1_box_status pr_r03f1_box_0000_cpu(pr_bus *b, size_t ram_size, pr_r03f1_box_regs *c, const pr_r03f1_box_env *env,
                                          pr_r03f1_box_diagnostic *d)
{
    pr_r03f1_box_diagnostic fallback;
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
        case 0x0003: c->sp=(uint16_t)(arith(c,c->sp,0x0004u,0u,16u,1u));c->ip=0x0006u;break; /* sub sp, 4 */
        case 0x0006: wr8(b,c->ss,(uint16_t)(c->bp+0x000Eu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu)),8u,1u)));c->ip=0x0009u;break; /* dec byte ptr [bp + 0xe] */
        case 0x0009: wr8(b,c->ss,(uint16_t)(c->bp+0x000Cu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu)),8u,1u)));c->ip=0x000Cu;break; /* dec byte ptr [bp + 0xc] */
        case 0x000C: wr8(b,c->ss,(uint16_t)(c->bp+0x000Au),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0x000Au)),8u,0u)));c->ip=0x000Fu;break; /* inc byte ptr [bp + 0xa] */
        case 0x000F: wr8(b,c->ss,(uint16_t)(c->bp+0x0008u),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0x0008u)),8u,0u)));c->ip=0x0012u;break; /* inc byte ptr [bp + 8] */
        case 0x0012: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0006u))));c->ip=0x0015u;break; /* mov al, byte ptr [bp + 6] */
        case 0x0015: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0017u;break; /* xor ah, ah */
        case 0x0017: c->si=(uint16_t)(c->ax);c->ip=0x0019u;break; /* mov si, ax */
        case 0x0019: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x001Bu;break; /* shl ax, 1 */
        case 0x001B: c->ax=(uint16_t)(arith(c,c->ax,c->si,0u,16u,0u));c->ip=0x001Du;break; /* add ax, si */
        case 0x001D: c->ax=(uint16_t)(arith(c,c->ax,0x0114u,0u,16u,0u));c->ip=0x0020u;break; /* add ax, 0x114 */
        case 0x0020: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFDu),(uint16_t)(c->ax));c->ip=0x0023u;break; /* mov word ptr [bp - 3], ax */
        case 0x0023: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x0026u;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x0026: push(b,c,c->ax);c->ip=0x0027u;break; /* push ax */
        case 0x0027: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x002Au;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x002A: push(b,c,c->ax);c->ip=0x002Bu;break; /* push ax */
        case 0x002B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x002Eu;break; /* mov ax, word ptr [bp - 3] */
        case 0x002E: c->ax=(uint16_t)(arith(c,c->ax,0x0000u,0u,16u,0u));c->ip=0x0031u;break; /* add ax, 0 */
        case 0x0031: push(b,c,c->ax);c->ip=0x0032u;break; /* push ax */
        case 0x0032: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0034u;break; /* mov al, 0 */
        case 0x0034: push(b,c,c->ax);c->ip=0x0035u;break; /* push ax */
        case 0x0035: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0037u;break; /* mov al, 0 */
        case 0x0037: push(b,c,c->ax);c->ip=0x0038u;break; /* push ax */
        case 0x0038: push(b,c,c->cs);push(b,c,0x003Du);c->cs=rd16(b,c->cs,0x003Bu);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x003Du)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x003D: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x0040u;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x0040: push(b,c,c->ax);c->ip=0x0041u;break; /* push ax */
        case 0x0041: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x0044u;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x0044: push(b,c,c->ax);c->ip=0x0045u;break; /* push ax */
        case 0x0045: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x0048u;break; /* mov ax, word ptr [bp - 3] */
        case 0x0048: c->ax=(uint16_t)(arith(c,c->ax,0x0000u,0u,16u,0u));c->ip=0x004Bu;break; /* add ax, 0 */
        case 0x004B: push(b,c,c->ax);c->ip=0x004Cu;break; /* push ax */
        case 0x004C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x004Eu;break; /* mov al, 0 */
        case 0x004E: push(b,c,c->ax);c->ip=0x004Fu;break; /* push ax */
        case 0x004F: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0051u;break; /* mov al, 0 */
        case 0x0051: push(b,c,c->ax);c->ip=0x0052u;break; /* push ax */
        case 0x0052: push(b,c,c->cs);push(b,c,0x0057u);c->cs=rd16(b,c->cs,0x0055u);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0057u)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x0057: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x005Au;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x005A: push(b,c,c->ax);c->ip=0x005Bu;break; /* push ax */
        case 0x005B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x005Eu;break; /* mov al, byte ptr [bp + 8] */
        case 0x005E: push(b,c,c->ax);c->ip=0x005Fu;break; /* push ax */
        case 0x005F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x0062u;break; /* mov ax, word ptr [bp - 3] */
        case 0x0062: c->ax=(uint16_t)(arith(c,c->ax,0x0000u,0u,16u,0u));c->ip=0x0065u;break; /* add ax, 0 */
        case 0x0065: push(b,c,c->ax);c->ip=0x0066u;break; /* push ax */
        case 0x0066: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0068u;break; /* mov al, 0 */
        case 0x0068: push(b,c,c->ax);c->ip=0x0069u;break; /* push ax */
        case 0x0069: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x006Bu;break; /* mov al, 0 */
        case 0x006B: push(b,c,c->ax);c->ip=0x006Cu;break; /* push ax */
        case 0x006C: push(b,c,c->cs);push(b,c,0x0071u);c->cs=rd16(b,c->cs,0x006Fu);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0071u)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x0071: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x0074u;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x0074: push(b,c,c->ax);c->ip=0x0075u;break; /* push ax */
        case 0x0075: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x0078u;break; /* mov al, byte ptr [bp + 8] */
        case 0x0078: push(b,c,c->ax);c->ip=0x0079u;break; /* push ax */
        case 0x0079: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x007Cu;break; /* mov ax, word ptr [bp - 3] */
        case 0x007C: c->ax=(uint16_t)(arith(c,c->ax,0x0000u,0u,16u,0u));c->ip=0x007Fu;break; /* add ax, 0 */
        case 0x007F: push(b,c,c->ax);c->ip=0x0080u;break; /* push ax */
        case 0x0080: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0082u;break; /* mov al, 0 */
        case 0x0082: push(b,c,c->ax);c->ip=0x0083u;break; /* push ax */
        case 0x0083: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0085u;break; /* mov al, 0 */
        case 0x0085: push(b,c,c->ax);c->ip=0x0086u;break; /* push ax */
        case 0x0086: push(b,c,c->cs);push(b,c,0x008Bu);c->cs=rd16(b,c->cs,0x0089u);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x008Bu)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x008B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x008Eu;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x008E: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x008Fu;break; /* cwde  */
        case 0x008F: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0090u;break; /* dec ax */
        case 0x0090: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint8_t)((uint8_t)c->ax));c->ip=0x0093u;break; /* mov byte ptr [bp - 4], al */
        case 0x0093: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x0096u;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x0096: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0097u;break; /* cwde  */
        case 0x0097: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x0098u;break; /* inc ax */
        case 0x0098: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),0u,8u,1u);c->ip=0x009Bu;break; /* cmp al, byte ptr [bp - 4] */
        case 0x009B: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x00DFu;}else c->ip=0x009Du;break; /* jg 0xdf */
        case 0x009D: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)((uint8_t)c->ax));c->ip=0x00A0u;break; /* mov byte ptr [bp - 1], al */
        case 0x00A0: c->ip=0x00A5u;break; /* jmp 0xa5 */
        case 0x00A2: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu)),8u,0u)));c->ip=0x00A5u;break; /* inc byte ptr [bp - 1] */
        case 0x00A5: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x00A8u;break; /* mov al, byte ptr [bp - 1] */
        case 0x00A8: push(b,c,c->ax);c->ip=0x00A9u;break; /* push ax */
        case 0x00A9: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x00ACu;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x00AC: push(b,c,c->ax);c->ip=0x00ADu;break; /* push ax */
        case 0x00AD: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x00B0u;break; /* mov ax, word ptr [bp - 3] */
        case 0x00B0: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x00B1u;break; /* inc ax */
        case 0x00B1: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x00B2u;break; /* inc ax */
        case 0x00B2: push(b,c,c->ax);c->ip=0x00B3u;break; /* push ax */
        case 0x00B3: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x00B5u;break; /* mov al, 0 */
        case 0x00B5: push(b,c,c->ax);c->ip=0x00B6u;break; /* push ax */
        case 0x00B6: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x00B8u;break; /* mov al, 0 */
        case 0x00B8: push(b,c,c->ax);c->ip=0x00B9u;break; /* push ax */
        case 0x00B9: push(b,c,c->cs);push(b,c,0x00BEu);c->cs=rd16(b,c->cs,0x00BCu);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x00BEu)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x00BE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x00C1u;break; /* mov al, byte ptr [bp - 1] */
        case 0x00C1: push(b,c,c->ax);c->ip=0x00C2u;break; /* push ax */
        case 0x00C2: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x00C5u;break; /* mov al, byte ptr [bp + 8] */
        case 0x00C5: push(b,c,c->ax);c->ip=0x00C6u;break; /* push ax */
        case 0x00C6: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x00C9u;break; /* mov ax, word ptr [bp - 3] */
        case 0x00C9: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x00CAu;break; /* inc ax */
        case 0x00CA: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x00CBu;break; /* inc ax */
        case 0x00CB: push(b,c,c->ax);c->ip=0x00CCu;break; /* push ax */
        case 0x00CC: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x00CEu;break; /* mov al, 0 */
        case 0x00CE: push(b,c,c->ax);c->ip=0x00CFu;break; /* push ax */
        case 0x00CF: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x00D1u;break; /* mov al, 0 */
        case 0x00D1: push(b,c,c->ax);c->ip=0x00D2u;break; /* push ax */
        case 0x00D2: push(b,c,c->cs);push(b,c,0x00D7u);c->cs=rd16(b,c->cs,0x00D5u);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x00D7u)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x00D7: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x00DAu;break; /* mov al, byte ptr [bp - 1] */
        case 0x00DA: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),0u,8u,1u);c->ip=0x00DDu;break; /* cmp al, byte ptr [bp - 4] */
        case 0x00DD: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x00DDu;return STATUS(LIMIT);}++loops;c->ip=0x00A2u;}else c->ip=0x00DFu;break; /* jne 0xa2 */
        case 0x00DF: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x00E2u;break; /* mov al, byte ptr [bp + 8] */
        case 0x00E2: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x00E3u;break; /* cwde  */
        case 0x00E3: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x00E4u;break; /* dec ax */
        case 0x00E4: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint8_t)((uint8_t)c->ax));c->ip=0x00E7u;break; /* mov byte ptr [bp - 4], al */
        case 0x00E7: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x00EAu;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x00EA: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x00EBu;break; /* cwde  */
        case 0x00EB: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x00ECu;break; /* inc ax */
        case 0x00EC: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),0u,8u,1u);c->ip=0x00EFu;break; /* cmp al, byte ptr [bp - 4] */
        case 0x00EF: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x0131u;}else c->ip=0x00F1u;break; /* jg 0x131 */
        case 0x00F1: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)((uint8_t)c->ax));c->ip=0x00F4u;break; /* mov byte ptr [bp - 1], al */
        case 0x00F4: c->ip=0x00F9u;break; /* jmp 0xf9 */
        case 0x00F6: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu)),8u,0u)));c->ip=0x00F9u;break; /* inc byte ptr [bp - 1] */
        case 0x00F9: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x00FCu;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x00FC: push(b,c,c->ax);c->ip=0x00FDu;break; /* push ax */
        case 0x00FD: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x0100u;break; /* mov al, byte ptr [bp - 1] */
        case 0x0100: push(b,c,c->ax);c->ip=0x0101u;break; /* push ax */
        case 0x0101: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x0104u;break; /* mov ax, word ptr [bp - 3] */
        case 0x0104: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x0105u;break; /* inc ax */
        case 0x0105: push(b,c,c->ax);c->ip=0x0106u;break; /* push ax */
        case 0x0106: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0108u;break; /* mov al, 0 */
        case 0x0108: push(b,c,c->ax);c->ip=0x0109u;break; /* push ax */
        case 0x0109: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x010Bu;break; /* mov al, 0 */
        case 0x010B: push(b,c,c->ax);c->ip=0x010Cu;break; /* push ax */
        case 0x010C: push(b,c,c->cs);push(b,c,0x0111u);c->cs=rd16(b,c->cs,0x010Fu);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0111u)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x0111: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x0114u;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x0114: push(b,c,c->ax);c->ip=0x0115u;break; /* push ax */
        case 0x0115: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x0118u;break; /* mov al, byte ptr [bp - 1] */
        case 0x0118: push(b,c,c->ax);c->ip=0x0119u;break; /* push ax */
        case 0x0119: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFDu)));c->ip=0x011Cu;break; /* mov ax, word ptr [bp - 3] */
        case 0x011C: c->ax=(uint16_t)(incdec(c,c->ax,16u,0u));c->ip=0x011Du;break; /* inc ax */
        case 0x011D: push(b,c,c->ax);c->ip=0x011Eu;break; /* push ax */
        case 0x011E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0120u;break; /* mov al, 0 */
        case 0x0120: push(b,c,c->ax);c->ip=0x0121u;break; /* push ax */
        case 0x0121: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0123u;break; /* mov al, 0 */
        case 0x0123: push(b,c,c->ax);c->ip=0x0124u;break; /* push ax */
        case 0x0124: push(b,c,c->cs);push(b,c,0x0129u);c->cs=rd16(b,c->cs,0x0127u);c->ip=0x002Au;{int s=call_tile_stub(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0129u)return STATUS(TRANSFER);break; /* lcall 0x428, 0x2a */
        case 0x0129: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x012Cu;break; /* mov al, byte ptr [bp - 1] */
        case 0x012C: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),0u,8u,1u);c->ip=0x012Fu;break; /* cmp al, byte ptr [bp - 4] */
        case 0x012F: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x012Fu;return STATUS(LIMIT);}++loops;c->ip=0x00F6u;}else c->ip=0x0131u;break; /* jne 0xf6 */
        case 0x0131: c->sp=(uint16_t)(c->bp);c->ip=0x0133u;break; /* mov sp, bp */
        case 0x0133: c->bp=(uint16_t)(pop(b,c));c->ip=0x0134u;break; /* pop bp */
        case 0x0134: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+10u);return STATUS(OK);break; /* retf 0xa */
        default: return STATUS(TRANSFER);
        }
    }
}
