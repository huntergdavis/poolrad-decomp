/* GAME.OVR unit 38h +0400 (009E slot 11), raw CPU on flat RAM (docs/re/OVL_0038_0400_CPU.md).
 * One case per original instruction; operands are reread from RAM in source order. */
#include "poolrad/recovered/overlay_u38_handler_0400_cpu.h"
#include <string.h>
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U38_0400_##s
typedef pr_u38_0400_status pr_status_t;
typedef pr_u38_0400_cpu pr_cpu_t;
typedef struct {
    size_t budget;
    int *callee_status;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_res_23db_cpu), "CPU layout");
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(uint8_t *b, size_t at) { return (uint16_t)(b[at] | ((uint16_t)b[(at + 1u) & MASK] << 8u)); }
static uint8_t rd8(uint8_t *b, uint16_t seg, uint16_t off) { return b[physical(seg, off)]; }
static uint16_t rd16(uint8_t *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static void wr8(uint8_t *b, uint16_t seg, uint16_t off, uint8_t v) { b[physical(seg, off)] = v; }
static void wr16(uint8_t *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    b[at] = (uint8_t)v; b[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(uint8_t *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
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
 * MSB(v<<(n-1)) ^ MSB(result) for SHL (pinned t_SHLw: MSB(result) ^ CF) and MSB(v>>(n-1)) ^ MSB(result) for SHR (0 above
 * count 1, as pinned t_SHRw); AF := 1 (pinned DOSBox-X; Unicorn clears it). The emitter's report shows neither is observed. */
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
/* 00BA:23DB, the party count, on the same RAM (its merged raw model). A non-RETURNED status stops here. */
static int call_party_count_23db(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    pr_res_23db_cpu in; size_t budget = r->budget; int s;
    memcpy(&in, c, sizeof in);
    s = (int)pr_res_00ba_party_count_23db_cpu(b, RAM_SIZE, &in, &budget);
    memcpy(c, &in, sizeof in);
    if (r->callee_status != NULL) *r->callee_status = s;
    return s == PR_RES_23DB_RETURNED ? 0 : STATUS(CALLEE);
}

pr_u38_0400_status pr_u38_handler_0400_cpu(uint8_t *b, size_t ram_size, pr_u38_0400_cpu *c, size_t party_budget,
                                           int *callee_status)
{
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE || c->ip != 0x0400u)
        return STATUS(ARGUMENT);
    run.budget = party_budget; run.callee_status = callee_status;
    if (callee_status != NULL) *callee_status = -1;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
        case 0x0400: push(b,c,c->bp);c->ip=0x0401u;break; /* push bp */
        case 0x0401: c->bp=(uint16_t)(c->sp);c->ip=0x0403u;break; /* mov bp, sp */
        case 0x0403: (void)arith(c,rd8(b,c->ss,(uint16_t)(c->bp+0x0006u)),0x00u,0u,8u,1u);c->ip=0x0407u;break; /* cmp byte ptr [bp + 6], 0 */
        case 0x0407: if((c->flags&0x40u)){c->ip=0x0437u;}else c->ip=0x0409u;break; /* je 0x437 */
        case 0x0409: {uint16_t o=(uint16_t)(c->bp+0x0008u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x040Cu;break; /* les di, ptr [bp + 8] */
        case 0x040C: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0003u))));c->ip=0x0410u;break; /* mov al, byte ptr es:[di + 3] */
        case 0x0410: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x40u),8u)));c->ip=0x0412u;break; /* and al, 0x40 */
        case 0x0412: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0414u;break; /* xor ah, ah */
        case 0x0414: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x06u));c->ip=0x0416u;break; /* mov cl, 6 */
        case 0x0416: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,1u));c->ip=0x0418u;break; /* shr ax, cl */
        case 0x0418: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x041Bu;break; /* les di, ptr [bp + 0xc] */
        case 0x041B: wr8(b,c->es,(uint16_t)(c->di+0x010Eu),(uint8_t)((uint8_t)c->ax));c->ip=0x0420u;break; /* mov byte ptr es:[di + 0x10e], al */
        case 0x0420: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0423u;break; /* les di, ptr [bp + 0xc] */
        case 0x0423: (void)arith(c,rd8(b,c->es,(uint16_t)(c->di+0x0084u)),0xB3u,0u,8u,1u);c->ip=0x0429u;break; /* cmp byte ptr es:[di + 0x84], 0xb3 */
        case 0x0429: if(!(c->flags&0x40u)){c->ip=0x0434u;}else c->ip=0x042Bu;break; /* jne 0x434 */
        case 0x042B: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x042Eu;break; /* les di, ptr [bp + 0xc] */
        case 0x042E: wr8(b,c->es,(uint16_t)(c->di+0x0084u),(uint8_t)(0x00u));c->ip=0x0434u;break; /* mov byte ptr es:[di + 0x84], 0 */
        case 0x0434: c->ip=0x04B6u;break; /* jmp 0x4b6 */
        case 0x0437: {uint16_t o=(uint16_t)(c->bp+0x0008u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x043Au;break; /* les di, ptr [bp + 8] */
        case 0x043A: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0003u))));c->ip=0x043Eu;break; /* mov al, byte ptr es:[di + 3] */
        case 0x043E: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x20u),8u)));c->ip=0x0440u;break; /* and al, 0x20 */
        case 0x0440: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x0442u;break; /* or al, al */
        case 0x0442: if(!(c->flags&0x40u)){c->ip=0x04B1u;}else c->ip=0x0444u;break; /* jne 0x4b1 */
        case 0x0444: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0447u;break; /* les di, ptr [bp + 0xc] */
        case 0x0447: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x010Eu))));c->ip=0x044Cu;break; /* mov al, byte ptr es:[di + 0x10e] */
        case 0x044C: c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&0x00FFu));c->ip=0x044Du;break; /* cwde  */
        case 0x044D: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x06u));c->ip=0x044Fu;break; /* mov cl, 6 */
        case 0x044F: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,0u));c->ip=0x0451u;break; /* shl ax, cl */
        case 0x0451: c->dx=(uint16_t)(c->ax);c->ip=0x0453u;break; /* mov dx, ax */
        case 0x0453: {uint16_t o=(uint16_t)(c->bp+0x0008u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0456u;break; /* les di, ptr [bp + 8] */
        case 0x0456: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0003u))));c->ip=0x045Au;break; /* mov al, byte ptr es:[di + 3] */
        case 0x045A: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x045Cu;break; /* xor ah, ah */
        case 0x045C: c->ax=(uint16_t)(arith(c,c->ax,0x0020u,0u,16u,0u));c->ip=0x045Fu;break; /* add ax, 0x20 */
        case 0x045F: c->ax=(uint16_t)(arith(c,c->ax,c->dx,0u,16u,0u));c->ip=0x0461u;break; /* add ax, dx */
        case 0x0461: {uint16_t o=(uint16_t)(c->bp+0x0008u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0464u;break; /* les di, ptr [bp + 8] */
        case 0x0464: wr8(b,c->es,(uint16_t)(c->di+0x0003u),(uint8_t)((uint8_t)c->ax));c->ip=0x0468u;break; /* mov byte ptr es:[di + 3], al */
        case 0x0468: {uint16_t o=(uint16_t)(c->bp+0x0008u);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x046Bu;break; /* les di, ptr [bp + 8] */
        case 0x046B: c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(rd8(b,c->es,(uint16_t)(c->di+0x0003u))));c->ip=0x046Fu;break; /* mov al, byte ptr es:[di + 3] */
        case 0x046F: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0471u;break; /* xor ah, ah */
        case 0x0471: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x07u));c->ip=0x0473u;break; /* mov cl, 7 */
        case 0x0473: c->ax=(uint16_t)(shift(c,c->ax,(uint8_t)((uint8_t)c->cx),16u,1u));c->ip=0x0475u;break; /* shr ax, cl */
        case 0x0475: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0478u;break; /* les di, ptr [bp + 0xc] */
        case 0x0478: wr8(b,c->es,(uint16_t)(c->di+0x010Eu),(uint8_t)((uint8_t)c->ax));c->ip=0x047Du;break; /* mov byte ptr es:[di + 0x10e], al */
        case 0x047D: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0480u;break; /* les di, ptr [bp + 0xc] */
        case 0x0480: wr8(b,c->es,(uint16_t)(c->di+0x010Fu),(uint8_t)(0x01u));c->ip=0x0486u;break; /* mov byte ptr es:[di + 0x10f], 1 */
        case 0x0486: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0489u;break; /* les di, ptr [bp + 0xc] */
        case 0x0489: (void)arith(c,rd8(b,c->es,(uint16_t)(c->di+0x0084u)),0x7Fu,0u,8u,1u);c->ip=0x048Fu;break; /* cmp byte ptr es:[di + 0x84], 0x7f */
        case 0x048F: if(!((c->flags&1u)||(c->flags&0x40u))){c->ip=0x049Au;}else c->ip=0x0491u;break; /* ja 0x49a */
        case 0x0491: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x0494u;break; /* les di, ptr [bp + 0xc] */
        case 0x0494: wr8(b,c->es,(uint16_t)(c->di+0x0084u),(uint8_t)(0xB3u));c->ip=0x049Au;break; /* mov byte ptr es:[di + 0x84], 0xb3 */
        case 0x049A: {uint16_t o=(uint16_t)(c->bp+0x000Cu);size_t at=physical(c->ss,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x049Du;break; /* les di, ptr [bp + 0xc] */
        case 0x049D: {uint16_t o=(uint16_t)(c->di+0x0108u);size_t at=physical(c->es,o);c->di=word_at(b,at);c->es=word_at(b,(at+2u)&MASK);}c->ip=0x04A2u;break; /* les di, ptr es:[di + 0x108] */
        case 0x04A2: c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u));c->ip=0x04A4u;break; /* xor ax, ax */
        case 0x04A4: wr16(b,c->es,(uint16_t)(c->di+0x000Au),(uint16_t)(c->ax));c->ip=0x04A8u;break; /* mov word ptr es:[di + 0xa], ax */
        case 0x04A8: wr16(b,c->es,(uint16_t)(c->di+0x000Cu),(uint16_t)(c->ax));c->ip=0x04ACu;break; /* mov word ptr es:[di + 0xc], ax */
        case 0x04AC: push(b,c,c->cs);push(b,c,0x04B1u);c->cs=rd16(b,c->cs,0x04AFu);c->ip=0x23DBu;{int s=call_party_count_23db(b,c,dg);if(s)return (pr_status_t)s;}if(c->cs!=caller||c->ip!=0x04B1u)return STATUS(TRANSFER);break; /* lcall 0xba, 0x23db */
        case 0x04B1: wr8(b,c->ds,(uint16_t)(0x6825u),(uint8_t)(0x64u));c->ip=0x04B6u;break; /* mov byte ptr [0x6825], 0x64 */
        case 0x04B6: c->bp=(uint16_t)(pop(b,c));c->ip=0x04B7u;break; /* pop bp */
        case 0x04B7: c->ip=pop(b,c);c->cs=pop(b,c);c->sp=(uint16_t)(c->sp+10u);return STATUS(OK);break; /* retf 0xa */
        default: return STATUS(TRANSFER);
        }
    }
}
