/* GAME.OVR unit 40Ch +015D tile placement, raw CPU (docs/re/OVL_040C_015D_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u40c_tile_015d_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL040C_015D_##s
typedef pr_ovl040c_015d_status pr_status_t;
typedef pr_ovl040c_015d_regs pr_cpu_t;
typedef struct {
    const pr_ovl040c_015d_env *env;
    pr_ovl040c_015d_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_r0464_video_cpu) && sizeof(pr_cpu_t) == sizeof(pr_system_text_cpu) &&
               sizeof(pr_cpu_t) == sizeof(pr_r0899_cpu) && sizeof(pr_cpu_t) == sizeof(pr_r0464_compose_cpu) &&
               sizeof(pr_cpu_t) == sizeof(pr_r0464_blit_cpu) && sizeof(pr_cpu_t) == sizeof(pr_system_leaf_cpu), "CPU layout");
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
static int meets_aperture(uint16_t seg, uint16_t off, uint16_t count)
{
    size_t lo = physical(seg, off), n;
    for (n = 0; n < count; ++n) {
        size_t at = (lo + n) & MASK;
        if (at >= 0xA0000u && at < 0xB0000u) return 1;
    }
    return 0;
}
/* Every hook is entered with the far frame pushed and CS:IP at the callee. A callee's non-success status becomes CALLEE
 * (or EXIT for QUIT's Halt and ReadKey's exits) with the CPU as the callee left it and the site recorded. */
static int stop(pr_run *r, uint16_t site, int status, int result)
{
    r->d->site = site; r->d->status = status; return result;
}
/* The error-branch callees are flat-RAM models: with an EGA attached they would bypass it, so the model stops first. */
static int flat_only(pr_bus *b, pr_run *r, uint16_t site)
{
    r->d->calls += 1u;
    return b->ega != NULL ? stop(r, site, -1, STATUS(CONTRACT)) : 0;
}
static int call_mode_00ff(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_video_cpu in; int s, intr = -1;
    if (flat_only(b, r, 0x01C5u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_restore_mode_00ff_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->video, &intr);
    memcpy(c, &in, sizeof in); r->d->service = intr;
    return s != PR_R0464_VIDEO_OK ? stop(r, 0x01C5u, s, STATUS(CALLEE)) : 0;
}
static int call_write_0919(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_text_cpu in; int s, svc = -1;
    if (flat_only(b, r, 0x01D7u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_write_str_0919(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->text, &svc);
    memcpy(c, &in, sizeof in); r->d->service = svc;
    return s != PR_SYSTEM_TEXT_RETURNED ? stop(r, 0x01D7u, s, STATUS(CALLEE)) : 0;
}
static int call_writeln_084a(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_text_cpu in; int s, svc = -1;
    if (flat_only(b, r, 0x01DCu)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_writeln_084a(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->text, &svc);
    memcpy(c, &in, sizeof in); r->d->service = svc;
    return s != PR_SYSTEM_TEXT_RETURNED ? stop(r, 0x01DCu, s, STATUS(CALLEE)) : 0;
}
static int call_readkey_0059(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0899_cpu in; int s;
    if (flat_only(b, r, 0x01E1u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0899_readkey_0059_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->key, &r->d->key);
    memcpy(c, &in, sizeof in);
    if (s == PR_R0899_KEY_RETURN) return 0;
    return stop(r, 0x01E1u, s, s == PR_R0899_KEY_EXIT || s == PR_R0899_KEY_BREAK ? STATUS(EXIT) : STATUS(CALLEE));
}
static int call_quit_0000(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0899_cpu in; int s;
    if (flat_only(b, r, 0x01E9u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0899_quit_0000_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->key.quit, &r->d->quit);
    memcpy(c, &in, sizeof in);
    return stop(r, 0x01E9u, s, s == PR_R0899_QUIT_EXIT ? STATUS(EXIT) : STATUS(CALLEE));
}
static int call_compose_08d4(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_compose_cpu in; int s;
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_compose_08d4_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->compose);
    memcpy(c, &in, sizeof in);
    return s != PR_R0464_COMPOSE_CPU_OK ? stop(r, 0x0249u, s, STATUS(CALLEE)) : 0;
}
/* SYSTEM Move 0AF8:1685 on the flat RAM (its accepted raw model), with the 08D4/1951 aperture contract. */
static int call_move_1685(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_leaf_cpu in; int s;
    r->d->calls += 1u;
    if (b->ega != NULL) {
        uint16_t count = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
        if (meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 8u)), rd16(b, c->ss, (uint16_t)(c->sp + 6u)), count) ||
            meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 12u)), rd16(b, c->ss, (uint16_t)(c->sp + 10u)), count))
            return stop(r, 0x028Bu, -1, STATUS(CONTRACT));
    }
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_move_1685(b->ram, PR_BUS_RAM_SIZE, &in);
    memcpy(c, &in, sizeof in);
    return s != 0 ? stop(r, 0x028Bu, s, STATUS(CALLEE)) : 0;
}
static int call_blit_1951(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_blit_cpu in; int s;
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_blit_1951_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->blit);
    memcpy(c, &in, sizeof in);
    return s != PR_R0464_BLIT_CPU_OK ? stop(r, 0x02A2u, s, STATUS(CALLEE)) : 0;
}

