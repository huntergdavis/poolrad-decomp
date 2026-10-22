/* START.EXE resident 0464:08D4 tile compositor, raw CPU (docs/re/RES_0464_COMPOSE_08D4_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R0464_COMPOSE_CPU_##s
typedef pr_r0464_compose_cpu_status pr_status_t;
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
static void push(pr_bus *b, pr_r0464_compose_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, pr_r0464_compose_cpu *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void szp(pr_r0464_compose_cpu *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t arith(pr_r0464_compose_cpu *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
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
static uint16_t logic(pr_r0464_compose_cpu *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
static uint16_t incdec(pr_r0464_compose_cpu *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t cf = c->flags & 1u; v = arith(c, v, 1u, 0u, bits, dec);
    c->flags = (uint16_t)((c->flags & ~1u) | cf); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1,
 * Unicorn's rule otherwise); AF := 0 (Unicorn). The report in the doc shows where these are observed. */
static uint16_t shift(pr_r0464_compose_cpu *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
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
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_r0464_compose_cpu *c, uint16_t src)
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
/* SYSTEM Move 0AF8:1685 on the flat RAM (its accepted raw model). Entered with the far frame pushed. */
static int call_move(pr_bus *b, pr_r0464_compose_cpu *c, pr_r0464_compose_cpu_diagnostic *diag)
{
    pr_system_leaf_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    if (b->ega != NULL) {
        uint16_t count = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
        if (meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 8u)), rd16(b, c->ss, (uint16_t)(c->sp + 6u)), count) ||
            meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 12u)), rd16(b, c->ss, (uint16_t)(c->sp + 10u)), count))
            return STATUS(CONTRACT);
    }
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_move_1685(b->ram, PR_BUS_RAM_SIZE, &inner);
    memcpy(c, &inner, sizeof inner);
    diag->move_calls += 1; diag->move_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}

