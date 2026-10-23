/* GAME.OVR unit 3CEh +00F1 tile placement, raw CPU (docs/re/OVL_03CE_00F1_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u3ce_tile_00f1_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL03CE_00F1_##s
typedef pr_ovl03ce_00f1_status pr_status_t;
typedef pr_ovl03ce_00f1_regs pr_cpu_t;
typedef struct {
    const pr_ovl03ce_00f1_env *env;
    pr_ovl03ce_00f1_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_r0464_compose_cpu), "CPU layout");
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(pr_bus *b, size_t at)
{
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static uint8_t rd8(pr_bus *b, uint16_t seg, uint16_t off) { return pr_bus_read8(b, physical(seg, off)); }
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
/* 0464:08D4 on the same bus (its accepted raw model); a non-OK status stops here with the site recorded. */
static int call_compose_08d4(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_compose_cpu in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u);
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_compose_08d4_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->compose);
    memcpy(c, &in, sizeof in);
    if (s == PR_R0464_COMPOSE_CPU_OK) return 0;
    r->d->site = site; r->d->status = s; return STATUS(CALLEE);
}

pr_ovl03ce_00f1_status pr_ovl03ce_00f1_cpu(pr_bus *b, size_t ram_size, pr_ovl03ce_00f1_regs *c,
                                           const pr_ovl03ce_00f1_env *env, pr_ovl03ce_00f1_diagnostic *d)
{
    pr_ovl03ce_00f1_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x00F1u)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1; run.d->compose.move_status = -1;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x00F1: push(b,c,c->bp);c->ip=0x00F2u;break; /* push bp */
        case 0x00F2: c->bp=(uint16_t)(c->sp);c->ip=0x00F4u;break; /* mov bp, sp */
        case 0x00F4: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0x0008u)),0x7Fu,0u,8u,1u);c->ip=0x00F8u;break; /* cmp byte ptr [bp + 8], 0x7f */
        case 0x00F8: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0124u;}else c->ip=0x00FAu;break; /* jbe 0x124 */
        case 0x00FA: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x00FDu;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x00FD: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x00FEu;break; /* cwde  */
        case 0x00FE: push(b,c,c->ax);c->ip=0x00FFu;break; /* push ax */
        case 0x00FF: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x0102u;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x0102: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0103u;break; /* cwde  */
        case 0x0103: push(b,c,c->ax);c->ip=0x0104u;break; /* push ax */
        case 0x0104: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x0107u;break; /* mov al, byte ptr [bp + 8] */
        case 0x0107: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x7Fu),8u)));c->ip=0x0109u;break; /* and al, 0x7f */
        case 0x0109: push(b,c,c->ax);c->ip=0x010Au;break; /* push ax */
        case 0x010A: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x010Cu;break; /* mov al, 0 */
        case 0x010C: push(b,c,c->ax);c->ip=0x010Du;break; /* push ax */
        case 0x010D: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E58u)));c->ip=0x0111u;break; /* push word ptr [0x5e58] */
        case 0x0111: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E56u)));c->ip=0x0115u;break; /* push word ptr [0x5e56] */
        case 0x0115: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A22u)));c->ip=0x0119u;break; /* push word ptr [0x4a22] */
        case 0x0119: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A20u)));c->ip=0x011Du;break; /* push word ptr [0x4a20] */
        case 0x011D: push(b,c,c->cs);push(b,c,0x0122u);c->cs=rd16(b,c->cs,0x0120u);c->ip=0x08D4u;{int s=call_compose_08d4(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0122u)return STATUS(TRANSFER);break; /* lcall 0x464, 0x8d4 */
        case 0x0122: c->ip=0x014Au;break; /* jmp 0x14a */
        case 0x0124: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x0127u;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x0127: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0128u;break; /* cwde  */
        case 0x0128: push(b,c,c->ax);c->ip=0x0129u;break; /* push ax */
        case 0x0129: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x012Cu;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x012C: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x012Du;break; /* cwde  */
        case 0x012D: push(b,c,c->ax);c->ip=0x012Eu;break; /* push ax */
        case 0x012E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0008u))));c->ip=0x0131u;break; /* mov al, byte ptr [bp + 8] */
        case 0x0131: push(b,c,c->ax);c->ip=0x0132u;break; /* push ax */
        case 0x0132: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x0134u;break; /* mov al, 0 */
        case 0x0134: push(b,c,c->ax);c->ip=0x0135u;break; /* push ax */
        case 0x0135: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E54u)));c->ip=0x0139u;break; /* push word ptr [0x5e54] */
        case 0x0139: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E52u)));c->ip=0x013Du;break; /* push word ptr [0x5e52] */
        case 0x013D: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A22u)));c->ip=0x0141u;break; /* push word ptr [0x4a22] */
        case 0x0141: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A20u)));c->ip=0x0145u;break; /* push word ptr [0x4a20] */
        case 0x0145: push(b,c,c->cs);push(b,c,0x014Au);c->cs=rd16(b,c->cs,0x0148u);c->ip=0x08D4u;{int s=call_compose_08d4(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x014Au)return STATUS(TRANSFER);break; /* lcall 0x464, 0x8d4 */
        case 0x014A: c->bp=(uint16_t)(pop(b,c));c->ip=0x014Bu;break; /* pop bp */
        case 0x014B: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+8u);return STATUS(OK);break; /* retf 8 */
        default: return STATUS(TRANSFER);
        }
    }
}
