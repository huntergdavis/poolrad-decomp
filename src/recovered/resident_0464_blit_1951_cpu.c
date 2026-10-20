/* START.EXE resident 0464:1951 bitmap blit, raw CPU (docs/re/RES_0464_BLIT_1951_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_0464_blit_1951_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R0464_BLIT_CPU_##s
typedef pr_r0464_blit_cpu_status pr_status_t;
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
static void push(pr_bus *b, pr_r0464_blit_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, pr_r0464_blit_cpu *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void szp(pr_r0464_blit_cpu *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t arith(pr_r0464_blit_cpu *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
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
static uint16_t logic(pr_r0464_blit_cpu *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
static uint16_t incdec(pr_r0464_blit_cpu *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t cf = c->flags & 1u; v = arith(c, v, 1u, 0u, bits, dec);
    c->flags = (uint16_t)((c->flags & ~1u) | cf); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1,
 * Unicorn's rule otherwise); AF := 0 (Unicorn). The report in the doc shows where these are observed. */
static uint16_t shift(pr_r0464_blit_cpu *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
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
static int call_move(pr_bus *b, pr_r0464_blit_cpu *c, pr_r0464_blit_cpu_diagnostic *diag)
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

pr_r0464_blit_cpu_status pr_r0464_blit_1951_cpu(pr_bus *b, size_t ram_size, pr_r0464_blit_cpu *c,
                                                 size_t max_loopbacks, pr_r0464_blit_cpu_diagnostic *d)
{
    pr_r0464_blit_cpu_diagnostic fallback = {0, -1};
    uint16_t caller; size_t loops = 0;
    if (b == NULL || b->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x1951u)
        return STATUS(ARGUMENT);
    pr_r0464_blit_cpu_diagnostic *dg = d != NULL ? d : &fallback;
    *dg = fallback;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x1951: push(b,c,c->bp);c->ip=0x1952u;break; /* push bp */
        case 0x1952: c->bp=(uint16_t)(c->sp);c->ip=0x1954u;break; /* mov bp, sp */
        case 0x1954: c->sp=(uint16_t)(arith(c,c->sp,0x001Au,0u,16u,1u));c->ip=0x1957u;break; /* sub sp, 0x1a */
        case 0x1957: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x0006u)));c->ip=0x195Au;break; /* mov ax, word ptr [bp + 6] */
        case 0x195A: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|rd16(b,c->ss,(uint16_t)(c->bp+0x0008u))),16u));c->ip=0x195Du;break; /* or ax, word ptr [bp + 8] */
        case 0x195D: if(!(c->flags&0x40u)){c->ip=0x1962u;}else c->ip=0x195Fu;break; /* jne 0x1962 */
        case 0x195F: c->ip=0x1B4Cu;break; /* jmp 0x1b4c */
        case 0x1962: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1965u;break; /* les di, ptr [bp + 6] */
        case 0x1965: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEBu),(uint16_t)(c->di));c->ip=0x1968u;break; /* mov word ptr [bp - 0x15], di */
        case 0x1968: wr16(b,c->ss,(uint16_t)(c->bp+0xFFEDu),(uint16_t)(c->es));c->ip=0x196Bu;break; /* mov word ptr [bp - 0x13], es */
        case 0x196B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(0x4A1Eu))));c->ip=0x196Eu;break; /* mov al, byte ptr [0x4a1e] */
        case 0x196E: (void)arith(c,(uint8_t)c->ax,0x00u,0u,8u,1u);c->ip=0x1970u;break; /* cmp al, 0 */
        case 0x1970: if((c->flags&0x40u)){c->ip=0x1975u;}else c->ip=0x1972u;break; /* je 0x1975 */
        case 0x1972: c->ip=0x19F2u;break; /* jmp 0x19f2 */
        case 0x1975: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1978u;break; /* les di, ptr [bp - 0x15] */
        case 0x1978: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x197Cu;break; /* mov ax, word ptr es:[di + 2] */
        case 0x197C: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x197Eu;break; /* shl ax, 1 */
        case 0x197E: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x1981u;break; /* mov word ptr [bp - 0xe], ax */
        case 0x1981: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x1983u;break; /* xor ax, ax */
        case 0x1983: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(c->ax));c->ip=0x1986u;break; /* mov word ptr [bp - 0x10], ax */
        case 0x1986: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)));c->ip=0x1989u;break; /* mov ax, word ptr [bp + 0xa] */
        case 0x1989: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x03u));c->ip=0x198Bu;break; /* mov cl, 3 */
        case 0x198B: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,0u));c->ip=0x198Du;break; /* shl ax, cl */
        case 0x198D: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x1990u;break; /* mov word ptr [bp - 8], ax */
        case 0x1990: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x1993u;break; /* mov ax, word ptr [bp - 8] */
        case 0x1993: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1996u;break; /* les di, ptr [bp - 0x15] */
        case 0x1996: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->es,(uint16_t)(c->di)),0u,16u,0u));c->ip=0x1999u;break; /* add ax, word ptr es:[di] */
        case 0x1999: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x199Au;break; /* dec ax */
        case 0x199A: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF4u),(uint16_t)(c->ax));c->ip=0x199Du;break; /* mov word ptr [bp - 0xc], ax */
        case 0x199D: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Cu)));c->ip=0x19A0u;break; /* mov ax, word ptr [bp + 0xc] */
        case 0x19A0: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x19A2u;break; /* shl ax, 1 */
        case 0x19A2: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x19A5u;break; /* mov word ptr [bp - 6], ax */
        case 0x19A5: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x19A8u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x19A8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE9u),(uint16_t)(c->ax));c->ip=0x19ABu;break; /* mov word ptr [bp - 0x17], ax */
        case 0x19AB: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x19AEu;break; /* mov ax, word ptr [bp - 8] */
        case 0x19AE: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x19B1u;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x19B1: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x19EFu;}else c->ip=0x19B3u;break; /* jg 0x19ef */
        case 0x19B3: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x19B6u;break; /* mov word ptr [bp - 4], ax */
        case 0x19B6: c->ip=0x19BBu;break; /* jmp 0x19bb */
        case 0x19B8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),16u,0u)));c->ip=0x19BBu;break; /* inc word ptr [bp - 4] */
        case 0x19BB: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)));c->ip=0x19BEu;break; /* mov ax, word ptr [bp - 0x10] */
        case 0x19BE: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x19C1u;break; /* les di, ptr [bp + 6] */
        case 0x19C1: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x19C3u;break; /* add di, ax */
        case 0x19C3: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x19C7u;break; /* add di, 0x17 */
        case 0x19C7: push(b,c,c->es);c->ip=0x19C8u;break; /* push es */
        case 0x19C8: push(b,c,c->di);c->ip=0x19C9u;break; /* push di */
        case 0x19C9: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x19CCu;break; /* mov di, word ptr [bp - 4] */
        case 0x19CC: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x19CEu;break; /* shl di, 1 */
        case 0x19CE: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x4EC0u)));c->ip=0x19D2u;break; /* mov ax, word ptr [di + 0x4ec0] */
        case 0x19D2: push(b,c,c->ax);c->ip=0x19D3u;break; /* push ax */
        case 0x19D3: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x19D6u;break; /* mov di, word ptr [bp - 6] */
        case 0x19D6: c->es=(uint16_t)(pop(b,c));c->ip=0x19D7u;break; /* pop es */
        case 0x19D7: push(b,c,c->es);c->ip=0x19D8u;break; /* push es */
        case 0x19D8: push(b,c,c->di);c->ip=0x19D9u;break; /* push di */
        case 0x19D9: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x19DCu;break; /* push word ptr [bp - 0xe] */
        case 0x19DC: push(b,c,c->cs);push(b,c,0x19E1u);c->cs=rd16(b,c->cs,0x19DFu);c->ip=0x1685u;{int s=call_move(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x19E1u)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x1685 */
        case 0x19E1: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x19E4u;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x19E4: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)),c->ax,0u,16u,0u)));c->ip=0x19E7u;break; /* add word ptr [bp - 0x10], ax */
        case 0x19E7: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x19EAu;break; /* mov ax, word ptr [bp - 4] */
        case 0x19EA: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x19EDu;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x19ED: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x19EDu;return STATUS(LIMIT);}++loops;c->ip=0x19B8u;}else c->ip=0x19EFu;break; /* jne 0x19b8 */
        case 0x19EF: c->ip=0x1B4Cu;break; /* jmp 0x1b4c */
        case 0x19F2: (void)arith(c,(uint8_t)c->ax,0x01u,0u,8u,1u);c->ip=0x19F4u;break; /* cmp al, 1 */
        case 0x19F4: if((c->flags&0x40u)){c->ip=0x19F9u;}else c->ip=0x19F6u;break; /* je 0x19f9 */
        case 0x19F6: c->ip=0x1AC7u;break; /* jmp 0x1ac7 */
        case 0x19F9: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x19FBu;break; /* xor ax, ax */
        case 0x19FB: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(c->ax));c->ip=0x19FEu;break; /* mov word ptr [bp - 0x10], ax */
        case 0x19FE: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)));c->ip=0x1A01u;break; /* mov ax, word ptr [bp + 0xa] */
        case 0x1A01: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x03u));c->ip=0x1A03u;break; /* mov cl, 3 */
        case 0x1A03: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,0u));c->ip=0x1A05u;break; /* shl ax, cl */
        case 0x1A05: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x1A08u;break; /* mov word ptr [bp - 8], ax */
        case 0x1A08: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x1A0Bu;break; /* mov ax, word ptr [bp - 8] */
        case 0x1A0B: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1A0Eu;break; /* les di, ptr [bp - 0x15] */
        case 0x1A0E: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->es,(uint16_t)(c->di)),0u,16u,0u));c->ip=0x1A11u;break; /* add ax, word ptr es:[di] */
        case 0x1A11: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x1A12u;break; /* dec ax */
        case 0x1A12: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF4u),(uint16_t)(c->ax));c->ip=0x1A15u;break; /* mov word ptr [bp - 0xc], ax */
        case 0x1A15: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Cu)));c->ip=0x1A18u;break; /* mov ax, word ptr [bp + 0xc] */
        case 0x1A18: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x1A1Bu;break; /* mov word ptr [bp - 6], ax */
        case 0x1A1B: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x1A1Eu;break; /* mov ax, word ptr [bp - 6] */
        case 0x1A1E: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1A21u;break; /* les di, ptr [bp - 0x15] */
        case 0x1A21: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->es,(uint16_t)(c->di+0x0002u)),0u,16u,0u));c->ip=0x1A25u;break; /* add ax, word ptr es:[di + 2] */
        case 0x1A25: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x1A26u;break; /* dec ax */
        case 0x1A26: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF6u),(uint16_t)(c->ax));c->ip=0x1A29u;break; /* mov word ptr [bp - 0xa], ax */
        case 0x1A29: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x02u));c->ip=0x1A2Bu;break; /* mov al, 2 */
        case 0x1A2B: c->dx=(uint16_t)(0x03C4u);c->ip=0x1A2Eu;break; /* mov dx, 0x3c4 */
        case 0x1A2E: if(pr_bus_out(b,c->dx,(uint8_t)c->ax,1u,c->cs,0x1A2Eu)){c->ip=0x1A2Eu;return STATUS(PORT);}c->ip=0x1A2Fu;break; /* out dx, al */
        case 0x1A2F: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x1A32u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x1A32: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE9u),(uint16_t)(c->ax));c->ip=0x1A35u;break; /* mov word ptr [bp - 0x17], ax */
        case 0x1A35: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x1A38u;break; /* mov ax, word ptr [bp - 8] */
        case 0x1A38: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x1A3Bu;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x1A3B: if(((c->flags&0x40u)||((c->flags>>7)&1u)!=((c->flags>>11)&1u))){c->ip=0x1A40u;}else c->ip=0x1A3Du;break; /* jle 0x1a40 */
        case 0x1A3D: c->ip=0x1ABEu;break; /* jmp 0x1abe */
        case 0x1A40: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x1A43u;break; /* mov word ptr [bp - 4], ax */
        case 0x1A43: c->ip=0x1A48u;break; /* jmp 0x1a48 */
        case 0x1A45: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),16u,0u)));c->ip=0x1A48u;break; /* inc word ptr [bp - 4] */
        case 0x1A48: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF6u)));c->ip=0x1A4Bu;break; /* mov ax, word ptr [bp - 0xa] */
        case 0x1A4B: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE7u),(uint16_t)(c->ax));c->ip=0x1A4Eu;break; /* mov word ptr [bp - 0x19], ax */
        case 0x1A4E: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x1A51u;break; /* mov ax, word ptr [bp - 6] */
        case 0x1A51: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,16u,1u);c->ip=0x1A54u;break; /* cmp ax, word ptr [bp - 0x19] */
        case 0x1A54: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x1AB6u;}else c->ip=0x1A56u;break; /* jg 0x1ab6 */
        case 0x1A56: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(c->ax));c->ip=0x1A59u;break; /* mov word ptr [bp - 2], ax */
        case 0x1A59: c->ip=0x1A5Eu;break; /* jmp 0x1a5e */
        case 0x1A5B: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFEu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)),16u,0u)));c->ip=0x1A5Eu;break; /* inc word ptr [bp - 2] */
        case 0x1A5E: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEFu),(uint8_t)(0x00u));c->ip=0x1A62u;break; /* mov byte ptr [bp - 0x11], 0 */
        case 0x1A62: c->ip=0x1A67u;break; /* jmp 0x1a67 */
        case 0x1A64: wr8(b,c->ss,(uint16_t)(c->bp+0xFFEFu),(uint8_t)(incdec(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEFu)),8u,0u)));c->ip=0x1A67u;break; /* inc byte ptr [bp - 0x11] */
        case 0x1A67: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ss,(uint16_t)(c->bp+0xFFEFu))));c->ip=0x1A6Au;break; /* mov al, byte ptr [bp - 0x11] */
        case 0x1A6A: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x1A6Cu;break; /* xor ah, ah */
        case 0x1A6C: c->di=(uint16_t)(c->ax);c->ip=0x1A6Eu;break; /* mov di, ax */
        case 0x1A6E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->ds,(uint16_t)(c->di+0x25F6u))));c->ip=0x1A72u;break; /* mov al, byte ptr [di + 0x25f6] */
        case 0x1A72: c->dx=(uint16_t)(0x03C5u);c->ip=0x1A75u;break; /* mov dx, 0x3c5 */
        case 0x1A75: if(pr_bus_out(b,c->dx,(uint8_t)c->ax,1u,c->cs,0x1A75u)){c->ip=0x1A75u;return STATUS(PORT);}c->ip=0x1A76u;break; /* out dx, al */
        case 0x1A76: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)));c->ip=0x1A79u;break; /* mov ax, word ptr [bp - 0x10] */
        case 0x1A79: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1A7Cu;break; /* les di, ptr [bp + 6] */
        case 0x1A7C: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x1A7Eu;break; /* add di, ax */
        case 0x1A7E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0017u))));c->ip=0x1A82u;break; /* mov al, byte ptr es:[di + 0x17] */
        case 0x1A82: push(b,c,c->ax);c->ip=0x1A83u;break; /* push ax */
        case 0x1A83: c->ax=(uint16_t)(0xA000u);c->ip=0x1A86u;break; /* mov ax, 0xa000 */
        case 0x1A86: push(b,c,c->ax);c->ip=0x1A87u;break; /* push ax */
        case 0x1A87: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x1A8Au;break; /* mov ax, word ptr [bp - 2] */
        case 0x1A8A: c->dx=(c->ax&0x8000u)?0xFFFFu:0u;c->ip=0x1A8Bu;break; /* cdq  */
        case 0x1A8B: c->cx=(uint16_t)(c->ax);c->ip=0x1A8Du;break; /* mov cx, ax */
        case 0x1A8D: c->bx=(uint16_t)(c->dx);c->ip=0x1A8Fu;break; /* mov bx, dx */
        case 0x1A8F: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x1A92u;break; /* mov di, word ptr [bp - 4] */
        case 0x1A92: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x1A94u;break; /* shl di, 1 */
        case 0x1A94: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x5050u)));c->ip=0x1A98u;break; /* mov ax, word ptr [di + 0x5050] */
        case 0x1A98: c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u));c->ip=0x1A9Au;break; /* xor dx, dx */
        case 0x1A9A: c->ax=(uint16_t)(arith(c,c->ax,c->cx,0u,16u,0u));c->ip=0x1A9Cu;break; /* add ax, cx */
        case 0x1A9C: c->dx=(uint16_t)(arith(c,c->dx,c->bx,(c->flags&1u),16u,0u));c->ip=0x1A9Eu;break; /* adc dx, bx */
        case 0x1A9E: c->di=(uint16_t)(c->ax);c->ip=0x1AA0u;break; /* mov di, ax */
        case 0x1AA0: c->es=(uint16_t)(pop(b,c));c->ip=0x1AA1u;break; /* pop es */
        case 0x1AA1: c->ax=(uint16_t)(pop(b,c));c->ip=0x1AA2u;break; /* pop ax */
        case 0x1AA2: wr8(b,c->es,(uint16_t)(c->di),(uint8_t)((uint8_t)c->ax));c->ip=0x1AA5u;break; /* mov byte ptr es:[di], al */
        case 0x1AA5: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)),16u,0u)));c->ip=0x1AA8u;break; /* inc word ptr [bp - 0x10] */
        case 0x1AA8: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0xFFEFu)),0x03u,0u,8u,1u);c->ip=0x1AACu;break; /* cmp byte ptr [bp - 0x11], 3 */
        case 0x1AAC: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x1AACu;return STATUS(LIMIT);}++loops;c->ip=0x1A64u;}else c->ip=0x1AAEu;break; /* jne 0x1a64 */
        case 0x1AAE: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFEu)));c->ip=0x1AB1u;break; /* mov ax, word ptr [bp - 2] */
        case 0x1AB1: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE7u)),0u,16u,1u);c->ip=0x1AB4u;break; /* cmp ax, word ptr [bp - 0x19] */
        case 0x1AB4: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x1AB4u;return STATUS(LIMIT);}++loops;c->ip=0x1A5Bu;}else c->ip=0x1AB6u;break; /* jne 0x1a5b */
        case 0x1AB6: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x1AB9u;break; /* mov ax, word ptr [bp - 4] */
        case 0x1AB9: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x1ABCu;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x1ABC: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x1ABCu;return STATUS(LIMIT);}++loops;c->ip=0x1A45u;}else c->ip=0x1ABEu;break; /* jne 0x1a45 */
        case 0x1ABE: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x0Fu));c->ip=0x1AC0u;break; /* mov al, 0xf */
        case 0x1AC0: c->dx=(uint16_t)(0x03C5u);c->ip=0x1AC3u;break; /* mov dx, 0x3c5 */
        case 0x1AC3: if(pr_bus_out(b,c->dx,(uint8_t)c->ax,1u,c->cs,0x1AC3u)){c->ip=0x1AC3u;return STATUS(PORT);}c->ip=0x1AC4u;break; /* out dx, al */
        case 0x1AC4: c->ip=0x1B4Cu;break; /* jmp 0x1b4c */
        case 0x1AC7: (void)arith(c,(uint8_t)c->ax,0x02u,0u,8u,1u);c->ip=0x1AC9u;break; /* cmp al, 2 */
        case 0x1AC9: if((c->flags&0x40u)){c->ip=0x1ACEu;}else c->ip=0x1ACBu;break; /* je 0x1ace */
        case 0x1ACB: c->ip=0x1B4Cu;break; /* jmp 0x1b4c */
        case 0x1ACE: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1AD1u;break; /* les di, ptr [bp - 0x15] */
        case 0x1AD1: c->ax=(uint16_t)(rd16(b,c->es,(uint16_t)(c->di+0x0002u)));c->ip=0x1AD5u;break; /* mov ax, word ptr es:[di + 2] */
        case 0x1AD5: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x1AD7u;break; /* shl ax, 1 */
        case 0x1AD7: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x1AD9u;break; /* shl ax, 1 */
        case 0x1AD9: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF2u),(uint16_t)(c->ax));c->ip=0x1ADCu;break; /* mov word ptr [bp - 0xe], ax */
        case 0x1ADC: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x1ADEu;break; /* xor ax, ax */
        case 0x1ADE: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(c->ax));c->ip=0x1AE1u;break; /* mov word ptr [bp - 0x10], ax */
        case 0x1AE1: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Au)));c->ip=0x1AE4u;break; /* mov ax, word ptr [bp + 0xa] */
        case 0x1AE4: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x03u));c->ip=0x1AE6u;break; /* mov cl, 3 */
        case 0x1AE6: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,0u));c->ip=0x1AE8u;break; /* shl ax, cl */
        case 0x1AE8: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF8u),(uint16_t)(c->ax));c->ip=0x1AEBu;break; /* mov word ptr [bp - 8], ax */
        case 0x1AEB: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x1AEEu;break; /* mov ax, word ptr [bp - 8] */
        case 0x1AEE: {uint16_t o=(uint16_t)(c->bp+0xFFEBu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1AF1u;break; /* les di, ptr [bp - 0x15] */
        case 0x1AF1: c->ax=(uint16_t)(arith(c,c->ax,rd16(b,c->es,(uint16_t)(c->di)),0u,16u,0u));c->ip=0x1AF4u;break; /* add ax, word ptr es:[di] */
        case 0x1AF4: c->ax=(uint16_t)(incdec(c,c->ax,16u,1u));c->ip=0x1AF5u;break; /* dec ax */
        case 0x1AF5: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF4u),(uint16_t)(c->ax));c->ip=0x1AF8u;break; /* mov word ptr [bp - 0xc], ax */
        case 0x1AF8: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0x000Cu)));c->ip=0x1AFBu;break; /* mov ax, word ptr [bp + 0xc] */
        case 0x1AFB: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x1AFDu;break; /* shl ax, 1 */
        case 0x1AFD: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)(0x01u),16u,0u));c->ip=0x1AFFu;break; /* shl ax, 1 */
        case 0x1AFF: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFAu),(uint16_t)(c->ax));c->ip=0x1B02u;break; /* mov word ptr [bp - 6], ax */
        case 0x1B02: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF4u)));c->ip=0x1B05u;break; /* mov ax, word ptr [bp - 0xc] */
        case 0x1B05: wr16(b,c->ss,(uint16_t)(c->bp+0xFFE9u),(uint16_t)(c->ax));c->ip=0x1B08u;break; /* mov word ptr [bp - 0x17], ax */
        case 0x1B08: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF8u)));c->ip=0x1B0Bu;break; /* mov ax, word ptr [bp - 8] */
        case 0x1B0B: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x1B0Eu;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x1B0E: if((!(c->flags&0x40u)&&((c->flags>>7)&1u)==((c->flags>>11)&1u))){c->ip=0x1B4Cu;}else c->ip=0x1B10u;break; /* jg 0x1b4c */
        case 0x1B10: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(c->ax));c->ip=0x1B13u;break; /* mov word ptr [bp - 4], ax */
        case 0x1B13: c->ip=0x1B18u;break; /* jmp 0x1b18 */
        case 0x1B15: wr16(b,c->ss,(uint16_t)(c->bp+0xFFFCu),(uint16_t)(incdec(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)),16u,0u)));c->ip=0x1B18u;break; /* inc word ptr [bp - 4] */
        case 0x1B18: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)));c->ip=0x1B1Bu;break; /* mov ax, word ptr [bp - 0x10] */
        case 0x1B1B: {uint16_t o=(uint16_t)(c->bp+0x0006u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x1B1Eu;break; /* les di, ptr [bp + 6] */
        case 0x1B1E: c->di=(uint16_t)(arith(c,c->di,c->ax,0u,16u,0u));c->ip=0x1B20u;break; /* add di, ax */
        case 0x1B20: c->di=(uint16_t)(arith(c,c->di,0x0017u,0u,16u,0u));c->ip=0x1B24u;break; /* add di, 0x17 */
        case 0x1B24: push(b,c,c->es);c->ip=0x1B25u;break; /* push es */
        case 0x1B25: push(b,c,c->di);c->ip=0x1B26u;break; /* push di */
        case 0x1B26: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x1B29u;break; /* mov di, word ptr [bp - 4] */
        case 0x1B29: c->di=(uint16_t)(shift(c,c->di,(uint8_t)(0x01u),16u,0u));c->ip=0x1B2Bu;break; /* shl di, 1 */
        case 0x1B2B: c->ax=(uint16_t)(rd16(b,c->ds,(uint16_t)(c->di+0x51E0u)));c->ip=0x1B2Fu;break; /* mov ax, word ptr [di + 0x51e0] */
        case 0x1B2F: push(b,c,c->ax);c->ip=0x1B30u;break; /* push ax */
        case 0x1B30: c->di=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFAu)));c->ip=0x1B33u;break; /* mov di, word ptr [bp - 6] */
        case 0x1B33: c->es=(uint16_t)(pop(b,c));c->ip=0x1B34u;break; /* pop es */
        case 0x1B34: push(b,c,c->es);c->ip=0x1B35u;break; /* push es */
        case 0x1B35: push(b,c,c->di);c->ip=0x1B36u;break; /* push di */
        case 0x1B36: push(b,c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x1B39u;break; /* push word ptr [bp - 0xe] */
        case 0x1B39: push(b,c,c->cs);push(b,c,0x1B3Eu);c->cs=rd16(b,c->cs,0x1B3Cu);c->ip=0x1685u;{int s=call_move(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x1B3Eu)return STATUS(TRANSFER);break; /* lcall 0xaf8, 0x1685 */
        case 0x1B3E: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFF2u)));c->ip=0x1B41u;break; /* mov ax, word ptr [bp - 0xe] */
        case 0x1B41: wr16(b,c->ss,(uint16_t)(c->bp+0xFFF0u),(uint16_t)(arith(c,rd16(b,c->ss,(uint16_t)(c->bp+0xFFF0u)),c->ax,0u,16u,0u)));c->ip=0x1B44u;break; /* add word ptr [bp - 0x10], ax */
        case 0x1B44: c->ax=(uint16_t)(rd16(b,c->ss,(uint16_t)(c->bp+0xFFFCu)));c->ip=0x1B47u;break; /* mov ax, word ptr [bp - 4] */
        case 0x1B47: (void)arith(c,c->ax,rd16(b,c->ss,(uint16_t)(c->bp+0xFFE9u)),0u,16u,1u);c->ip=0x1B4Au;break; /* cmp ax, word ptr [bp - 0x17] */
        case 0x1B4A: if(!(c->flags&0x40u)){if(loops==max_loopbacks){c->ip=0x1B4Au;return STATUS(LIMIT);}++loops;c->ip=0x1B15u;}else c->ip=0x1B4Cu;break; /* jne 0x1b15 */
        case 0x1B4C: c->sp=(uint16_t)(c->bp);c->ip=0x1B4Eu;break; /* mov sp, bp */
        case 0x1B4E: c->bp=(uint16_t)(pop(b,c));c->ip=0x1B4Fu;break; /* pop bp */
        case 0x1B4F: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+8u);return STATUS(OK);break; /* retf 8 */
        default: return STATUS(TRANSFER);
        }
    }
}