pr_r0464_compose_cpu_status pr_r0464_compose_08d4_cpu(pr_bus *b, size_t ram_size, pr_r0464_compose_cpu *c,
                                                 size_t max_loopbacks, pr_r0464_compose_cpu_diagnostic *d)
{
    pr_r0464_compose_cpu_diagnostic fallback = {0, -1};
    uint16_t caller; size_t loops = 0;
    if (b == NULL || b->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x08D4u)
        return STATUS(ARGUMENT);
    pr_r0464_compose_cpu_diagnostic *dg = d != NULL ? d : &fallback;
    *dg = fallback;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x08D4: push(b,c,c->bp);c->ip=0x08D5u;break; /* push bp */
        case 0x08D5: c->bp=(uint16_t)(c->sp);c->ip=0x08D7u;break; /* mov bp, sp */
        case 0x08D7: c->sp=(uint16_t)(arith(c,c->sp,0x001Au,0u,16u,1u));c->ip=0x08DAu;break; /* sub sp, 0x1a */
        case 0x08DA: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x000Eu))));c->ip=0x08DDu;break; /* mov al, byte ptr [bp + 0xe] */
        case 0x08DD: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x01u),8u)));c->ip=0x08DFu;break; /* and al, 1 */
        case 0x08DF: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x08E1u;break; /* or al, al */
        case 0x08E1: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x08F0u;}else c->ip=0x08E3u;break; /* jbe 0x8f0 */
        case 0x08E3: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x08E6u;break; /* les di, ptr [bp + 0xa] */
        case 0x08E6: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0013u)));c->ip=0x08EAu;break; /* mov ax, word ptr es:[di + 0x13] */
        case 0x08EA: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|rd16(b,c->es,(uint16_t)(c->di+0x0015u))),16u));c->ip=0x08EEu;break; /* or ax, word ptr es:[di + 0x15] */
        case 0x08EE: if(!(c->flags&0x40u)){c->ip=0x08F4u;}else c->ip=0x08F0u;break; /* jne 0x8f4 */
        case 0x08F0: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u));c->ip=0x08F2u;break; /* mov al, 0 */
        case 0x08F2: c->ip=0x08F6u;break; /* jmp 0x8f6 */
        case 0x08F4: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x08F6u;break; /* mov al, 1 */
        case 0x08F6: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEAu),(uint8_t)((uint8_t)c->ax));c->ip=0x08F9u;break; /* mov byte ptr [bp - 0x16], al */
        case 0x08F9: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0012u)));c->ip=0x08FCu;break; /* mov ax, word ptr [bp + 0x12] */
        case 0x08FC: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x03u));c->ip=0x08FEu;break; /* mov cl, 3 */
        case 0x08FE: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,0u));c->ip=0x0900u;break; /* shl ax, cl */
        case 0x0900: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(c->ax));c->ip=0x0903u;break; /* mov word ptr [bp - 0x13], ax */
        case 0x0903: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0906u;break; /* mov ax, word ptr [bp - 0x13] */
        case 0x0906: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0909u;break; /* les di, ptr [bp + 6] */
        case 0x0909: mul16(c,rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x090Du;break; /* mul word ptr es:[di + 2] */
        case 0x090D: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(c->ax));c->ip=0x0910u;break; /* mov word ptr [bp - 0xa], ax */
        case 0x0910: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0913u;break; /* les di, ptr [bp + 0xa] */
        case 0x0913: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di)));c->ip=0x0916u;break; /* mov ax, word ptr es:[di] */
        case 0x0916: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0917u;break; /* dec ax */
        case 0x0917: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF4u),(uint16_t)(c->ax));c->ip=0x091Au;break; /* mov word ptr [bp - 0xc], ax */
        case 0x091A: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0014u)));c->ip=0x091Du;break; /* mov ax, word ptr [bp + 0x14] */
        case 0x091D: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x0920u;break; /* mov word ptr [bp - 2], ax */
        case 0x0920: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0923u;break; /* les di, ptr [bp + 0xa] */
        case 0x0923: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x0927u;break; /* mov ax, word ptr es:[di + 2] */
        case 0x0927: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x0929u;break; /* xor dx, dx */
        case 0x0929: c->cx=(uint16_t)(c->ax);c->ip=0x092Bu;break; /* mov cx, ax */
        case 0x092B: c->bx=(uint16_t)(c->dx);c->ip=0x092Du;break; /* mov bx, dx */
        case 0x092D: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0014u)));c->ip=0x0930u;break; /* mov ax, word ptr [bp + 0x14] */
        case 0x0930: c->dx=(c->ax&0x8000u)?0xFFFFu:0u;c->ip=0x0931u;break; /* cdq  */
        case 0x0931: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,0u));c->ip=0x0933u;break; /* add ax, cx */
        case 0x0933: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,0u));c->ip=0x0935u;break; /* adc dx, bx */
        case 0x0935: c->cx=(uint16_t)(c->ax);c->ip=0x0937u;break; /* mov cx, ax */
        case 0x0937: c->bx=(uint16_t)(c->dx);c->ip=0x0939u;break; /* mov bx, dx */
        case 0x0939: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x093Cu;break; /* les di, ptr [bp + 6] */
        case 0x093C: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x0940u;break; /* mov ax, word ptr es:[di + 2] */
        case 0x0940: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x0942u;break; /* xor dx, dx */
        case 0x0942: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,1u));c->ip=0x0944u;break; /* sub ax, cx */
        case 0x0944: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,1u));c->ip=0x0946u;break; /* sbb dx, bx */
        case 0x0946: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x0949u;break; /* mov word ptr [bp - 4], ax */
        case 0x0949: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x4A1Eu))));c->ip=0x094Cu;break; /* mov al, byte ptr [0x4a1e] */
        case 0x094C: (void)arith(c,(uint8_t)c->ax,0x00u,0u,8u,1u);c->ip=0x094Eu;break; /* cmp al, 0 */
        case 0x094E: if((c->flags&0x40u)){c->ip=0x0957u;}else c->ip=0x0950u;break; /* je 0x957 */
        case 0x0950: (void)arith(c,(uint8_t)c->ax,0x02u,0u,8u,1u);c->ip=0x0952u;break; /* cmp al, 2 */
        case 0x0952: if((c->flags&0x40u)){c->ip=0x0957u;}else c->ip=0x0954u;break; /* je 0x957 */
        case 0x0954: c->ip=0x0B74u;break; /* jmp 0xb74 */
        case 0x0957: (void)arith(c,rd8(b,c->ds,(uint16_t)(0x4A1Eu)),0x00u,0u,8u,1u);c->ip=0x095Cu;break; /* cmp byte ptr [0x4a1e], 0 */
        case 0x095C: if(!(c->flags&0x40u)){c->ip=0x099Bu;}else c->ip=0x095Eu;break; /* jne 0x99b */
        case 0x095E: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0961u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0961: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0963u;break; /* shl ax, 1 */
        case 0x0963: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEBu),(uint16_t)(c->ax));c->ip=0x0966u;break; /* mov word ptr [bp - 0x15], ax */
        case 0x0966: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0969u;break; /* les di, ptr [bp + 0xa] */
        case 0x0969: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x096Du;break; /* mov ax, word ptr es:[di + 2] */
        case 0x096D: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x096Fu;break; /* shl ax, 1 */
        case 0x096F: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x0972u;break; /* mov word ptr [bp - 6], ax */
        case 0x0972: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0975u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0975: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0977u;break; /* shl ax, 1 */
        case 0x0977: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x097Au;break; /* mov word ptr [bp - 2], ax */
        case 0x097A: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x097Du;break; /* mov ax, word ptr [bp - 4] */
        case 0x097D: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x097Fu;break; /* shl ax, 1 */
        case 0x097F: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x0982u;break; /* mov word ptr [bp - 4], ax */
        case 0x0982: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0010u))));c->ip=0x0985u;break; /* mov al, byte ptr [bp + 0x10] */
        case 0x0985: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0987u;break; /* xor ah, ah */
        case 0x0987: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x098Au;break; /* les di, ptr [bp + 0xa] */
        case 0x098A: mul16(c,rd16(b,c->es,(uint16_t)(c->di+0x0011u)));c->ip=0x098Eu;break; /* mul word ptr es:[di + 0x11] */
        case 0x098E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x0991u;break; /* mov word ptr [bp - 8], ax */
        case 0x0991: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0994u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0994: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0996u;break; /* shl ax, 1 */
        case 0x0996: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(c->ax));c->ip=0x0999u;break; /* mov word ptr [bp - 0xa], ax */
        case 0x0999: c->ip=0x09E0u;break; /* jmp 0x9e0 */
        case 0x099B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x099Eu;break; /* mov ax, word ptr [bp - 2] */
        case 0x099E: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09A0u;break; /* shl ax, 1 */
        case 0x09A0: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09A2u;break; /* shl ax, 1 */
        case 0x09A2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEBu),(uint16_t)(c->ax));c->ip=0x09A5u;break; /* mov word ptr [bp - 0x15], ax */
        case 0x09A5: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x09A8u;break; /* les di, ptr [bp + 0xa] */
        case 0x09A8: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x09ACu;break; /* mov ax, word ptr es:[di + 2] */
        case 0x09AC: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09AEu;break; /* shl ax, 1 */
        case 0x09AE: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09B0u;break; /* shl ax, 1 */
        case 0x09B0: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x09B3u;break; /* mov word ptr [bp - 6], ax */
        case 0x09B3: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x09B6u;break; /* mov ax, word ptr [bp - 2] */
        case 0x09B6: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09B8u;break; /* shl ax, 1 */
        case 0x09B8: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09BAu;break; /* shl ax, 1 */
        case 0x09BA: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x09BDu;break; /* mov word ptr [bp - 2], ax */
        case 0x09BD: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x09C0u;break; /* mov ax, word ptr [bp - 4] */
        case 0x09C0: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09C2u;break; /* shl ax, 1 */
        case 0x09C2: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09C4u;break; /* shl ax, 1 */
        case 0x09C4: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x09C7u;break; /* mov word ptr [bp - 4], ax */
        case 0x09C7: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0010u))));c->ip=0x09CAu;break; /* mov al, byte ptr [bp + 0x10] */
        case 0x09CA: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x09CCu;break; /* xor ah, ah */
        case 0x09CC: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x09CFu;break; /* les di, ptr [bp + 0xa] */
        case 0x09CF: mul16(c,rd16(b,c->es,(uint16_t)(c->di+0x0011u)));c->ip=0x09D3u;break; /* mul word ptr es:[di + 0x11] */
        case 0x09D3: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x09D6u;break; /* mov word ptr [bp - 8], ax */
        case 0x09D6: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x09D9u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x09D9: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09DBu;break; /* shl ax, 1 */
        case 0x09DB: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x09DDu;break; /* shl ax, 1 */
        case 0x09DD: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(c->ax));c->ip=0x09E0u;break; /* mov word ptr [bp - 0xa], ax */
        case 0x09E0: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEAu)),0x00u,0u,8u,1u);c->ip=0x09E4u;break; /* cmp byte ptr [bp - 0x16], 0 */
        case 0x09E4: if(!(c->flags&0x40u)){c->ip=0x09E9u;}else c->ip=0x09E6u;break; /* jne 0x9e9 */
        case 0x09E6: c->ip=0x0AC5u;break; /* jmp 0xac5 */
        case 0x09E9: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x09ECu;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x09EC: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE8u),(uint16_t)(c->ax));c->ip=0x09EFu;break; /* mov word ptr [bp - 0x18], ax */
        case 0x09EF: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x09F1u;break; /* xor ax, ax */
        case 0x09F1: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x09F4u;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x09F4: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x09F9u;}else c->ip=0x09F6u;break; /* jbe 0x9f9 */
        case 0x09F6: c->ip=0x0AC2u;break; /* jmp 0xac2 */
        case 0x09F9: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x09FCu;break; /* mov word ptr [bp - 0xe], ax */
        case 0x09FC: c->ip=0x0A01u;break; /* jmp 0xa01 */
        case 0x09FE: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)),16u,0u)));c->ip=0x0A01u;break; /* inc word ptr [bp - 0xe] */
        case 0x0A01: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0A04u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0A04: wr8(b,c->ds,(uint16_t)(c->di+0x4A28u),(uint8_t)(0x01u));c->ip=0x0A09u;break; /* mov byte ptr [di + 0x4a28], 1 */
        case 0x0A09: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0A0Cu;break; /* mov ax, word ptr [bp - 2] */
        case 0x0A0C: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0A0Fu;break; /* add word ptr [bp - 0xa], ax */
        case 0x0A0F: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0A12u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0A12: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0A14u;break; /* shl di, 1 */
        case 0x0A14: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x4C20u)));c->ip=0x0A18u;break; /* mov ax, word ptr [di + 0x4c20] */
        case 0x0A18: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),0u,16u,1u);c->ip=0x0A1Bu;break; /* cmp ax, word ptr [bp - 0xa] */
        case 0x0A1B: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0A35u;}else c->ip=0x0A1Du;break; /* jbe 0xa35 */
        case 0x0A1D: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEBu)));c->ip=0x0A20u;break; /* mov ax, word ptr [bp - 0x15] */
        case 0x0A20: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0A23u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0A23: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0A25u;break; /* shl di, 1 */
        case 0x0A25: wr16(b,c->ds,(uint16_t)(c->di+0x4AD0u),(uint16_t)(c->ax));c->ip=0x0A29u;break; /* mov word ptr [di + 0x4ad0], ax */
        case 0x0A29: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0A2Cu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0A2C: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0A2Fu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0A2F: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0A31u;break; /* shl di, 1 */
        case 0x0A31: wr16(b,c->ds,(uint16_t)(c->di+0x4C20u),(uint16_t)(c->ax));c->ip=0x0A35u;break; /* mov word ptr [di + 0x4c20], ax */
        case 0x0A35: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFAu))));c->ip=0x0A38u;break; /* mov al, byte ptr [bp - 6] */
        case 0x0A38: wr8(b,c->ss,(uint16_t)(c->bp+0xFFE7u),(uint8_t)((uint8_t)c->ax));c->ip=0x0A3Bu;break; /* mov byte ptr [bp - 0x19], al */
        case 0x0A3B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x0A3Du;break; /* mov al, 1 */
        case 0x0A3D: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,8u,1u);c->ip=0x0A40u;break; /* cmp al, byte ptr [bp - 0x19] */
        case 0x0A40: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x0A92u;}else c->ip=0x0A42u;break; /* jg 0xa92 */
        case 0x0A42: wr8(b,c->ds,(uint16_t)(0x70C7u),(uint8_t)((uint8_t)c->ax));c->ip=0x0A45u;break; /* mov byte ptr [0x70c7], al */
        case 0x0A45: c->ip=0x0A4Bu;break; /* jmp 0xa4b */
        case 0x0A47: wr8(b,c->ds,(uint16_t)(0x70C7u),(uint8_t)(incdec(c,rd8(b,c->ds,(uint16_t)(0x70C7u)),8u,0u)));c->ip=0x0A4Bu;break; /* inc byte ptr [0x70c7] */
        case 0x0A4B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0A4Eu;break; /* mov ax, word ptr [bp - 8] */
        case 0x0A4E: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0A51u;break; /* les di, ptr [bp + 0xa] */
        case 0x0A51: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0A53u;break; /* add di, ax */
        case 0x0A53: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0017u))));c->ip=0x0A57u;break; /* mov cl, byte ptr es:[di + 0x17] */
        case 0x0A57: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0A5Au;break; /* mov ax, word ptr [bp - 8] */
        case 0x0A5A: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0A5Du;break; /* les di, ptr [bp + 0xa] */
        case 0x0A5D: {uint16_t o=(uint16_t)(c->di+0x0013u);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0A61u;break; /* les di, ptr es:[di + 0x13] */
        case 0x0A61: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0A63u;break; /* add di, ax */
        case 0x0A63: c->dx=(uint16_t)((c->dx&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di))));c->ip=0x0A66u;break; /* mov dl, byte ptr es:[di] */
        case 0x0A66: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0A69u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0A69: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0A6Cu;break; /* les di, ptr [bp + 6] */
        case 0x0A6C: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0A6Eu;break; /* add di, ax */
        case 0x0A6E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0017u))));c->ip=0x0A72u;break; /* mov al, byte ptr es:[di + 0x17] */
        case 0x0A72: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&(uint8_t)c->dx),8u)));c->ip=0x0A74u;break; /* and al, dl */
        case 0x0A74: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->cx),8u)));c->ip=0x0A76u;break; /* or al, cl */
        case 0x0A76: c->dx=(uint16_t)((c->dx&0xFF00u)|(uint8_t)((uint8_t)c->ax));c->ip=0x0A78u;break; /* mov dl, al */
        case 0x0A78: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0A7Bu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0A7B: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0A7Eu;break; /* les di, ptr [bp + 6] */
        case 0x0A7E: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0A80u;break; /* add di, ax */
        case 0x0A80: wr8(b,c->es,(uint16_t)(c->di+0x0017u),(uint8_t)((uint8_t)c->dx));c->ip=0x0A84u;break; /* mov byte ptr es:[di + 0x17], dl */
        case 0x0A84: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),16u,0u)));c->ip=0x0A87u;break; /* inc word ptr [bp - 8] */
        case 0x0A87: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),16u,0u)));c->ip=0x0A8Au;break; /* inc word ptr [bp - 0xa] */
        case 0x0A8A: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x70C7u))));c->ip=0x0A8Du;break; /* mov al, byte ptr [0x70c7] */
        case 0x0A8D: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,8u,1u);c->ip=0x0A90u;break; /* cmp al, byte ptr [bp - 0x19] */
        case 0x0A90: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x0A90u;return STATUS(LIMIT);}++loops;c->ip=0x0A47u;}else c->ip=0x0A92u;break; /* jne 0xa47 */
        case 0x0A92: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0A95u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0A95: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0A96u;break; /* dec ax */
        case 0x0A96: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0A99u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0A99: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0A9Bu;break; /* shl di, 1 */
        case 0x0A9B: (void)arith(c,c->ax,rd16(b,c->ds,(uint16_t)(c->di+0x4D70u)),0u,16u,1u);c->ip=0x0A9Fu;break; /* cmp ax, word ptr [di + 0x4d70] */
        case 0x0A9F: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0AAEu;}else c->ip=0x0AA1u;break; /* jbe 0xaae */
        case 0x0AA1: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0AA4u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0AA4: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0AA5u;break; /* dec ax */
        case 0x0AA5: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0AA8u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0AA8: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0AAAu;break; /* shl di, 1 */
        case 0x0AAA: wr16(b,c->ds,(uint16_t)(c->di+0x4D70u),(uint16_t)(c->ax));c->ip=0x0AAEu;break; /* mov word ptr [di + 0x4d70], ax */
        case 0x0AAE: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0AB1u;break; /* mov ax, word ptr [bp - 4] */
        case 0x0AB1: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0AB4u;break; /* add word ptr [bp - 0xa], ax */
        case 0x0AB4: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)),16u,0u)));c->ip=0x0AB7u;break; /* inc word ptr [bp - 0x13] */
        case 0x0AB7: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x0ABAu;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x0ABA: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0ABDu;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0ABD: if((c->flags&0x40u)){c->ip=0x0AC2u;}else c->ip=0x0ABFu;break; /* je 0xac2 */
        case 0x0ABF: if(loops==max_loopbacks){c->ip=0x0ABFu;return STATUS(LIMIT);}++loops;c->ip=0x09FEu;break; /* jmp 0x9fe */
        case 0x0AC2: c->ip=0x0B71u;break; /* jmp 0xb71 */
        case 0x0AC5: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x0AC8u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x0AC8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE8u),(uint16_t)(c->ax));c->ip=0x0ACBu;break; /* mov word ptr [bp - 0x18], ax */
        case 0x0ACB: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x0ACDu;break; /* xor ax, ax */
        case 0x0ACD: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0AD0u;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0AD0: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0AD5u;}else c->ip=0x0AD2u;break; /* jbe 0xad5 */
        case 0x0AD2: c->ip=0x0B71u;break; /* jmp 0xb71 */
        case 0x0AD5: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x0AD8u;break; /* mov word ptr [bp - 0xe], ax */
        case 0x0AD8: c->ip=0x0ADDu;break; /* jmp 0xadd */
        case 0x0ADA: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)),16u,0u)));c->ip=0x0ADDu;break; /* inc word ptr [bp - 0xe] */
        case 0x0ADD: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0AE0u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0AE0: wr8(b,c->ds,(uint16_t)(c->di+0x4A28u),(uint8_t)(0x01u));c->ip=0x0AE5u;break; /* mov byte ptr [di + 0x4a28], 1 */
        case 0x0AE5: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0AE8u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0AE8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0AEBu;break; /* add word ptr [bp - 0xa], ax */
        case 0x0AEB: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0AEEu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0AEE: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0AF0u;break; /* shl di, 1 */
        case 0x0AF0: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x4C20u)));c->ip=0x0AF4u;break; /* mov ax, word ptr [di + 0x4c20] */
        case 0x0AF4: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),0u,16u,1u);c->ip=0x0AF7u;break; /* cmp ax, word ptr [bp - 0xa] */
        case 0x0AF7: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0B11u;}else c->ip=0x0AF9u;break; /* jbe 0xb11 */
        case 0x0AF9: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEBu)));c->ip=0x0AFCu;break; /* mov ax, word ptr [bp - 0x15] */
        case 0x0AFC: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0AFFu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0AFF: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0B01u;break; /* shl di, 1 */
        case 0x0B01: wr16(b,c->ds,(uint16_t)(c->di+0x4AD0u),(uint16_t)(c->ax));c->ip=0x0B05u;break; /* mov word ptr [di + 0x4ad0], ax */
        case 0x0B05: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0B08u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0B08: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0B0Bu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0B0B: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0B0Du;break; /* shl di, 1 */
        case 0x0B0D: wr16(b,c->ds,(uint16_t)(c->di+0x4C20u),(uint16_t)(c->ax));c->ip=0x0B11u;break; /* mov word ptr [di + 0x4c20], ax */
        case 0x0B11: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0B14u;break; /* mov ax, word ptr [bp - 8] */
        case 0x0B14: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0B17u;break; /* les di, ptr [bp + 0xa] */
        case 0x0B17: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0B19u;break; /* add di, ax */
        case 0x0B19: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0B1Du;break; /* add di, 0x17 */
        case 0x0B1D: push(b,c,c->es);c->ip=0x0B1Eu;break; /* push es */
        case 0x0B1E: push(b,c,c->di);c->ip=0x0B1Fu;break; /* push di */
        case 0x0B1F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0B22u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0B22: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0B25u;break; /* les di, ptr [bp + 6] */
        case 0x0B25: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0B27u;break; /* add di, ax */
        case 0x0B27: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0B2Bu;break; /* add di, 0x17 */
        case 0x0B2B: push(b,c,c->es);c->ip=0x0B2Cu;break; /* push es */
        case 0x0B2C: push(b,c,c->di);c->ip=0x0B2Du;break; /* push di */
        case 0x0B2D: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0B30u;break; /* push word ptr [bp - 6] */
        case 0x0B30: push(b,c,c->cs);push(b,c,0x0B35u);c->cs=rd16(b,c->cs,0x0B33u);c->ip=0x1685u;{int s=call_move(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0B35u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x1685 */
        case 0x0B35: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0B38u;break; /* mov ax, word ptr [bp - 6] */
        case 0x0B38: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),c->ax,0u,16u,0u)));c->ip=0x0B3Bu;break; /* add word ptr [bp - 8], ax */
        case 0x0B3B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0B3Eu;break; /* mov ax, word ptr [bp - 6] */
        case 0x0B3E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0B41u;break; /* add word ptr [bp - 0xa], ax */
        case 0x0B41: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0B44u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0B44: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0B45u;break; /* dec ax */
        case 0x0B45: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0B48u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0B48: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0B4Au;break; /* shl di, 1 */
        case 0x0B4A: (void)arith(c,c->ax,rd16(b,c->ds,(uint16_t)(c->di+0x4D70u)),0u,16u,1u);c->ip=0x0B4Eu;break; /* cmp ax, word ptr [di + 0x4d70] */
        case 0x0B4E: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0B5Du;}else c->ip=0x0B50u;break; /* jbe 0xb5d */
        case 0x0B50: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0B53u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0B53: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0B54u;break; /* dec ax */
        case 0x0B54: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0B57u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0B57: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0B59u;break; /* shl di, 1 */
        case 0x0B59: wr16(b,c->ds,(uint16_t)(c->di+0x4D70u),(uint16_t)(c->ax));c->ip=0x0B5Du;break; /* mov word ptr [di + 0x4d70], ax */
        case 0x0B5D: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0B60u;break; /* mov ax, word ptr [bp - 4] */
        case 0x0B60: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0B63u;break; /* add word ptr [bp - 0xa], ax */
        case 0x0B63: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)),16u,0u)));c->ip=0x0B66u;break; /* inc word ptr [bp - 0x13] */
        case 0x0B66: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x0B69u;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x0B69: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0B6Cu;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0B6C: if((c->flags&0x40u)){c->ip=0x0B71u;}else c->ip=0x0B6Eu;break; /* je 0xb71 */
        case 0x0B6E: if(loops==max_loopbacks){c->ip=0x0B6Eu;return STATUS(LIMIT);}++loops;c->ip=0x0ADAu;break; /* jmp 0xada */
        case 0x0B71: c->ip=0x0D6Bu;break; /* jmp 0xd6b */
        case 0x0B74: (void)arith(c,(uint8_t)c->ax,0x01u,0u,8u,1u);c->ip=0x0B76u;break; /* cmp al, 1 */
        case 0x0B76: if((c->flags&0x40u)){c->ip=0x0B7Bu;}else c->ip=0x0B78u;break; /* je 0xb7b */
        case 0x0B78: c->ip=0x0D6Bu;break; /* jmp 0xd6b */
        case 0x0B7B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0B7Eu;break; /* mov ax, word ptr [bp - 2] */
        case 0x0B7E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEBu),(uint16_t)(c->ax));c->ip=0x0B81u;break; /* mov word ptr [bp - 0x15], ax */
        case 0x0B81: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0B84u;break; /* les di, ptr [bp + 0xa] */
        case 0x0B84: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x0B88u;break; /* mov ax, word ptr es:[di + 2] */
        case 0x0B88: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0B8Au;break; /* shl ax, 1 */
        case 0x0B8A: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0B8Cu;break; /* shl ax, 1 */
        case 0x0B8C: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x0B8Fu;break; /* mov word ptr [bp - 6], ax */
        case 0x0B8F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0B92u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0B92: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0B94u;break; /* shl ax, 1 */
        case 0x0B94: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0B96u;break; /* shl ax, 1 */
        case 0x0B96: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x0B99u;break; /* mov word ptr [bp - 2], ax */
        case 0x0B99: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0B9Cu;break; /* mov ax, word ptr [bp - 4] */
        case 0x0B9C: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0B9Eu;break; /* shl ax, 1 */
        case 0x0B9E: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0BA0u;break; /* shl ax, 1 */
        case 0x0BA0: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x0BA3u;break; /* mov word ptr [bp - 4], ax */
        case 0x0BA3: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0x0010u))));c->ip=0x0BA6u;break; /* mov al, byte ptr [bp + 0x10] */
        case 0x0BA6: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0BA8u;break; /* xor ah, ah */
        case 0x0BA8: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0BABu;break; /* les di, ptr [bp + 0xa] */
        case 0x0BAB: mul16(c,rd16(b,c->es,(uint16_t)(c->di+0x0011u)));c->ip=0x0BAFu;break; /* mul word ptr es:[di + 0x11] */
        case 0x0BAF: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x0BB2u;break; /* mov word ptr [bp - 8], ax */
        case 0x0BB2: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0BB5u;break; /* mov ax, word ptr [bp - 8] */
        case 0x0BB5: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x0BB7u;break; /* shr ax, 1 */
        case 0x0BB7: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,1u));c->ip=0x0BB9u;break; /* shr ax, 1 */
        case 0x0BB9: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(c->ax));c->ip=0x0BBCu;break; /* mov word ptr [bp - 0x10], ax */
        case 0x0BBC: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEFu),(uint8_t)(0x04u));c->ip=0x0BC0u;break; /* mov byte ptr [bp - 0x11], 4 */
        case 0x0BC0: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0BC3u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0BC3: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0BC5u;break; /* shl ax, 1 */
        case 0x0BC5: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x0BC7u;break; /* shl ax, 1 */
        case 0x0BC7: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(c->ax));c->ip=0x0BCAu;break; /* mov word ptr [bp - 0xa], ax */
        case 0x0BCA: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEAu)),0x00u,0u,8u,1u);c->ip=0x0BCEu;break; /* cmp byte ptr [bp - 0x16], 0 */
        case 0x0BCE: if(!(c->flags&0x40u)){c->ip=0x0BD3u;}else c->ip=0x0BD0u;break; /* jne 0xbd3 */
        case 0x0BD0: c->ip=0x0CBFu;break; /* jmp 0xcbf */
        case 0x0BD3: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x0BD6u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x0BD6: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE8u),(uint16_t)(c->ax));c->ip=0x0BD9u;break; /* mov word ptr [bp - 0x18], ax */
        case 0x0BD9: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x0BDBu;break; /* xor ax, ax */
        case 0x0BDB: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0BDEu;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0BDE: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0BE3u;}else c->ip=0x0BE0u;break; /* jbe 0xbe3 */
        case 0x0BE0: c->ip=0x0CBCu;break; /* jmp 0xcbc */
        case 0x0BE3: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x0BE6u;break; /* mov word ptr [bp - 0xe], ax */
        case 0x0BE6: c->ip=0x0BEBu;break; /* jmp 0xbeb */
        case 0x0BE8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)),16u,0u)));c->ip=0x0BEBu;break; /* inc word ptr [bp - 0xe] */
        case 0x0BEB: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0BEEu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0BEE: wr8(b,c->ds,(uint16_t)(c->di+0x4A28u),(uint8_t)(0x01u));c->ip=0x0BF3u;break; /* mov byte ptr [di + 0x4a28], 1 */
        case 0x0BF3: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0BF6u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0BF6: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0BF9u;break; /* add word ptr [bp - 0xa], ax */
        case 0x0BF9: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0BFCu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0BFC: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0BFEu;break; /* shl di, 1 */
        case 0x0BFE: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x4C20u)));c->ip=0x0C02u;break; /* mov ax, word ptr [di + 0x4c20] */
        case 0x0C02: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),0u,16u,1u);c->ip=0x0C05u;break; /* cmp ax, word ptr [bp - 0xa] */
        case 0x0C05: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0C1Fu;}else c->ip=0x0C07u;break; /* jbe 0xc1f */
        case 0x0C07: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEBu)));c->ip=0x0C0Au;break; /* mov ax, word ptr [bp - 0x15] */
        case 0x0C0A: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0C0Du;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0C0D: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0C0Fu;break; /* shl di, 1 */
        case 0x0C0F: wr16(b,c->ds,(uint16_t)(c->di+0x4AD0u),(uint16_t)(c->ax));c->ip=0x0C13u;break; /* mov word ptr [di + 0x4ad0], ax */
        case 0x0C13: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0C16u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0C16: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0C19u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0C19: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0C1Bu;break; /* shl di, 1 */
        case 0x0C1B: wr16(b,c->ds,(uint16_t)(c->di+0x4C20u),(uint16_t)(c->ax));c->ip=0x0C1Fu;break; /* mov word ptr [di + 0x4c20], ax */
        case 0x0C1F: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFFAu))));c->ip=0x0C22u;break; /* mov al, byte ptr [bp - 6] */
        case 0x0C22: wr8(b,c->ss,(uint16_t)(c->bp+0xFFE7u),(uint8_t)((uint8_t)c->ax));c->ip=0x0C25u;break; /* mov byte ptr [bp - 0x19], al */
        case 0x0C25: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u));c->ip=0x0C27u;break; /* mov al, 1 */
        case 0x0C27: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,8u,1u);c->ip=0x0C2Au;break; /* cmp al, byte ptr [bp - 0x19] */
        case 0x0C2A: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x0C8Cu;}else c->ip=0x0C2Cu;break; /* jg 0xc8c */
        case 0x0C2C: wr8(b,c->ds,(uint16_t)(0x70C7u),(uint8_t)((uint8_t)c->ax));c->ip=0x0C2Fu;break; /* mov byte ptr [0x70c7], al */
        case 0x0C2F: c->ip=0x0C35u;break; /* jmp 0xc35 */
        case 0x0C31: wr8(b,c->ds,(uint16_t)(0x70C7u),(uint8_t)(incdec(c,rd8(b,c->ds,(uint16_t)(0x70C7u)),8u,0u)));c->ip=0x0C35u;break; /* inc byte ptr [0x70c7] */
        case 0x0C35: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0C38u;break; /* mov ax, word ptr [bp - 8] */
        case 0x0C38: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0C3Bu;break; /* les di, ptr [bp + 0xa] */
        case 0x0C3B: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0C3Du;break; /* add di, ax */
        case 0x0C3D: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0017u))));c->ip=0x0C41u;break; /* mov cl, byte ptr es:[di + 0x17] */
        case 0x0C41: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)));c->ip=0x0C44u;break; /* mov ax, word ptr [bp - 0x10] */
        case 0x0C44: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0C47u;break; /* les di, ptr [bp + 0xa] */
        case 0x0C47: {uint16_t o=(uint16_t)(c->di+0x0013u);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0C4Bu;break; /* les di, ptr es:[di + 0x13] */
        case 0x0C4B: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0C4Du;break; /* add di, ax */
        case 0x0C4D: c->dx=(uint16_t)((c->dx&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di))));c->ip=0x0C50u;break; /* mov dl, byte ptr es:[di] */
        case 0x0C50: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0C53u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0C53: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0C56u;break; /* les di, ptr [bp + 6] */
        case 0x0C56: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0C58u;break; /* add di, ax */
        case 0x0C58: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0017u))));c->ip=0x0C5Cu;break; /* mov al, byte ptr es:[di + 0x17] */
        case 0x0C5C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&(uint8_t)c->dx),8u)));c->ip=0x0C5Eu;break; /* and al, dl */
        case 0x0C5E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->cx),8u)));c->ip=0x0C60u;break; /* or al, cl */
        case 0x0C60: c->dx=(uint16_t)((c->dx&0xFF00u)|(uint8_t)((uint8_t)c->ax));c->ip=0x0C62u;break; /* mov dl, al */
        case 0x0C62: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0C65u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0C65: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0C68u;break; /* les di, ptr [bp + 6] */
        case 0x0C68: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0C6Au;break; /* add di, ax */
        case 0x0C6A: wr8(b,c->es,(uint16_t)(c->di+0x0017u),(uint8_t)((uint8_t)c->dx));c->ip=0x0C6Eu;break; /* mov byte ptr es:[di + 0x17], dl */
        case 0x0C6E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),16u,0u)));c->ip=0x0C71u;break; /* inc word ptr [bp - 8] */
        case 0x0C71: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEFu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEFu)),8u,1u)));c->ip=0x0C74u;break; /* dec byte ptr [bp - 0x11] */
        case 0x0C74: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEFu)),0x00u,0u,8u,1u);c->ip=0x0C78u;break; /* cmp byte ptr [bp - 0x11], 0 */
        case 0x0C78: if(!(c->flags&0x40u)){c->ip=0x0C81u;}else c->ip=0x0C7Au;break; /* jne 0xc81 */
        case 0x0C7A: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEFu),(uint8_t)(0x04u));c->ip=0x0C7Eu;break; /* mov byte ptr [bp - 0x11], 4 */
        case 0x0C7E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)),16u,0u)));c->ip=0x0C81u;break; /* inc word ptr [bp - 0x10] */
        case 0x0C81: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),16u,0u)));c->ip=0x0C84u;break; /* inc word ptr [bp - 0xa] */
        case 0x0C84: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x70C7u))));c->ip=0x0C87u;break; /* mov al, byte ptr [0x70c7] */
        case 0x0C87: (void)arith(c,(uint8_t)c->ax,rd8(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,8u,1u);c->ip=0x0C8Au;break; /* cmp al, byte ptr [bp - 0x19] */
        case 0x0C8A: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x0C8Au;return STATUS(LIMIT);}++loops;c->ip=0x0C31u;}else c->ip=0x0C8Cu;break; /* jne 0xc31 */
        case 0x0C8C: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0C8Fu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0C8F: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0C90u;break; /* dec ax */
        case 0x0C90: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0C93u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0C93: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0C95u;break; /* shl di, 1 */
        case 0x0C95: (void)arith(c,c->ax,rd16(b,c->ds,(uint16_t)(c->di+0x4D70u)),0u,16u,1u);c->ip=0x0C99u;break; /* cmp ax, word ptr [di + 0x4d70] */
        case 0x0C99: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0CA8u;}else c->ip=0x0C9Bu;break; /* jbe 0xca8 */
        case 0x0C9B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0C9Eu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0C9E: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0C9Fu;break; /* dec ax */
        case 0x0C9F: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0CA2u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0CA2: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0CA4u;break; /* shl di, 1 */
        case 0x0CA4: wr16(b,c->ds,(uint16_t)(c->di+0x4D70u),(uint16_t)(c->ax));c->ip=0x0CA8u;break; /* mov word ptr [di + 0x4d70], ax */
        case 0x0CA8: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0CABu;break; /* mov ax, word ptr [bp - 4] */
        case 0x0CAB: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0CAEu;break; /* add word ptr [bp - 0xa], ax */
        case 0x0CAE: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)),16u,0u)));c->ip=0x0CB1u;break; /* inc word ptr [bp - 0x13] */
        case 0x0CB1: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x0CB4u;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x0CB4: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0CB7u;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0CB7: if((c->flags&0x40u)){c->ip=0x0CBCu;}else c->ip=0x0CB9u;break; /* je 0xcbc */
        case 0x0CB9: if(loops==max_loopbacks){c->ip=0x0CB9u;return STATUS(LIMIT);}++loops;c->ip=0x0BE8u;break; /* jmp 0xbe8 */
        case 0x0CBC: c->ip=0x0D6Bu;break; /* jmp 0xd6b */
        case 0x0CBF: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x0CC2u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x0CC2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE8u),(uint16_t)(c->ax));c->ip=0x0CC5u;break; /* mov word ptr [bp - 0x18], ax */
        case 0x0CC5: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x0CC7u;break; /* xor ax, ax */
        case 0x0CC7: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0CCAu;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0CCA: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0CCFu;}else c->ip=0x0CCCu;break; /* jbe 0xccf */
        case 0x0CCC: c->ip=0x0D6Bu;break; /* jmp 0xd6b */
        case 0x0CCF: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x0CD2u;break; /* mov word ptr [bp - 0xe], ax */
        case 0x0CD2: c->ip=0x0CD7u;break; /* jmp 0xcd7 */
        case 0x0CD4: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)),16u,0u)));c->ip=0x0CD7u;break; /* inc word ptr [bp - 0xe] */
        case 0x0CD7: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0CDAu;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0CDA: wr8(b,c->ds,(uint16_t)(c->di+0x4A28u),(uint8_t)(0x01u));c->ip=0x0CDFu;break; /* mov byte ptr [di + 0x4a28], 1 */
        case 0x0CDF: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x0CE2u;break; /* mov ax, word ptr [bp - 2] */
        case 0x0CE2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0CE5u;break; /* add word ptr [bp - 0xa], ax */
        case 0x0CE5: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0CE8u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0CE8: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0CEAu;break; /* shl di, 1 */
        case 0x0CEA: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x4C20u)));c->ip=0x0CEEu;break; /* mov ax, word ptr [di + 0x4c20] */
        case 0x0CEE: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),0u,16u,1u);c->ip=0x0CF1u;break; /* cmp ax, word ptr [bp - 0xa] */
        case 0x0CF1: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0D0Bu;}else c->ip=0x0CF3u;break; /* jbe 0xd0b */
        case 0x0CF3: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEBu)));c->ip=0x0CF6u;break; /* mov ax, word ptr [bp - 0x15] */
        case 0x0CF6: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0CF9u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0CF9: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0CFBu;break; /* shl di, 1 */
        case 0x0CFB: wr16(b,c->ds,(uint16_t)(c->di+0x4AD0u),(uint16_t)(c->ax));c->ip=0x0CFFu;break; /* mov word ptr [di + 0x4ad0], ax */
        case 0x0CFF: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0D02u;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0D02: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0D05u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0D05: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0D07u;break; /* shl di, 1 */
        case 0x0D07: wr16(b,c->ds,(uint16_t)(c->di+0x4C20u),(uint16_t)(c->ax));c->ip=0x0D0Bu;break; /* mov word ptr [di + 0x4c20], ax */
        case 0x0D0B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x0D0Eu;break; /* mov ax, word ptr [bp - 8] */
        case 0x0D0E: {uint16_t o=(uint16_t)(c->bp+0x000Au);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0D11u;break; /* les di, ptr [bp + 0xa] */
        case 0x0D11: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0D13u;break; /* add di, ax */
        case 0x0D13: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0D17u;break; /* add di, 0x17 */
        case 0x0D17: push(b,c,c->es);c->ip=0x0D18u;break; /* push es */
        case 0x0D18: push(b,c,c->di);c->ip=0x0D19u;break; /* push di */
        case 0x0D19: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0D1Cu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0D1C: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0D1Fu;break; /* les di, ptr [bp + 6] */
        case 0x0D1F: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x0D21u;break; /* add di, ax */
        case 0x0D21: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x0D25u;break; /* add di, 0x17 */
        case 0x0D25: push(b,c,c->es);c->ip=0x0D26u;break; /* push es */
        case 0x0D26: push(b,c,c->di);c->ip=0x0D27u;break; /* push di */
        case 0x0D27: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0D2Au;break; /* push word ptr [bp - 6] */
        case 0x0D2A: push(b,c,c->cs);push(b,c,0x0D2Fu);c->cs=rd16(b,c->cs,0x0D2Du);c->ip=0x1685u;{int s=call_move(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x0D2Fu)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x1685 */
        case 0x0D2F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0D32u;break; /* mov ax, word ptr [bp - 6] */
        case 0x0D32: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)),c->ax,0u,16u,0u)));c->ip=0x0D35u;break; /* add word ptr [bp - 8], ax */
        case 0x0D35: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x0D38u;break; /* mov ax, word ptr [bp - 6] */
        case 0x0D38: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0D3Bu;break; /* add word ptr [bp - 0xa], ax */
        case 0x0D3B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0D3Eu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0D3E: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0D3Fu;break; /* dec ax */
        case 0x0D3F: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0D42u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0D42: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0D44u;break; /* shl di, 1 */
        case 0x0D44: (void)arith(c,c->ax,rd16(b,c->ds,(uint16_t)(c->di+0x4D70u)),0u,16u,1u);c->ip=0x0D48u;break; /* cmp ax, word ptr [di + 0x4d70] */
        case 0x0D48: if(((c->flags&1u)||(c->flags&0x40u))){c->ip=0x0D57u;}else c->ip=0x0D4Au;break; /* jbe 0xd57 */
        case 0x0D4A: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x0D4Du;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x0D4D: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x0D4Eu;break; /* dec ax */
        case 0x0D4E: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)));c->ip=0x0D51u;break; /* mov di, word ptr [bp - 0x13] */
        case 0x0D51: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x0D53u;break; /* shl di, 1 */
        case 0x0D53: wr16(b,c->ds,(uint16_t)(c->di+0x4D70u),(uint16_t)(c->ax));c->ip=0x0D57u;break; /* mov word ptr [di + 0x4d70], ax */
        case 0x0D57: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x0D5Au;break; /* mov ax, word ptr [bp - 4] */
        case 0x0D5A: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)),c->ax,0u,16u,0u)));c->ip=0x0D5Du;break; /* add word ptr [bp - 0xa], ax */
        case 0x0D5D: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFEDu)),16u,0u)));c->ip=0x0D60u;break; /* inc word ptr [bp - 0x13] */
        case 0x0D60: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x0D63u;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x0D63: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE8u)),0u,16u,1u);c->ip=0x0D66u;break; /* cmp ax, word ptr [bp - 0x18] */
        case 0x0D66: if((c->flags&0x40u)){c->ip=0x0D6Bu;}else c->ip=0x0D68u;break; /* je 0xd6b */
        case 0x0D68: if(loops==max_loopbacks){c->ip=0x0D68u;return STATUS(LIMIT);}++loops;c->ip=0x0CD4u;break; /* jmp 0xcd4 */
        case 0x0D6B: c->sp=(uint16_t)(c->bp);c->ip=0x0D6Du;break; /* mov sp, bp */
        case 0x0D6D: c->bp=(uint16_t)(pop(b,c));c->ip=0x0D6Eu;break; /* pop bp */
        case 0x0D6E: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+16u);return STATUS(OK);break; /* retf 0x10 */
        default: return STATUS(TRANSFER);
        }
    }
}