pr_ovl040c_015d_status pr_ovl040c_015d_cpu(pr_bus *b, size_t ram_size, pr_ovl040c_015d_regs *c,
                                           const pr_ovl040c_015d_env *env, pr_ovl040c_015d_diagnostic *d)
{
    pr_ovl040c_015d_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x015Du)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1; run.d->service = -1;
    run.d->compose.move_status = -1; run.d->blit.move_status = -1;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x015D: push(b,c,c->bp);c->ip=0x015Eu;break; /* push bp */
        case 0x015E: c->bp=(uint16_t)(c->sp);c->ip=0x0160u;break; /* mov bp, sp */
        case 0x0160: c->sp=(uint16_t)(arith(c,c->sp,0x0006u,0u,16u,1u));c->ip=0x0163u;break; /* sub sp, 6 */
        case 0x0163: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)));c->ip=0x0166u;break; /* mov ax, word ptr [bp + 0xa] */
        case 0x0166: (void)arith(c,c->ax,0x0001u,0u,16u,1u);c->ip=0x0169u;break; /* cmp ax, 1 */
        case 0x0169: if((c->flags&1u)){c->ip=0x0176u;}else c->ip=0x016Bu;break; /* jb 0x176 */
        case 0x016B: (void)arith(c,c->ax,0x002Du,0u,16u,1u);c->ip=0x016Eu;break; /* cmp ax, 0x2d */
        case 0x016E: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0176u;}else c->ip=0x0170u;break; /* ja 0x176 */
        case 0x0170: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x00u));c->ip=0x0174u;break; /* mov byte ptr [bp - 1], 0 */
        case 0x0174: c->ip=0x01EEu;break; /* jmp 0x1ee */
        case 0x0176: (void)arith(c,c->ax,0x002Eu,0u,16u,1u);c->ip=0x0179u;break; /* cmp ax, 0x2e */
        case 0x0179: if((c->flags&1u)){c->ip=0x0186u;}else c->ip=0x017Bu;break; /* jb 0x186 */
        case 0x017B: (void)arith(c,c->ax,0x0073u,0u,16u,1u);c->ip=0x017Eu;break; /* cmp ax, 0x73 */
        case 0x017E: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0186u;}else c->ip=0x0180u;break; /* ja 0x186 */
        case 0x0180: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x01u));c->ip=0x0184u;break; /* mov byte ptr [bp - 1], 1 */
        case 0x0184: c->ip=0x01EEu;break; /* jmp 0x1ee */
        case 0x0186: (void)arith(c,c->ax,0x0074u,0u,16u,1u);c->ip=0x0189u;break; /* cmp ax, 0x74 */
        case 0x0189: if((c->flags&1u)){c->ip=0x0196u;}else c->ip=0x018Bu;break; /* jb 0x196 */
        case 0x018B: (void)arith(c,c->ax,0x00B9u,0u,16u,1u);c->ip=0x018Eu;break; /* cmp ax, 0xb9 */
        case 0x018E: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0196u;}else c->ip=0x0190u;break; /* ja 0x196 */
        case 0x0190: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x02u));c->ip=0x0194u;break; /* mov byte ptr [bp - 1], 2 */
        case 0x0194: c->ip=0x01EEu;break; /* jmp 0x1ee */
        case 0x0196: (void)arith(c,c->ax,0x00BAu,0u,16u,1u);c->ip=0x0199u;break; /* cmp ax, 0xba */
        case 0x0199: if((c->flags&1u)){c->ip=0x01A6u;}else c->ip=0x019Bu;break; /* jb 0x1a6 */
        case 0x019B: (void)arith(c,c->ax,0x00FFu,0u,16u,1u);c->ip=0x019Eu;break; /* cmp ax, 0xff */
        case 0x019E: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x01A6u;}else c->ip=0x01A0u;break; /* ja 0x1a6 */
        case 0x01A0: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x03u));c->ip=0x01A4u;break; /* mov byte ptr [bp - 1], 3 */
        case 0x01A4: c->ip=0x01EEu;break; /* jmp 0x1ee */
        case 0x01A6: (void)arith(c,c->ax,0x0100u,0u,16u,1u);c->ip=0x01A9u;break; /* cmp ax, 0x100 */
        case 0x01A9: if((c->flags&1u)){c->ip=0x01B6u;}else c->ip=0x01ABu;break; /* jb 0x1b6 */
        case 0x01AB: (void)arith(c,c->ax,0x011Eu,0u,16u,1u);c->ip=0x01AEu;break; /* cmp ax, 0x11e */
        case 0x01AE: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x01B6u;}else c->ip=0x01B0u;break; /* ja 0x1b6 */
        case 0x01B0: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFFu),(uint8_t)(0x04u));c->ip=0x01B4u;break; /* mov byte ptr [bp - 1], 4 */
        case 0x01B4: c->ip=0x01EEu;break; /* jmp 0x1ee */
        case 0x01B6: (void)arith(c,c->ax,0x0000u,0u,16u,1u);c->ip=0x01B9u;break; /* cmp ax, 0 */
        case 0x01B9: if((c->flags&0x40u)){c->ip=0x01C5u;}else c->ip=0x01BBu;break; /* je 0x1c5 */
        case 0x01BB: (void)arith(c,c->ax,0x011Fu,0u,16u,1u);c->ip=0x01BEu;break; /* cmp ax, 0x11f */
        case 0x01BE: if((c->flags&1u)){c->ip=0x01EEu;}else c->ip=0x01C0u;break; /* jb 0x1ee */
        case 0x01C0: (void)arith(c,c->ax,0x7FFFu,0u,16u,1u);c->ip=0x01C3u;break; /* cmp ax, 0x7fff */
        case 0x01C3: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x01EEu;}else c->ip=0x01C5u;break; /* ja 0x1ee */
        case 0x01C5: push(b,c,c->cs);push(b,c,0x01CAu);c->cs=rd16(b,c->cs,0x01C8u);c->ip=0x00FFu;{int s=call_mode_00ff(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01CAu)return STATUS(TRANSFER);break; /* lcall 0x464, 0xff */
        case 0x01CA: c->di=(uint16_t)(0x8608u);c->ip=0x01CDu;break; /* mov di, 0x8608 */
        case 0x01CD: push(b,c,c->ds);c->ip=0x01CEu;break; /* push ds */
        case 0x01CE: push(b,c,c->di);c->ip=0x01CFu;break; /* push di */
        case 0x01CF: c->di=(uint16_t)(0x013Au);c->ip=0x01D2u;break; /* mov di, 0x13a */
        case 0x01D2: push(b,c,c->cs);c->ip=0x01D3u;break; /* push cs */
        case 0x01D3: push(b,c,c->di);c->ip=0x01D4u;break; /* push di */
        case 0x01D4: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x01D6u;break; /* xor ax, ax */
        case 0x01D6: push(b,c,c->ax);c->ip=0x01D7u;break; /* push ax */
        case 0x01D7: push(b,c,c->cs);push(b,c,0x01DCu);c->cs=rd16(b,c->cs,0x01DAu);c->ip=0x0919u;{int s=call_write_0919(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01DCu)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x919 */
        case 0x01DC: push(b,c,c->cs);push(b,c,0x01E1u);c->cs=rd16(b,c->cs,0x01DFu);c->ip=0x084Au;{int s=call_writeln_084a(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01E1u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x84a */
        case 0x01E1: push(b,c,c->cs);push(b,c,0x01E6u);c->cs=rd16(b,c->cs,0x01E4u);c->ip=0x0059u;{int s=call_readkey_0059(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01E6u)return STATUS(TRANSFER);break; /* lcall 0x899, 0x59 */
        case 0x01E6: wr8(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint8_t)((uint8_t)c->ax));c->ip=0x01E9u;break; /* mov byte ptr [bp - 2], al */
        case 0x01E9: push(b,c,c->cs);push(b,c,0x01EEu);c->cs=rd16(b,c->cs,0x01ECu);c->ip=0x0000u;{int s=call_quit_0000(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x01EEu)return STATUS(TRANSFER);break; /* lcall 0x899, 0 */
        case 0x01EE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x01F1u;break; /* mov al, byte ptr [bp - 1] */
        case 0x01F1: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x01F3u;break; /* xor ah, ah */
        case 0x01F3: c->di=(uint16_t)(c->ax);c->ip=0x01F5u;break; /* mov di, ax */
        case 0x01F5: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x01F7u;break; /* shl di, 1 */
        case 0x01F7: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x01F9u;break; /* shl di, 1 */
        case 0x01F9: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x5E3Au)));c->ip=0x01FDu;break; /* mov ax, word ptr [di + 0x5e3a] */
        case 0x01FD: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|rd16(b,c->ds,(uint16_t)(c->di+0x5E3Cu))),16u));c->ip=0x0201u;break; /* or ax, word ptr [di + 0x5e3c] */
        case 0x0201: if(!(c->flags&0x40u)){c->ip=0x0206u;}else c->ip=0x0203u;break; /* jne 0x206 */
        case 0x0203: c->ip=0x02A7u;break; /* jmp 0x2a7 */
        case 0x0206: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x0209u;break; /* mov al, byte ptr [bp - 1] */
        case 0x0209: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x020Bu;break; /* xor ah, ah */
        case 0x020B: c->di=(uint16_t)(c->ax);c->ip=0x020Du;break; /* mov di, ax */
        case 0x020D: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x020Fu;break; /* shl di, 1 */
        case 0x020F: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x2722u)));c->ip=0x0213u;break; /* mov ax, word ptr [di + 0x2722] */
        case 0x0213: wr16(b,c->ss,(uint16_t)(c->bp+0x000Au),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)),c->ax,0u,16u,1u)));c->ip=0x0216u;break; /* sub word ptr [bp + 0xa], ax */
        case 0x0216: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0x0008u)),0x00u,0u,8u,1u);c->ip=0x021Au;break; /* cmp byte ptr [bp + 8], 0 */
        case 0x021A: if((c->flags&0x40u)){c->ip=0x0250u;}else c->ip=0x021Cu;break; /* je 0x250 */
        case 0x021C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x021Fu;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x021F: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0220u;break; /* cwde  */
        case 0x0220: push(b,c,c->ax);c->ip=0x0221u;break; /* push ax */
        case 0x0221: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x0224u;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x0224: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0225u;break; /* cwde  */
        case 0x0225: push(b,c,c->ax);c->ip=0x0226u;break; /* push ax */
        case 0x0226: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Au))));c->ip=0x0229u;break; /* mov al, byte ptr [bp + 0xa] */
        case 0x0229: push(b,c,c->ax);c->ip=0x022Au;break; /* push ax */
        case 0x022A: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0006u))));c->ip=0x022Du;break; /* mov al, byte ptr [bp + 6] */
        case 0x022D: push(b,c,c->ax);c->ip=0x022Eu;break; /* push ax */
        case 0x022E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x0231u;break; /* mov al, byte ptr [bp - 1] */
        case 0x0231: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0233u;break; /* xor ah, ah */
        case 0x0233: c->di=(uint16_t)(c->ax);c->ip=0x0235u;break; /* mov di, ax */
        case 0x0235: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0237u;break; /* shl di, 1 */
        case 0x0237: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0239u;break; /* shl di, 1 */
        case 0x0239: push(b,c,rd16(b,c->ds,(uint16_t)(c->di+0x5E3Cu)));c->ip=0x023Du;break; /* push word ptr [di + 0x5e3c] */
        case 0x023D: push(b,c,rd16(b,c->ds,(uint16_t)(c->di+0x5E3Au)));c->ip=0x0241u;break; /* push word ptr [di + 0x5e3a] */
        case 0x0241: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A22u)));c->ip=0x0245u;break; /* push word ptr [0x4a22] */
        case 0x0245: push(b,c,rd16(b,c->ds,(uint16_t)(0x4A20u)));c->ip=0x0249u;break; /* push word ptr [0x4a20] */
        case 0x0249: push(b,c,c->cs);push(b,c,0x024Eu);c->cs=rd16(b,c->cs,0x024Cu);c->ip=0x08D4u;{int s=call_compose_08d4(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x024Eu)return STATUS(TRANSFER);break; /* lcall 0x464, 0x8d4 */
        case 0x024E: c->ip=0x02A7u;break; /* jmp 0x2a7 */
        case 0x0250: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFFu))));c->ip=0x0253u;break; /* mov al, byte ptr [bp - 1] */
        case 0x0253: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0255u;break; /* xor ah, ah */
        case 0x0255: c->di=(uint16_t)(c->ax);c->ip=0x0257u;break; /* mov di, ax */
        case 0x0257: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0259u;break; /* shl di, 1 */
        case 0x0259: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x025Bu;break; /* shl di, 1 */
        case 0x025B: {uint16_t o=(uint16_t)(c->di+0x5E3Au);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x025Fu;break; /* les di, ptr [di + 0x5e3a] */
        case 0x025F: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->di));c->ip=0x0262u;break; /* mov word ptr [bp - 6], di */
        case 0x0262: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->es));c->ip=0x0265u;break; /* mov word ptr [bp - 4], es */
        case 0x0265: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)));c->ip=0x0268u;break; /* mov ax, word ptr [bp + 0xa] */
        case 0x0268: {uint16_t o=(uint16_t)(c->bp+0xFFFAu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x026Bu;break; /* les di, ptr [bp - 6] */
        case 0x026B: mul16(c,rd16(b,c->es,(uint16_t)(c->di+0x0011u)));c->ip=0x026Fu;break; /* mul word ptr es:[di + 0x11] */
        case 0x026F: {uint16_t o=(uint16_t)(c->bp+0xFFFAu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0272u;break; /* les di, ptr [bp - 6] */
        case 0x0272: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0274u;break; /* add di, ax */
        case 0x0274: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0278u;break; /* add di, 0x17 */
        case 0x0278: push(b,c,c->es);c->ip=0x0279u;break; /* push es */
        case 0x0279: push(b,c,c->di);c->ip=0x027Au;break; /* push di */
        case 0x027A: {uint16_t o=(uint16_t)(0x5E4Eu);size_t at=physical(c->ds,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x027Eu;break; /* les di, ptr [0x5e4e] */
        case 0x027E: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0282u;break; /* add di, 0x17 */
        case 0x0282: push(b,c,c->es);c->ip=0x0283u;break; /* push es */
        case 0x0283: push(b,c,c->di);c->ip=0x0284u;break; /* push di */
        case 0x0284: {uint16_t o=(uint16_t)(c->bp+0xFFFAu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0287u;break; /* les di, ptr [bp - 6] */
        case 0x0287: push(b,c,rd16(b,c->es,(uint16_t)(c->di+0x0011u)));c->ip=0x028Bu;break; /* push word ptr es:[di + 0x11] */
        case 0x028B: push(b,c,c->cs);push(b,c,0x0290u);c->cs=rd16(b,c->cs,0x028Eu);c->ip=0x1685u;{int s=call_move_1685(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0290u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x1685 */
        case 0x0290: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x0293u;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x0293: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0294u;break; /* cwde  */
        case 0x0294: push(b,c,c->ax);c->ip=0x0295u;break; /* push ax */
        case 0x0295: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Cu))));c->ip=0x0298u;break; /* mov al, byte ptr [bp + 0xc] */
        case 0x0298: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x0299u;break; /* cwde  */
        case 0x0299: push(b,c,c->ax);c->ip=0x029Au;break; /* push ax */
        case 0x029A: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E50u)));c->ip=0x029Eu;break; /* push word ptr [0x5e50] */
        case 0x029E: push(b,c,rd16(b,c->ds,(uint16_t)(0x5E4Eu)));c->ip=0x02A2u;break; /* push word ptr [0x5e4e] */
        case 0x02A2: push(b,c,c->cs);push(b,c,0x02A7u);c->cs=rd16(b,c->cs,0x02A5u);c->ip=0x1951u;{int s=call_blit_1951(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x02A7u)return STATUS(TRANSFER);break; /* lcall 0x464, 0x1951 */
        case 0x02A7: c->sp=(uint16_t)(c->bp);c->ip=0x02A9u;break; /* mov sp, bp */
        case 0x02A9: c->bp=(uint16_t)(pop(b,c));c->ip=0x02AAu;break; /* pop bp */
        case 0x02AA: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+10u);return STATUS(OK);break; /* retf 0xa */
        default: return STATUS(TRANSFER);
        }
    }
}
