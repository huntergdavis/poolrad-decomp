/* Raw resident 00BA character recalculation and its four local callees.
 * Literal control flow preserves stack/data aliases and instruction budgets.
 * Arithmetic conventions follow merged U86 +07D6 and U3C6 +03A0 models.
 * Immutable code; no original instruction bytes or runtime decoding. */
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"
#define MASK 0xfffffu
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4u)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) {
    return (uint16_t)(r[p]|((uint16_t)r[(p+1u)&MASK]<<8u));
}
static uint16_t read16(const uint8_t *r,uint16_t s,uint16_t o) {
    return word_at(r,physical(s,o));
}
static void write16(uint8_t *r,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);r[p]=(uint8_t)v;r[(p+1u)&MASK]=(uint8_t)(v>>8u);
}
static void push(uint8_t *r,pr_res00ba_cpu *c,uint16_t v) {
    c->sp=(uint16_t)(c->sp-2u);write16(r,c->ss,c->sp,v);
}
static uint16_t pop(const uint8_t *r,pr_res00ba_cpu *c) {
    uint16_t v=read16(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);return v;
}
static unsigned parity(uint8_t v) {
    v^=(uint8_t)(v>>4u);v^=(uint8_t)(v>>2u);v^=(uint8_t)(v>>1u);
    return (v&1u)==0u;
}
static void szp(pr_res00ba_cpu *c,uint16_t v,unsigned bits) {
    if(parity((uint8_t)v))c->flags|=4u;
    if(v==0u)c->flags|=0x40u;
    if(v&(bits==8u?0x80u:0x8000u))c->flags|=0x80u;
}
static uint16_t arithmetic(pr_res00ba_cpu *c,uint16_t a,uint16_t b,
                            unsigned carry,unsigned bits,unsigned subtract) {
    uint32_t mask=bits==8u?255u:65535u;
    uint16_t sign=bits==8u?0x80u:0x8000u;
    uint32_t wide=subtract?(uint32_t)a-b-carry:(uint32_t)a+b+carry;
    uint16_t v=(uint16_t)(wide&mask);
    c->flags&=(uint16_t)~0x08d5u;
    if(subtract?(uint32_t)a<(uint32_t)b+carry:wide>mask)c->flags|=1u;
    if((a^b^v)&0x10u)c->flags|=0x10u;
    if((subtract?(a^b)&(a^v):~(a^b)&(a^v))&sign)c->flags|=0x800u;
    szp(c,v,bits);return v;
}
static uint16_t logic(pr_res00ba_cpu *c,uint16_t v,unsigned bits) {
    c->flags&=(uint16_t)~0x08d5u;szp(c,v,bits);return v;
}
static uint16_t step(pr_res00ba_cpu *c,uint16_t v,unsigned bits,unsigned sub) {
    uint16_t carry=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,sub);c->flags=(uint16_t)((c->flags&~1u)|carry);
    return v;
}
static uint16_t shl(pr_res00ba_cpu *c,uint16_t v,unsigned count) {
    uint16_t result;
    count&=31u;if(!count)return v;
    result=(uint16_t)((uint32_t)v<<count);
    c->flags&=(uint16_t)~0x08d5u;c->flags|=0x10u;
    if(count<=16u && ((uint32_t)v>>(16u-count))&1u)c->flags|=1u;
    if((v^result)&0x8000u)c->flags|=0x800u;
    szp(c,result,16u);return result;
}
static void multiply(pr_res00ba_cpu *c,uint16_t rhs) {
    uint32_t product=(uint32_t)c->ax*rhs;
    uint16_t flags=(uint16_t)(c->flags&0x0090u);
    c->ax=(uint16_t)product;c->dx=(uint16_t)(product>>16u);
    if(parity((uint8_t)(product^(product>>8u)^(product>>16u)^(product>>24u))))flags|=4u;
    if(!c->ax)flags|=0x40u;
    if(c->dx)flags|=0x0801u;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|flags);
}
static pr_res00ba_status core(uint8_t *ram,size_t size,pr_res00ba_cpu *c,size_t *budget)
{
    uint16_t segment=c->cs;
    for (;;) {
        if(c->cs!=segment)return PR_RES00BA_TRANSFER;
        if(!*budget)return PR_RES00BA_LIMIT;
        --*budget;
        switch(c->ip) {
        case 0x0000u: { /* push bp */
            push(ram,c,c->bp); c->ip=0x0001u; break; }
        case 0x0001u: { /* mov bp, sp */
            c->bp=(uint16_t)(c->sp); c->ip=0x0003u; break; }
        case 0x0003u: { /* sub sp, 6 */
            c->sp=(uint16_t)(arithmetic(c,c->sp,6u,0u,16u,1u)); c->ip=0x0006u; break; }
        case 0x0006u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0009u; break; }
        case 0x0009u: { /* mov ax, word ptr es:[di + 0xcc] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xcc))); c->ip=0x000eu; break; }
        case 0x000eu: { /* mov dx, word ptr es:[di + 0xce] */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xce))); c->ip=0x0013u; break; }
        case 0x0013u: { /* mov word ptr [bp - 4], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); c->ip=0x0016u; break; }
        case 0x0016u: { /* mov word ptr [bp - 2], dx */
            write16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); c->ip=0x0019u; break; }
        case 0x0019u: { /* mov ax, word ptr [bp - 4] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x001cu; break; }
        case 0x001cu: { /* or ax, word ptr [bp - 2] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->ss,(uint16_t)(c->bp - 2))),16u)); c->ip=0x001fu; break; }
        case 0x001fu: { /* jne 0x24 */
            c->ip=(!(c->flags&0x40u))?0x24u:0x0021u; break; }
        case 0x0021u: { /* jmp 0x1f5 */
            c->ip=0x1f5u; break; }
        case 0x0024u: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0027u; break; }
        case 0x0027u: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x002bu; break; }
        case 0x002bu: { /* mov byte ptr [bp - 5], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 5))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x002eu; break; }
        case 0x002eu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0031u; break; }
        case 0x0031u: { /* mov al, byte ptr es:[di + 0x2d] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2d))])); c->ip=0x0035u; break; }
        case 0x0035u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0038u; break; }
        case 0x0038u: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x003du; break; }
        case 0x003du: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x0040u; break; }
        case 0x0040u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0042u; break; }
        case 0x0042u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0044u; break; }
        case 0x0044u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x0046u; break; }
        case 0x0046u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x0048u; break; }
        case 0x0048u: { /* mov al, byte ptr [di + 0x5590] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5590))])); c->ip=0x004cu; break; }
        case 0x004cu: { /* and al, 2 */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&2u),8u))); c->ip=0x004eu; break; }
        case 0x004eu: { /* or al, al */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8u))); c->ip=0x0050u; break; }
        case 0x0050u: { /* je 0x73 */
            c->ip=(c->flags&0x40u)?0x73u:0x0052u; break; }
        case 0x0052u: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0055u; break; }
        case 0x0055u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0058u; break; }
        case 0x0058u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0059u; break; }
        case 0x0059u: { /* call 0x1168 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x005cu);c->ip=0x1168u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x005cu)return PR_RES00BA_TRANSFER; break; }
        case 0x005cu: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x005du; break; }
        case 0x005du: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x005fu; break; }
        case 0x005fu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0062u; break; }
        case 0x0062u: { /* mov al, byte ptr es:[di + 0x110] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x110))])); c->ip=0x0067u; break; }
        case 0x0067u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0069u; break; }
        case 0x0069u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x006bu; break; }
        case 0x006bu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x006eu; break; }
        case 0x006eu: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0073u; break; }
        case 0x0073u: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x0076u; break; }
        case 0x0076u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0078u; break; }
        case 0x0078u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x007au; break; }
        case 0x007au: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x007cu; break; }
        case 0x007cu: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x007eu; break; }
        case 0x007eu: { /* mov al, byte ptr [di + 0x558d] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x558d))])); c->ip=0x0082u; break; }
        case 0x0082u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0085u; break; }
        case 0x0085u: { /* mov byte ptr es:[di + 0x119], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x119))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x008au; break; }
        case 0x008au: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x008du; break; }
        case 0x008du: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x008fu; break; }
        case 0x008fu: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0091u; break; }
        case 0x0091u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x0093u; break; }
        case 0x0093u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x0095u; break; }
        case 0x0095u: { /* mov al, byte ptr [di + 0x5590] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5590))])); c->ip=0x0099u; break; }
        case 0x0099u: { /* and al, 4 */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&4u),8u))); c->ip=0x009bu; break; }
        case 0x009bu: { /* or al, al */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8u))); c->ip=0x009du; break; }
        case 0x009du: { /* je 0xe0 */
            c->ip=(c->flags&0x40u)?0xe0u:0x009fu; break; }
        case 0x009fu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x00a2u; break; }
        case 0x00a2u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x00a5u; break; }
        case 0x00a5u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x00a6u; break; }
        case 0x00a6u: { /* call 0x1282 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x00a9u);c->ip=0x1282u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x00a9u)return PR_RES00BA_TRANSFER; break; }
        case 0x00a9u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x00aau; break; }
        case 0x00aau: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x00acu; break; }
        case 0x00acu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00afu; break; }
        case 0x00afu: { /* mov al, byte ptr es:[di + 0x110] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x110))])); c->ip=0x00b4u; break; }
        case 0x00b4u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x00b6u; break; }
        case 0x00b6u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x00b8u; break; }
        case 0x00b8u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00bbu; break; }
        case 0x00bbu: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x00c0u; break; }
        case 0x00c0u: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x00c3u; break; }
        case 0x00c3u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x00c6u; break; }
        case 0x00c6u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x00c7u; break; }
        case 0x00c7u: { /* call 0x1325 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x00cau);c->ip=0x1325u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x00cau)return PR_RES00BA_TRANSFER; break; }
        case 0x00cau: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x00cbu; break; }
        case 0x00cbu: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x00cdu; break; }
        case 0x00cdu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00d0u; break; }
        case 0x00d0u: { /* mov al, byte ptr es:[di + 0x119] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x119))])); c->ip=0x00d5u; break; }
        case 0x00d5u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x00d6u; break; }
        case 0x00d6u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x00d8u; break; }
        case 0x00d8u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00dbu; break; }
        case 0x00dbu: { /* mov byte ptr es:[di + 0x119], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x119))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x00e0u; break; }
        case 0x00e0u: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00e3u; break; }
        case 0x00e3u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x00e7u; break; }
        case 0x00e7u: { /* mov byte ptr [bp - 6], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 6))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x00eau; break; }
        case 0x00eau: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x00edu; break; }
        case 0x00edu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x00efu; break; }
        case 0x00efu: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x00f1u; break; }
        case 0x00f1u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x00f3u; break; }
        case 0x00f3u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x00f5u; break; }
        case 0x00f5u: { /* cmp byte ptr [di + 0x5590], 0x7f */
            (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(c->di + 0x5590))],0x7fu,0u,8u,1u); c->ip=0x00fau; break; }
        case 0x00fau: { /* jbe 0x123 */
            c->ip=(c->flags&0x41u)?0x123u:0x00fcu; break; }
        case 0x00fcu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x00ffu; break; }
        case 0x00ffu: { /* mov ax, word ptr es:[di + 0xfc] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xfc))); c->ip=0x0104u; break; }
        case 0x0104u: { /* or ax, word ptr es:[di + 0xfe] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->es,(uint16_t)(c->di + 0xfe))),16u)); c->ip=0x0109u; break; }
        case 0x0109u: { /* je 0x123 */
            c->ip=(c->flags&0x40u)?0x123u:0x010bu; break; }
        case 0x010bu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x010eu; break; }
        case 0x010eu: { /* les di, ptr es:[di + 0xfc] */
            size_t p=physical(c->es,(uint16_t)(c->di + 0xfc)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0113u; break; }
        case 0x0113u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x0117u; break; }
        case 0x0117u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0118u; break; }
        case 0x0118u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x011au; break; }
        case 0x011au: { /* mov al, byte ptr [bp - 6] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 6))])); c->ip=0x011du; break; }
        case 0x011du: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x011eu; break; }
        case 0x011eu: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0120u; break; }
        case 0x0120u: { /* mov byte ptr [bp - 6], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 6))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0123u; break; }
        case 0x0123u: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x0126u; break; }
        case 0x0126u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0128u; break; }
        case 0x0128u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x012au; break; }
        case 0x012au: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x012cu; break; }
        case 0x012cu: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x012eu; break; }
        case 0x012eu: { /* mov al, byte ptr [di + 0x5590] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5590))])); c->ip=0x0132u; break; }
        case 0x0132u: { /* and al, 1 */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&1u),8u))); c->ip=0x0134u; break; }
        case 0x0134u: { /* or al, al */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8u))); c->ip=0x0136u; break; }
        case 0x0136u: { /* jbe 0x15f */
            c->ip=(c->flags&0x41u)?0x15fu:0x0138u; break; }
        case 0x0138u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x013bu; break; }
        case 0x013bu: { /* mov ax, word ptr es:[di + 0xf8] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xf8))); c->ip=0x0140u; break; }
        case 0x0140u: { /* or ax, word ptr es:[di + 0xfa] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->es,(uint16_t)(c->di + 0xfa))),16u)); c->ip=0x0145u; break; }
        case 0x0145u: { /* je 0x15f */
            c->ip=(c->flags&0x40u)?0x15fu:0x0147u; break; }
        case 0x0147u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x014au; break; }
        case 0x014au: { /* les di, ptr es:[di + 0xf8] */
            size_t p=physical(c->es,(uint16_t)(c->di + 0xf8)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x014fu; break; }
        case 0x014fu: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x0153u; break; }
        case 0x0153u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0154u; break; }
        case 0x0154u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0156u; break; }
        case 0x0156u: { /* mov al, byte ptr [bp - 6] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 6))])); c->ip=0x0159u; break; }
        case 0x0159u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x015au; break; }
        case 0x015au: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x015cu; break; }
        case 0x015cu: { /* mov byte ptr [bp - 6], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 6))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x015fu; break; }
        case 0x015fu: { /* mov al, byte ptr [bp - 6] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 6))])); c->ip=0x0162u; break; }
        case 0x0162u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0163u; break; }
        case 0x0163u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0165u; break; }
        case 0x0165u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0168u; break; }
        case 0x0168u: { /* mov al, byte ptr es:[di + 0x119] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x119))])); c->ip=0x016du; break; }
        case 0x016du: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x016eu; break; }
        case 0x016eu: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0170u; break; }
        case 0x0170u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0173u; break; }
        case 0x0173u: { /* mov byte ptr es:[di + 0x119], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x119))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0178u; break; }
        case 0x0178u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x017bu; break; }
        case 0x017bu: { /* cmp byte ptr es:[di + 0x2e], 2 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],2u,0u,8u,1u); c->ip=0x0180u; break; }
        case 0x0180u: { /* jne 0x1ad */
            c->ip=(!(c->flags&0x40u))?0x1adu:0x0182u; break; }
        case 0x0182u: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0185u; break; }
        case 0x0185u: { /* cmp byte ptr es:[di + 0x2e], 0x28 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x28u,0u,8u,1u); c->ip=0x018au; break; }
        case 0x018au: { /* jbe 0x196 */
            c->ip=(c->flags&0x41u)?0x196u:0x018cu; break; }
        case 0x018cu: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x018fu; break; }
        case 0x018fu: { /* cmp byte ptr es:[di + 0x2e], 0x2d */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x2du,0u,8u,1u); c->ip=0x0194u; break; }
        case 0x0194u: { /* jb 0x1aa */
            c->ip=(c->flags&1u)?0x1aau:0x0196u; break; }
        case 0x0196u: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0199u; break; }
        case 0x0199u: { /* cmp byte ptr es:[di + 0x2e], 0x25 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x25u,0u,8u,1u); c->ip=0x019eu; break; }
        case 0x019eu: { /* je 0x1aa */
            c->ip=(c->flags&0x40u)?0x1aau:0x01a0u; break; }
        case 0x01a0u: { /* les di, ptr [bp - 4] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x01a3u; break; }
        case 0x01a3u: { /* cmp byte ptr es:[di + 0x2e], 0x24 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x24u,0u,8u,1u); c->ip=0x01a8u; break; }
        case 0x01a8u: { /* jne 0x1ad */
            c->ip=(!(c->flags&0x40u))?0x1adu:0x01aau; break; }
        case 0x01aau: { /* inc byte ptr [bp - 6] */
            ram[physical(c->ss,(uint16_t)(c->bp - 6))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 6))],8u,0u)); c->ip=0x01adu; break; }
        case 0x01adu: { /* mov al, byte ptr [bp - 6] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 6))])); c->ip=0x01b0u; break; }
        case 0x01b0u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x01b1u; break; }
        case 0x01b1u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x01b3u; break; }
        case 0x01b3u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x01b6u; break; }
        case 0x01b6u: { /* mov al, byte ptr es:[di + 0x110] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x110))])); c->ip=0x01bbu; break; }
        case 0x01bbu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x01bdu; break; }
        case 0x01bdu: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x01bfu; break; }
        case 0x01bfu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x01c2u; break; }
        case 0x01c2u: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x01c7u; break; }
        case 0x01c7u: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x01cau; break; }
        case 0x01cau: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x01ccu; break; }
        case 0x01ccu: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x01ceu; break; }
        case 0x01ceu: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x01d0u; break; }
        case 0x01d0u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x01d2u; break; }
        case 0x01d2u: { /* mov al, byte ptr [di + 0x558b] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x558b))])); c->ip=0x01d6u; break; }
        case 0x01d6u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x01d9u; break; }
        case 0x01d9u: { /* mov byte ptr es:[di + 0x115], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x115))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x01deu; break; }
        case 0x01deu: { /* mov al, byte ptr [bp - 5] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x01e1u; break; }
        case 0x01e1u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x01e3u; break; }
        case 0x01e3u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x01e5u; break; }
        case 0x01e5u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x01e7u; break; }
        case 0x01e7u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x01e9u; break; }
        case 0x01e9u: { /* mov al, byte ptr [di + 0x558c] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x558c))])); c->ip=0x01edu; break; }
        case 0x01edu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x01f0u; break; }
        case 0x01f0u: { /* mov byte ptr es:[di + 0x117], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x117))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x01f5u; break; }
        case 0x01f5u: { /* mov sp, bp */
            c->sp=(uint16_t)(c->bp); c->ip=0x01f7u; break; }
        case 0x01f7u: { /* pop bp */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x01f8u; break; }
        case 0x01f8u: { /* retf 4 */
            c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+4u);return PR_RES00BA_OK; break; }
        case 0x01fbu: { /* push bp */
            push(ram,c,c->bp); c->ip=0x01fcu; break; }
        case 0x01fcu: { /* mov bp, sp */
            c->bp=(uint16_t)(c->sp); c->ip=0x01feu; break; }
        case 0x01feu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0201u; break; }
        case 0x0201u: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x0205u; break; }
        case 0x0205u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0207u; break; }
        case 0x0207u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0209u; break; }
        case 0x0209u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x020bu; break; }
        case 0x020bu: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x020du; break; }
        case 0x020du: { /* cmp byte ptr [di + 0x5582], 2 */
            (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(c->di + 0x5582))],2u,0u,8u,1u); c->ip=0x0212u; break; }
        case 0x0212u: { /* jne 0x27e */
            c->ip=(!(c->flags&0x40u))?0x27eu:0x0214u; break; }
        case 0x0214u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0217u; break; }
        case 0x0217u: { /* mov ax, word ptr es:[di + 0x37] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x37))); c->ip=0x021bu; break; }
        case 0x021bu: { /* cmp ax, 0 */
            (void)arithmetic(c,c->ax,0u,0u,16u,1u); c->ip=0x021eu; break; }
        case 0x021eu: { /* jb 0x236 */
            c->ip=(c->flags&1u)?0x236u:0x0220u; break; }
        case 0x0220u: { /* cmp ax, 0x96 */
            (void)arithmetic(c,c->ax,0x96u,0u,16u,1u); c->ip=0x0223u; break; }
        case 0x0223u: { /* ja 0x236 */
            c->ip=(!(c->flags&0x41u))?0x236u:0x0225u; break; }
        case 0x0225u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0228u; break; }
        case 0x0228u: { /* mov al, byte ptr es:[di + 0x72] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x72))])); c->ip=0x022cu; break; }
        case 0x022cu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x022fu; break; }
        case 0x022fu: { /* mov byte ptr es:[di + 0x11c], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0234u; break; }
        case 0x0234u: { /* jmp 0x254 */
            c->ip=0x254u; break; }
        case 0x0236u: { /* cmp ax, 0x97 */
            (void)arithmetic(c,c->ax,0x97u,0u,16u,1u); c->ip=0x0239u; break; }
        case 0x0239u: { /* jb 0x24b */
            c->ip=(c->flags&1u)?0x24bu:0x023bu; break; }
        case 0x023bu: { /* cmp ax, 0x18f */
            (void)arithmetic(c,c->ax,0x18fu,0u,16u,1u); c->ip=0x023eu; break; }
        case 0x023eu: { /* ja 0x24b */
            c->ip=(!(c->flags&0x41u))?0x24bu:0x0240u; break; }
        case 0x0240u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0243u; break; }
        case 0x0243u: { /* mov byte ptr es:[di + 0x11c], 9 */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)(9u); c->ip=0x0249u; break; }
        case 0x0249u: { /* jmp 0x254 */
            c->ip=0x254u; break; }
        case 0x024bu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x024eu; break; }
        case 0x024eu: { /* mov byte ptr es:[di + 0x11c], 6 */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)(6u); c->ip=0x0254u; break; }
        case 0x0254u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0257u; break; }
        case 0x0257u: { /* cmp byte ptr es:[di + 0x32], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x32))],0u,0u,8u,1u); c->ip=0x025cu; break; }
        case 0x025cu: { /* je 0x27e */
            c->ip=(c->flags&0x40u)?0x27eu:0x025eu; break; }
        case 0x025eu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0261u; break; }
        case 0x0261u: { /* cmp byte ptr es:[di + 0x11c], 9 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x11c))],9u,0u,8u,1u); c->ip=0x0267u; break; }
        case 0x0267u: { /* ja 0x27e */
            c->ip=(!(c->flags&0x41u))?0x27eu:0x0269u; break; }
        case 0x0269u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x026cu; break; }
        case 0x026cu: { /* mov al, byte ptr es:[di + 0x11c] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x11c))])); c->ip=0x0271u; break; }
        case 0x0271u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0273u; break; }
        case 0x0273u: { /* add ax, 3 */
            c->ax=(uint16_t)(arithmetic(c,c->ax,3u,0u,16u,0u)); c->ip=0x0276u; break; }
        case 0x0276u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0279u; break; }
        case 0x0279u: { /* mov byte ptr es:[di + 0x11c], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x027eu; break; }
        case 0x027eu: { /* pop bp */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x027fu; break; }
        case 0x027fu: { /* retf 8 */
            c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+8u);return PR_RES00BA_OK; break; }
        case 0x0282u: { /* push bp */
            push(ram,c,c->bp); c->ip=0x0283u; break; }
        case 0x0283u: { /* mov bp, sp */
            c->bp=(uint16_t)(c->sp); c->ip=0x0285u; break; }
        case 0x0285u: { /* sub sp, 2 */
            c->sp=(uint16_t)(arithmetic(c,c->sp,2u,0u,16u,1u)); c->ip=0x0288u; break; }
        case 0x0288u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x028bu; break; }
        case 0x028bu: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x028fu; break; }
        case 0x028fu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0291u; break; }
        case 0x0291u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0293u; break; }
        case 0x0293u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x0295u; break; }
        case 0x0295u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x0297u; break; }
        case 0x0297u: { /* mov al, byte ptr [di + 0x5588] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5588))])); c->ip=0x029bu; break; }
        case 0x029bu: { /* mov byte ptr [bp - 1], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 1))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x029eu; break; }
        case 0x029eu: { /* cmp byte ptr [bp - 1], 0x7f */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 1))],0x7fu,0u,8u,1u); c->ip=0x02a2u; break; }
        case 0x02a2u: { /* ja 0x2a7 */
            c->ip=(!(c->flags&0x41u))?0x2a7u:0x02a4u; break; }
        case 0x02a4u: { /* jmp 0x39a */
            c->ip=0x39au; break; }
        case 0x02a7u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x02aau; break; }
        case 0x02aau: { /* and al, 0x7f */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x7fu),8u))); c->ip=0x02acu; break; }
        case 0x02acu: { /* mov byte ptr [bp - 1], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 1))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x02afu; break; }
        case 0x02afu: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x02b2u; break; }
        case 0x02b2u: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x02b6u; break; }
        case 0x02b6u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x02b8u; break; }
        case 0x02b8u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x02bau; break; }
        case 0x02bau: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x02bcu; break; }
        case 0x02bcu: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x02beu; break; }
        case 0x02beu: { /* mov al, byte ptr [di + 0x5582] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5582))])); c->ip=0x02c2u; break; }
        case 0x02c2u: { /* mov byte ptr [bp - 2], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 2))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x02c5u; break; }
        case 0x02c5u: { /* cmp byte ptr [bp - 2], 1 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],1u,0u,8u,1u); c->ip=0x02c9u; break; }
        case 0x02c9u: { /* jne 0x2e6 */
            c->ip=(!(c->flags&0x40u))?0x2e6u:0x02cbu; break; }
        case 0x02cbu: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x02ceu; break; }
        case 0x02ceu: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x02d2u; break; }
        case 0x02d2u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x02d3u; break; }
        case 0x02d3u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x02d5u; break; }
        case 0x02d5u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x02d8u; break; }
        case 0x02d8u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x02dau; break; }
        case 0x02dau: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x02dcu; break; }
        case 0x02dcu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x02dfu; break; }
        case 0x02dfu: { /* mov byte ptr es:[di + 1], al */
            ram[physical(c->es,(uint16_t)(c->di + 1))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x02e3u; break; }
        case 0x02e3u: { /* jmp 0x39a */
            c->ip=0x39au; break; }
        case 0x02e6u: { /* cmp byte ptr [bp - 1], 0 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 1))],0u,0u,8u,1u); c->ip=0x02eau; break; }
        case 0x02eau: { /* jne 0x34c */
            c->ip=(!(c->flags&0x40u))?0x34cu:0x02ecu; break; }
        case 0x02ecu: { /* cmp byte ptr [bp - 2], 9 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],9u,0u,8u,1u); c->ip=0x02f0u; break; }
        case 0x02f0u: { /* jne 0x312 */
            c->ip=(!(c->flags&0x40u))?0x312u:0x02f2u; break; }
        case 0x02f2u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x02f5u; break; }
        case 0x02f5u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x02f9u; break; }
        case 0x02f9u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x02fcu; break; }
        case 0x02fcu: { /* cmp al, byte ptr es:[di + 3] */
            (void)arithmetic(c,(uint8_t)(c->ax),ram[physical(c->es,(uint16_t)(c->di + 3))],0u,8u,1u); c->ip=0x0300u; break; }
        case 0x0300u: { /* jle 0x310 */
            c->ip=((c->flags&0x40u)||(((c->flags>>7u)^(c->flags>>11u))&1u))?0x310u:0x0302u; break; }
        case 0x0302u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0305u; break; }
        case 0x0305u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x0309u; break; }
        case 0x0309u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x030cu; break; }
        case 0x030cu: { /* mov byte ptr es:[di + 3], al */
            ram[physical(c->es,(uint16_t)(c->di + 3))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0310u; break; }
        case 0x0310u: { /* jmp 0x32d */
            c->ip=0x32du; break; }
        case 0x0312u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0315u; break; }
        case 0x0315u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x0319u; break; }
        case 0x0319u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x031au; break; }
        case 0x031au: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x031cu; break; }
        case 0x031cu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x031fu; break; }
        case 0x031fu: { /* mov al, byte ptr es:[di + 2] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 2))])); c->ip=0x0323u; break; }
        case 0x0323u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0324u; break; }
        case 0x0324u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0326u; break; }
        case 0x0326u: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0329u; break; }
        case 0x0329u: { /* mov byte ptr es:[di + 2], al */
            ram[physical(c->es,(uint16_t)(c->di + 2))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x032du; break; }
        case 0x032du: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0330u; break; }
        case 0x0330u: { /* mov al, byte ptr es:[di + 0x33] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x33))])); c->ip=0x0334u; break; }
        case 0x0334u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0335u; break; }
        case 0x0335u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0337u; break; }
        case 0x0337u: { /* les di, ptr [bp + 0x12] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0x12)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x033au; break; }
        case 0x033au: { /* mov al, byte ptr es:[di + 0x101] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x101))])); c->ip=0x033fu; break; }
        case 0x033fu: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0340u; break; }
        case 0x0340u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0342u; break; }
        case 0x0342u: { /* les di, ptr [bp + 0x12] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0x12)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0345u; break; }
        case 0x0345u: { /* mov byte ptr es:[di + 0x101], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x101))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x034au; break; }
        case 0x034au: { /* jmp 0x39a */
            c->ip=0x39au; break; }
        case 0x034cu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x034fu; break; }
        case 0x034fu: { /* mov al, byte ptr es:[di + 4] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 4))])); c->ip=0x0353u; break; }
        case 0x0353u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0354u; break; }
        case 0x0354u: { /* mov cx, ax */
            c->cx=(uint16_t)(c->ax); c->ip=0x0356u; break; }
        case 0x0356u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0359u; break; }
        case 0x0359u: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x035du; break; }
        case 0x035du: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x035eu; break; }
        case 0x035eu: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0360u; break; }
        case 0x0360u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0363u; break; }
        case 0x0363u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0365u; break; }
        case 0x0365u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0367u; break; }
        case 0x0367u: { /* cmp ax, cx */
            (void)arithmetic(c,c->ax,c->cx,0u,16u,1u); c->ip=0x0369u; break; }
        case 0x0369u: { /* jle 0x39a */
            c->ip=((c->flags&0x40u)||(((c->flags>>7u)^(c->flags>>11u))&1u))?0x39au:0x036bu; break; }
        case 0x036bu: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x036eu; break; }
        case 0x036eu: { /* mov al, byte ptr es:[di + 0x32] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x32))])); c->ip=0x0372u; break; }
        case 0x0372u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0373u; break; }
        case 0x0373u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0375u; break; }
        case 0x0375u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0378u; break; }
        case 0x0378u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x037au; break; }
        case 0x037au: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x037cu; break; }
        case 0x037cu: { /* les di, ptr [bp + 0xa] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xa)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x037fu; break; }
        case 0x037fu: { /* mov byte ptr es:[di + 4], al */
            ram[physical(c->es,(uint16_t)(c->di + 4))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0383u; break; }
        case 0x0383u: { /* les di, ptr [bp + 0xe] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 0xe)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0386u; break; }
        case 0x0386u: { /* cmp byte ptr es:[di + 0x32], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x32))],0u,0u,8u,1u); c->ip=0x038bu; break; }
        case 0x038bu: { /* jle 0x39a */
            c->ip=((c->flags&0x40u)||(((c->flags>>7u)^(c->flags>>11u))&1u))?0x39au:0x038du; break; }
        case 0x038du: { /* cmp byte ptr [bp - 2], 2 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],2u,0u,8u,1u); c->ip=0x0391u; break; }
        case 0x0391u: { /* jne 0x39a */
            c->ip=(!(c->flags&0x40u))?0x39au:0x0393u; break; }
        case 0x0393u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0396u; break; }
        case 0x0396u: { /* mov byte ptr es:[di], 1 */
            ram[physical(c->es,(uint16_t)(c->di))]=(uint8_t)(1u); c->ip=0x039au; break; }
        case 0x039au: { /* mov sp, bp */
            c->sp=(uint16_t)(c->bp); c->ip=0x039cu; break; }
        case 0x039cu: { /* pop bp */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x039du; break; }
        case 0x039du: { /* retf 0x10 */
            c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+0x10u);return PR_RES00BA_OK; break; }
        case 0x03a0u: { /* push bp */
            push(ram,c,c->bp); c->ip=0x03a1u; break; }
        case 0x03a1u: { /* mov bp, sp */
            c->bp=(uint16_t)(c->sp); c->ip=0x03a3u; break; }
        case 0x03a3u: { /* sub sp, 4 */
            c->sp=(uint16_t)(arithmetic(c,c->sp,4u,0u,16u,1u)); c->ip=0x03a6u; break; }
        case 0x03a6u: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x03a9u; break; }
        case 0x03a9u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x03acu; break; }
        case 0x03acu: { /* push cs */
            push(ram,c,c->cs); c->ip=0x03adu; break; }
        case 0x03adu: { /* call 0x13ab */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x03b0u);c->ip=0x13abu; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x03b0u)return PR_RES00BA_TRANSFER; break; }
        case 0x03b0u: { /* cdq  */
            c->dx=(uint16_t)((c->ax&0x8000u)?0xffffu:0u); c->ip=0x03b1u; break; }
        case 0x03b1u: { /* mov cx, ax */
            c->cx=(uint16_t)(c->ax); c->ip=0x03b3u; break; }
        case 0x03b3u: { /* mov bx, dx */
            c->bx=(uint16_t)(c->dx); c->ip=0x03b5u; break; }
        case 0x03b5u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x03b8u; break; }
        case 0x03b8u: { /* mov ax, word ptr es:[di + 0x102] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x102))); c->ip=0x03bdu; break; }
        case 0x03bdu: { /* xor dx, dx */
            c->dx=(uint16_t)(logic(c,(uint16_t)(c->dx^c->dx),16u)); c->ip=0x03bfu; break; }
        case 0x03bfu: { /* sub ax, cx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->cx,0u,16u,1u)); c->ip=0x03c1u; break; }
        case 0x03c1u: { /* sbb dx, bx */
            c->dx=(uint16_t)(arithmetic(c,c->dx,c->bx,c->flags&1u,16u,1u)); c->ip=0x03c3u; break; }
        case 0x03c3u: { /* mov word ptr [bp - 2], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->ax)); c->ip=0x03c6u; break; }
        case 0x03c6u: { /* cmp word ptr [bp - 2], 0 */
            (void)arithmetic(c,read16(ram,c->ss,(uint16_t)(c->bp - 2)),0u,0u,16u,1u); c->ip=0x03cau; break; }
        case 0x03cau: { /* jge 0x3d1 */
            c->ip=(!(((c->flags>>7u)^(c->flags>>11u))&1u))?0x3d1u:0x03ccu; break; }
        case 0x03ccu: { /* xor ax, ax */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u)); c->ip=0x03ceu; break; }
        case 0x03ceu: { /* mov word ptr [bp - 2], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->ax)); c->ip=0x03d1u; break; }
        case 0x03d1u: { /* mov ax, word ptr [bp - 2] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 2))); c->ip=0x03d4u; break; }
        case 0x03d4u: { /* cmp ax, 0 */
            (void)arithmetic(c,c->ax,0u,0u,16u,1u); c->ip=0x03d7u; break; }
        case 0x03d7u: { /* jl 0x3eb */
            c->ip=(((c->flags>>7u)^(c->flags>>11u))&1u)?0x3ebu:0x03d9u; break; }
        case 0x03d9u: { /* cmp ax, 0x200 */
            (void)arithmetic(c,c->ax,0x200u,0u,16u,1u); c->ip=0x03dcu; break; }
        case 0x03dcu: { /* jg 0x3eb */
            c->ip=(!(c->flags&0x40u)&&!(((c->flags>>7u)^(c->flags>>11u))&1u))?0x3ebu:0x03deu; break; }
        case 0x03deu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x03e1u; break; }
        case 0x03e1u: { /* mov al, byte ptr es:[di + 0x11c] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x11c))])); c->ip=0x03e6u; break; }
        case 0x03e6u: { /* mov byte ptr [bp - 3], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 3))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x03e9u; break; }
        case 0x03e9u: { /* jmp 0x40f */
            c->ip=0x40fu; break; }
        case 0x03ebu: { /* cmp ax, 0x201 */
            (void)arithmetic(c,c->ax,0x201u,0u,16u,1u); c->ip=0x03eeu; break; }
        case 0x03eeu: { /* jl 0x3fb */
            c->ip=(((c->flags>>7u)^(c->flags>>11u))&1u)?0x3fbu:0x03f0u; break; }
        case 0x03f0u: { /* cmp ax, 0x300 */
            (void)arithmetic(c,c->ax,0x300u,0u,16u,1u); c->ip=0x03f3u; break; }
        case 0x03f3u: { /* jg 0x3fb */
            c->ip=(!(c->flags&0x40u)&&!(((c->flags>>7u)^(c->flags>>11u))&1u))?0x3fbu:0x03f5u; break; }
        case 0x03f5u: { /* mov byte ptr [bp - 3], 9 */
            ram[physical(c->ss,(uint16_t)(c->bp - 3))]=(uint8_t)(9u); c->ip=0x03f9u; break; }
        case 0x03f9u: { /* jmp 0x40f */
            c->ip=0x40fu; break; }
        case 0x03fbu: { /* cmp ax, 0x301 */
            (void)arithmetic(c,c->ax,0x301u,0u,16u,1u); c->ip=0x03feu; break; }
        case 0x03feu: { /* jl 0x40b */
            c->ip=(((c->flags>>7u)^(c->flags>>11u))&1u)?0x40bu:0x0400u; break; }
        case 0x0400u: { /* cmp ax, 0x400 */
            (void)arithmetic(c,c->ax,0x400u,0u,16u,1u); c->ip=0x0403u; break; }
        case 0x0403u: { /* jg 0x40b */
            c->ip=(!(c->flags&0x40u)&&!(((c->flags>>7u)^(c->flags>>11u))&1u))?0x40bu:0x0405u; break; }
        case 0x0405u: { /* mov byte ptr [bp - 3], 6 */
            ram[physical(c->ss,(uint16_t)(c->bp - 3))]=(uint8_t)(6u); c->ip=0x0409u; break; }
        case 0x0409u: { /* jmp 0x40f */
            c->ip=0x40fu; break; }
        case 0x040bu: { /* mov byte ptr [bp - 3], 3 */
            ram[physical(c->ss,(uint16_t)(c->bp - 3))]=(uint8_t)(3u); c->ip=0x040fu; break; }
        case 0x040fu: { /* mov al, byte ptr [bp - 3] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 3))])); c->ip=0x0412u; break; }
        case 0x0412u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0415u; break; }
        case 0x0415u: { /* cmp al, byte ptr es:[di + 0x11c] */
            (void)arithmetic(c,(uint8_t)(c->ax),ram[physical(c->es,(uint16_t)(c->di + 0x11c))],0u,8u,1u); c->ip=0x041au; break; }
        case 0x041au: { /* jae 0x427 */
            c->ip=(!(c->flags&1u))?0x427u:0x041cu; break; }
        case 0x041cu: { /* mov al, byte ptr [bp - 3] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 3))])); c->ip=0x041fu; break; }
        case 0x041fu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0422u; break; }
        case 0x0422u: { /* mov byte ptr es:[di + 0x11c], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0427u; break; }
        case 0x0427u: { /* mov sp, bp */
            c->sp=(uint16_t)(c->bp); c->ip=0x0429u; break; }
        case 0x0429u: { /* pop bp */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x042au; break; }
        case 0x042au: { /* retf 4 */
            c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+4u);return PR_RES00BA_OK; break; }
        case 0x0bb8u: { /* push bp */
            push(ram,c,c->bp); c->ip=0x0bb9u; break; }
        case 0x0bb9u: { /* mov bp, sp */
            c->bp=(uint16_t)(c->sp); c->ip=0x0bbbu; break; }
        case 0x0bbbu: { /* sub sp, 0x14 */
            c->sp=(uint16_t)(arithmetic(c,c->sp,0x14u,0u,16u,1u)); c->ip=0x0bbeu; break; }
        case 0x0bbeu: { /* mov byte ptr [bp - 0xe], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 0xe))]=(uint8_t)(0u); c->ip=0x0bc2u; break; }
        case 0x0bc2u: { /* jmp 0xbc7 */
            c->ip=0xbc7u; break; }
        case 0x0bc4u: { /* inc byte ptr [bp - 0xe] */
            ram[physical(c->ss,(uint16_t)(c->bp - 0xe))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 0xe))],8u,0u)); c->ip=0x0bc7u; break; }
        case 0x0bc7u: { /* mov al, byte ptr [bp - 0xe] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0xe))])); c->ip=0x0bcau; break; }
        case 0x0bcau: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0bcbu; break; }
        case 0x0bcbu: { /* shl ax, 1 */
            c->ax=(uint16_t)(shl(c,c->ax,1u)); c->ip=0x0bcdu; break; }
        case 0x0bcdu: { /* shl ax, 1 */
            c->ax=(uint16_t)(shl(c,c->ax,1u)); c->ip=0x0bcfu; break; }
        case 0x0bcfu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0bd2u; break; }
        case 0x0bd2u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0bd4u; break; }
        case 0x0bd4u: { /* xor ax, ax */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u)); c->ip=0x0bd6u; break; }
        case 0x0bd6u: { /* mov word ptr es:[di + 0xcc], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xcc),(uint16_t)(c->ax)); c->ip=0x0bdbu; break; }
        case 0x0bdbu: { /* mov word ptr es:[di + 0xce], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xce),(uint16_t)(c->ax)); c->ip=0x0be0u; break; }
        case 0x0be0u: { /* cmp byte ptr [bp - 0xe], 0xc */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 0xe))],0xcu,0u,8u,1u); c->ip=0x0be4u; break; }
        case 0x0be4u: { /* jne 0xbc4 */
            c->ip=(!(c->flags&0x40u))?0xbc4u:0x0be6u; break; }
        case 0x0be6u: { /* mov byte ptr [bp - 8], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 8))]=(uint8_t)(0u); c->ip=0x0beau; break; }
        case 0x0beau: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0bedu; break; }
        case 0x0bedu: { /* mov byte ptr es:[di + 0xc7], 0 */
            ram[physical(c->es,(uint16_t)(c->di + 0xc7))]=(uint8_t)(0u); c->ip=0x0bf3u; break; }
        case 0x0bf3u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0bf6u; break; }
        case 0x0bf6u: { /* mov ax, word ptr es:[di + 0xc8] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xc8))); c->ip=0x0bfbu; break; }
        case 0x0bfbu: { /* mov dx, word ptr es:[di + 0xca] */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xca))); c->ip=0x0c00u; break; }
        case 0x0c00u: { /* mov word ptr [bp - 6], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 6),(uint16_t)(c->ax)); c->ip=0x0c03u; break; }
        case 0x0c03u: { /* mov word ptr [bp - 4], dx */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->dx)); c->ip=0x0c06u; break; }
        case 0x0c06u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c09u; break; }
        case 0x0c09u: { /* mov byte ptr es:[di + 0x100], 0 */
            ram[physical(c->es,(uint16_t)(c->di + 0x100))]=(uint8_t)(0u); c->ip=0x0c0fu; break; }
        case 0x0c0fu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c12u; break; }
        case 0x0c12u: { /* xor ax, ax */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u)); c->ip=0x0c14u; break; }
        case 0x0c14u: { /* mov word ptr es:[di + 0x102], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(c->ax)); c->ip=0x0c19u; break; }
        case 0x0c19u: { /* xor ax, ax */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u)); c->ip=0x0c1bu; break; }
        case 0x0c1bu: { /* mov word ptr [0x4846], ax */
            write16(ram,c->ds,(uint16_t)(0x4846),(uint16_t)(c->ax)); c->ip=0x0c1eu; break; }
        case 0x0c1eu: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0c21u; break; }
        case 0x0c21u: { /* or ax, word ptr [bp - 4] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->ss,(uint16_t)(c->bp - 4))),16u)); c->ip=0x0c24u; break; }
        case 0x0c24u: { /* jne 0xc29 */
            c->ip=(!(c->flags&0x40u))?0xc29u:0x0c26u; break; }
        case 0x0c26u: { /* jmp 0xd84 */
            c->ip=0xd84u; break; }
        case 0x0c29u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c2cu; break; }
        case 0x0c2cu: { /* inc byte ptr es:[di + 0xc7] */
            ram[physical(c->es,(uint16_t)(c->di + 0xc7))]=(uint8_t)(step(c,ram[physical(c->es,(uint16_t)(c->di + 0xc7))],8u,0u)); c->ip=0x0c31u; break; }
        case 0x0c31u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c34u; break; }
        case 0x0c34u: { /* mov ax, word ptr es:[di + 0x37] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x37))); c->ip=0x0c38u; break; }
        case 0x0c38u: { /* mov word ptr [bp - 0x12], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 0x12),(uint16_t)(c->ax)); c->ip=0x0c3bu; break; }
        case 0x0c3bu: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c3eu; break; }
        case 0x0c3eu: { /* cmp byte ptr es:[di + 0x39], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x39))],0u,0u,8u,1u); c->ip=0x0c43u; break; }
        case 0x0c43u: { /* jbe 0xc54 */
            c->ip=(c->flags&0x41u)?0xc54u:0x0c45u; break; }
        case 0x0c45u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c48u; break; }
        case 0x0c48u: { /* mov al, byte ptr es:[di + 0x39] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x39))])); c->ip=0x0c4cu; break; }
        case 0x0c4cu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0c4eu; break; }
        case 0x0c4eu: { /* mul word ptr [bp - 0x12] */
            multiply(c,read16(ram,c->ss,(uint16_t)(c->bp - 0x12))); c->ip=0x0c51u; break; }
        case 0x0c51u: { /* mov word ptr [bp - 0x12], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 0x12),(uint16_t)(c->ax)); c->ip=0x0c54u; break; }
        case 0x0c54u: { /* mov ax, word ptr [bp - 0x12] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 0x12))); c->ip=0x0c57u; break; }
        case 0x0c57u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c5au; break; }
        case 0x0c5au: { /* add word ptr es:[di + 0x102], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(arithmetic(c,read16(ram,c->es,(uint16_t)(c->di + 0x102)),c->ax,0u,16u,0u))); c->ip=0x0c5fu; break; }
        case 0x0c5fu: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c62u; break; }
        case 0x0c62u: { /* cmp byte ptr es:[di + 0x34], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x34))],0u,0u,8u,1u); c->ip=0x0c67u; break; }
        case 0x0c67u: { /* jne 0xc6c */
            c->ip=(!(c->flags&0x40u))?0xc6cu:0x0c69u; break; }
        case 0x0c69u: { /* jmp 0xd70 */
            c->ip=0xd70u; break; }
        case 0x0c6cu: { /* mov ax, word ptr [bp - 0x12] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 0x12))); c->ip=0x0c6fu; break; }
        case 0x0c6fu: { /* add word ptr [0x4846], ax */
            write16(ram,c->ds,(uint16_t)(0x4846),(uint16_t)(arithmetic(c,read16(ram,c->ds,(uint16_t)(0x4846)),c->ax,0u,16u,0u))); c->ip=0x0c73u; break; }
        case 0x0c73u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0c76u; break; }
        case 0x0c76u: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x0c7au; break; }
        case 0x0c7au: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0c7cu; break; }
        case 0x0c7cu: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0c7eu; break; }
        case 0x0c7eu: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x0c80u; break; }
        case 0x0c80u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x0c82u; break; }
        case 0x0c82u: { /* mov al, byte ptr [di + 0x5582] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5582))])); c->ip=0x0c86u; break; }
        case 0x0c86u: { /* mov byte ptr [bp - 0x13], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 0x13))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0c89u; break; }
        case 0x0c89u: { /* mov al, byte ptr [bp - 0x13] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0x13))])); c->ip=0x0c8cu; break; }
        case 0x0c8cu: { /* cmp al, 0 */
            (void)arithmetic(c,(uint8_t)(c->ax),0u,0u,8u,1u); c->ip=0x0c8eu; break; }
        case 0x0c8eu: { /* jb 0xcb3 */
            c->ip=(c->flags&1u)?0xcb3u:0x0c90u; break; }
        case 0x0c90u: { /* cmp al, 8 */
            (void)arithmetic(c,(uint8_t)(c->ax),8u,0u,8u,1u); c->ip=0x0c92u; break; }
        case 0x0c92u: { /* ja 0xcb3 */
            c->ip=(!(c->flags&0x41u))?0xcb3u:0x0c94u; break; }
        case 0x0c94u: { /* mov cx, word ptr [bp - 6] */
            c->cx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0c97u; break; }
        case 0x0c97u: { /* mov bx, word ptr [bp - 4] */
            c->bx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0c9au; break; }
        case 0x0c9au: { /* mov al, byte ptr [bp - 0x13] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0x13))])); c->ip=0x0c9du; break; }
        case 0x0c9du: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0c9eu; break; }
        case 0x0c9eu: { /* shl ax, 1 */
            c->ax=(uint16_t)(shl(c,c->ax,1u)); c->ip=0x0ca0u; break; }
        case 0x0ca0u: { /* shl ax, 1 */
            c->ax=(uint16_t)(shl(c,c->ax,1u)); c->ip=0x0ca2u; break; }
        case 0x0ca2u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ca5u; break; }
        case 0x0ca5u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0ca7u; break; }
        case 0x0ca7u: { /* mov word ptr es:[di + 0xcc], cx */
            write16(ram,c->es,(uint16_t)(c->di + 0xcc),(uint16_t)(c->cx)); c->ip=0x0cacu; break; }
        case 0x0cacu: { /* mov word ptr es:[di + 0xce], bx */
            write16(ram,c->es,(uint16_t)(c->di + 0xce),(uint16_t)(c->bx)); c->ip=0x0cb1u; break; }
        case 0x0cb1u: { /* jmp 0xcfd */
            c->ip=0xcfdu; break; }
        case 0x0cb3u: { /* cmp al, 9 */
            (void)arithmetic(c,(uint8_t)(c->ax),9u,0u,8u,1u); c->ip=0x0cb5u; break; }
        case 0x0cb5u: { /* jne 0xcfd */
            c->ip=(!(c->flags&0x40u))?0xcfdu:0x0cb7u; break; }
        case 0x0cb7u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0cbau; break; }
        case 0x0cbau: { /* mov ax, word ptr es:[di + 0xf0] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xf0))); c->ip=0x0cbfu; break; }
        case 0x0cbfu: { /* or ax, word ptr es:[di + 0xf2] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->es,(uint16_t)(c->di + 0xf2))),16u)); c->ip=0x0cc4u; break; }
        case 0x0cc4u: { /* je 0xcea */
            c->ip=(c->flags&0x40u)?0xceau:0x0cc6u; break; }
        case 0x0cc6u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0cc9u; break; }
        case 0x0cc9u: { /* mov ax, word ptr es:[di + 0xf4] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xf4))); c->ip=0x0cceu; break; }
        case 0x0cceu: { /* or ax, word ptr es:[di + 0xf6] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->es,(uint16_t)(c->di + 0xf6))),16u)); c->ip=0x0cd3u; break; }
        case 0x0cd3u: { /* jne 0xce8 */
            c->ip=(!(c->flags&0x40u))?0xce8u:0x0cd5u; break; }
        case 0x0cd5u: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0cd8u; break; }
        case 0x0cd8u: { /* mov dx, word ptr [bp - 4] */
            c->dx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0cdbu; break; }
        case 0x0cdbu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0cdeu; break; }
        case 0x0cdeu: { /* mov word ptr es:[di + 0xf4], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xf4),(uint16_t)(c->ax)); c->ip=0x0ce3u; break; }
        case 0x0ce3u: { /* mov word ptr es:[di + 0xf6], dx */
            write16(ram,c->es,(uint16_t)(c->di + 0xf6),(uint16_t)(c->dx)); c->ip=0x0ce8u; break; }
        case 0x0ce8u: { /* jmp 0xcfd */
            c->ip=0xcfdu; break; }
        case 0x0ceau: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0cedu; break; }
        case 0x0cedu: { /* mov dx, word ptr [bp - 4] */
            c->dx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0cf0u; break; }
        case 0x0cf0u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0cf3u; break; }
        case 0x0cf3u: { /* mov word ptr es:[di + 0xf0], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xf0),(uint16_t)(c->ax)); c->ip=0x0cf8u; break; }
        case 0x0cf8u: { /* mov word ptr es:[di + 0xf2], dx */
            write16(ram,c->es,(uint16_t)(c->di + 0xf2),(uint16_t)(c->dx)); c->ip=0x0cfdu; break; }
        case 0x0cfdu: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d00u; break; }
        case 0x0d00u: { /* cmp byte ptr es:[di + 0x2e], 0x49 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x49u,0u,8u,1u); c->ip=0x0d05u; break; }
        case 0x0d05u: { /* jne 0xd1a */
            c->ip=(!(c->flags&0x40u))?0xd1au:0x0d07u; break; }
        case 0x0d07u: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0d0au; break; }
        case 0x0d0au: { /* mov dx, word ptr [bp - 4] */
            c->dx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0d0du; break; }
        case 0x0d0du: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d10u; break; }
        case 0x0d10u: { /* mov word ptr es:[di + 0xf8], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xf8),(uint16_t)(c->ax)); c->ip=0x0d15u; break; }
        case 0x0d15u: { /* mov word ptr es:[di + 0xfa], dx */
            write16(ram,c->es,(uint16_t)(c->di + 0xfa),(uint16_t)(c->dx)); c->ip=0x0d1au; break; }
        case 0x0d1au: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d1du; break; }
        case 0x0d1du: { /* cmp byte ptr es:[di + 0x2e], 0x1c */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0x1cu,0u,8u,1u); c->ip=0x0d22u; break; }
        case 0x0d22u: { /* jne 0xd37 */
            c->ip=(!(c->flags&0x40u))?0xd37u:0x0d24u; break; }
        case 0x0d24u: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0d27u; break; }
        case 0x0d27u: { /* mov dx, word ptr [bp - 4] */
            c->dx=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0d2au; break; }
        case 0x0d2au: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d2du; break; }
        case 0x0d2du: { /* mov word ptr es:[di + 0xfc], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0xfc),(uint16_t)(c->ax)); c->ip=0x0d32u; break; }
        case 0x0d32u: { /* mov word ptr es:[di + 0xfe], dx */
            write16(ram,c->es,(uint16_t)(c->di + 0xfe),(uint16_t)(c->dx)); c->ip=0x0d37u; break; }
        case 0x0d37u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d3au; break; }
        case 0x0d3au: { /* cmp byte ptr es:[di + 0x2f], 0xba */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2f))],0xbau,0u,8u,1u); c->ip=0x0d3fu; break; }
        case 0x0d3fu: { /* jne 0xd45 */
            c->ip=(!(c->flags&0x40u))?0xd45u:0x0d41u; break; }
        case 0x0d41u: { /* mov byte ptr [bp - 8], 1 */
            ram[physical(c->ss,(uint16_t)(c->bp - 8))]=(uint8_t)(1u); c->ip=0x0d45u; break; }
        case 0x0d45u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d48u; break; }
        case 0x0d48u: { /* mov al, byte ptr es:[di + 0x2e] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2e))])); c->ip=0x0d4cu; break; }
        case 0x0d4cu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0d4eu; break; }
        case 0x0d4eu: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0d50u; break; }
        case 0x0d50u: { /* mov cl, 4 */
            c->cx=(uint16_t)((c->cx&0xff00u)|(uint8_t)(4u)); c->ip=0x0d52u; break; }
        case 0x0d52u: { /* shl di, cl */
            c->di=(uint16_t)(shl(c,c->di,(uint8_t)(c->cx))); c->ip=0x0d54u; break; }
        case 0x0d54u: { /* mov al, byte ptr [di + 0x5583] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x5583))])); c->ip=0x0d58u; break; }
        case 0x0d58u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0d5au; break; }
        case 0x0d5au: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0d5cu; break; }
        case 0x0d5cu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d5fu; break; }
        case 0x0d5fu: { /* mov al, byte ptr es:[di + 0x100] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x100))])); c->ip=0x0d64u; break; }
        case 0x0d64u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0d66u; break; }
        case 0x0d66u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0d68u; break; }
        case 0x0d68u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d6bu; break; }
        case 0x0d6bu: { /* mov byte ptr es:[di + 0x100], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x100))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0d70u; break; }
        case 0x0d70u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d73u; break; }
        case 0x0d73u: { /* mov ax, word ptr es:[di + 0x2a] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x2a))); c->ip=0x0d77u; break; }
        case 0x0d77u: { /* mov dx, word ptr es:[di + 0x2c] */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x2c))); c->ip=0x0d7bu; break; }
        case 0x0d7bu: { /* mov word ptr [bp - 6], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 6),(uint16_t)(c->ax)); c->ip=0x0d7eu; break; }
        case 0x0d7eu: { /* mov word ptr [bp - 4], dx */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->dx)); c->ip=0x0d81u; break; }
        case 0x0d81u: { /* jmp 0xc1e */
            c->ip=0xc1eu; break; }
        case 0x0d84u: { /* mov byte ptr [bp - 0x10], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 0x10))]=(uint8_t)(0u); c->ip=0x0d88u; break; }
        case 0x0d88u: { /* jmp 0xd8d */
            c->ip=0xd8du; break; }
        case 0x0d8au: { /* inc byte ptr [bp - 0x10] */
            ram[physical(c->ss,(uint16_t)(c->bp - 0x10))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 0x10))],8u,0u)); c->ip=0x0d8du; break; }
        case 0x0d8du: { /* mov al, byte ptr [bp - 0x10] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0x10))])); c->ip=0x0d90u; break; }
        case 0x0d90u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0d91u; break; }
        case 0x0d91u: { /* shl ax, 1 */
            c->ax=(uint16_t)(shl(c,c->ax,1u)); c->ip=0x0d93u; break; }
        case 0x0d93u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0d96u; break; }
        case 0x0d96u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0d98u; break; }
        case 0x0d98u: { /* mov ax, word ptr es:[di + 0x88] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x88))); c->ip=0x0d9du; break; }
        case 0x0d9du: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0da0u; break; }
        case 0x0da0u: { /* add ax, word ptr es:[di + 0x102] */
            c->ax=(uint16_t)(arithmetic(c,c->ax,read16(ram,c->es,(uint16_t)(c->di + 0x102)),0u,16u,0u)); c->ip=0x0da5u; break; }
        case 0x0da5u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0da8u; break; }
        case 0x0da8u: { /* mov word ptr es:[di + 0x102], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(c->ax)); c->ip=0x0dadu; break; }
        case 0x0dadu: { /* cmp byte ptr [bp - 0x10], 6 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 0x10))],6u,0u,8u,1u); c->ip=0x0db1u; break; }
        case 0x0db1u: { /* jne 0xd8a */
            c->ip=(!(c->flags&0x40u))?0xd8au:0x0db3u; break; }
        case 0x0db3u: { /* mov byte ptr [bp - 1], 1 */
            ram[physical(c->ss,(uint16_t)(c->bp - 1))]=(uint8_t)(1u); c->ip=0x0db7u; break; }
        case 0x0db7u: { /* jmp 0xdbc */
            c->ip=0xdbcu; break; }
        case 0x0db9u: { /* inc byte ptr [bp - 1] */
            ram[physical(c->ss,(uint16_t)(c->bp - 1))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 1))],8u,0u)); c->ip=0x0dbcu; break; }
        case 0x0dbcu: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0dbfu; break; }
        case 0x0dbfu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0dc1u; break; }
        case 0x0dc1u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0dc4u; break; }
        case 0x0dc4u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0dc6u; break; }
        case 0x0dc6u: { /* mov dl, byte ptr es:[di + 0xa2] */
            c->dx=(uint16_t)((c->dx&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0xa2))])); c->ip=0x0dcbu; break; }
        case 0x0dcbu: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0dceu; break; }
        case 0x0dceu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0dd0u; break; }
        case 0x0dd0u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0dd3u; break; }
        case 0x0dd3u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0dd5u; break; }
        case 0x0dd5u: { /* mov byte ptr es:[di + 0x114], dl */
            ram[physical(c->es,(uint16_t)(c->di + 0x114))]=(uint8_t)((uint8_t)(c->dx)); c->ip=0x0ddau; break; }
        case 0x0ddau: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0dddu; break; }
        case 0x0dddu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0ddfu; break; }
        case 0x0ddfu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0de2u; break; }
        case 0x0de2u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0de4u; break; }
        case 0x0de4u: { /* mov dl, byte ptr es:[di + 0xa4] */
            c->dx=(uint16_t)((c->dx&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0xa4))])); c->ip=0x0de9u; break; }
        case 0x0de9u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0decu; break; }
        case 0x0decu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0deeu; break; }
        case 0x0deeu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0df1u; break; }
        case 0x0df1u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0df3u; break; }
        case 0x0df3u: { /* mov byte ptr es:[di + 0x116], dl */
            ram[physical(c->es,(uint16_t)(c->di + 0x116))]=(uint8_t)((uint8_t)(c->dx)); c->ip=0x0df8u; break; }
        case 0x0df8u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0dfbu; break; }
        case 0x0dfbu: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0dfdu; break; }
        case 0x0dfdu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e00u; break; }
        case 0x0e00u: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0e02u; break; }
        case 0x0e02u: { /* mov dl, byte ptr es:[di + 0xa6] */
            c->dx=(uint16_t)((c->dx&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0xa6))])); c->ip=0x0e07u; break; }
        case 0x0e07u: { /* mov al, byte ptr [bp - 1] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 1))])); c->ip=0x0e0au; break; }
        case 0x0e0au: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0e0cu; break; }
        case 0x0e0cu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e0fu; break; }
        case 0x0e0fu: { /* add di, ax */
            c->di=(uint16_t)(arithmetic(c,c->di,c->ax,0u,16u,0u)); c->ip=0x0e11u; break; }
        case 0x0e11u: { /* mov byte ptr es:[di + 0x118], dl */
            ram[physical(c->es,(uint16_t)(c->di + 0x118))]=(uint8_t)((uint8_t)(c->dx)); c->ip=0x0e16u; break; }
        case 0x0e16u: { /* cmp byte ptr [bp - 1], 2 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 1))],2u,0u,8u,1u); c->ip=0x0e1au; break; }
        case 0x0e1au: { /* jne 0xdb9 */
            c->ip=(!(c->flags&0x40u))?0xdb9u:0x0e1cu; break; }
        case 0x0e1cu: { /* mov byte ptr [bp - 2], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 2))]=(uint8_t)(0u); c->ip=0x0e20u; break; }
        case 0x0e20u: { /* jmp 0xe25 */
            c->ip=0xe25u; break; }
        case 0x0e22u: { /* inc byte ptr [bp - 2] */
            ram[physical(c->ss,(uint16_t)(c->bp - 2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],8u,0u)); c->ip=0x0e25u; break; }
        case 0x0e25u: { /* mov al, byte ptr [bp - 2] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 2))])); c->ip=0x0e28u; break; }
        case 0x0e28u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0e29u; break; }
        case 0x0e29u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0e2bu; break; }
        case 0x0e2bu: { /* mov byte ptr [bp + di - 0xd], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp + c->di - 0xd))]=(uint8_t)(0u); c->ip=0x0e2fu; break; }
        case 0x0e2fu: { /* cmp byte ptr [bp - 2], 4 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],4u,0u,8u,1u); c->ip=0x0e33u; break; }
        case 0x0e33u: { /* jne 0xe22 */
            c->ip=(!(c->flags&0x40u))?0xe22u:0x0e35u; break; }
        case 0x0e35u: { /* mov byte ptr [bp - 7], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 7))]=(uint8_t)(0u); c->ip=0x0e39u; break; }
        case 0x0e39u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e3cu; break; }
        case 0x0e3cu: { /* mov byte ptr es:[di + 0x101], 0 */
            ram[physical(c->es,(uint16_t)(c->di + 0x101))]=(uint8_t)(0u); c->ip=0x0e42u; break; }
        case 0x0e42u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e45u; break; }
        case 0x0e45u: { /* mov al, byte ptr es:[di + 0xa9] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0xa9))])); c->ip=0x0e4au; break; }
        case 0x0e4au: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e4du; break; }
        case 0x0e4du: { /* mov byte ptr es:[di + 0x111], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x111))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0e52u; break; }
        case 0x0e52u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e55u; break; }
        case 0x0e55u: { /* mov al, byte ptr es:[di + 0x72] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x72))])); c->ip=0x0e59u; break; }
        case 0x0e59u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e5cu; break; }
        case 0x0e5cu: { /* mov byte ptr es:[di + 0x11c], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x11c))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0e61u; break; }
        case 0x0e61u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e64u; break; }
        case 0x0e64u: { /* mov al, byte ptr es:[di + 0x2d] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x2d))])); c->ip=0x0e68u; break; }
        case 0x0e68u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e6bu; break; }
        case 0x0e6bu: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0e70u; break; }
        case 0x0e70u: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0e73u; break; }
        case 0x0e73u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0e76u; break; }
        case 0x0e76u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0e77u; break; }
        case 0x0e77u: { /* call 0x10e6 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0e7au);c->ip=0x10e6u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0e7au)return PR_RES00BA_TRANSFER; break; }
        case 0x0e7au: { /* mov byte ptr [bp - 0xd], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 0xd))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0e7du; break; }
        case 0x0e7du: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e80u; break; }
        case 0x0e80u: { /* mov ax, word ptr es:[di + 0xcc] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xcc))); c->ip=0x0e85u; break; }
        case 0x0e85u: { /* or ax, word ptr es:[di + 0xce] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->es,(uint16_t)(c->di + 0xce))),16u)); c->ip=0x0e8au; break; }
        case 0x0e8au: { /* jne 0xecd */
            c->ip=(!(c->flags&0x40u))?0xecdu:0x0e8cu; break; }
        case 0x0e8cu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0e8fu; break; }
        case 0x0e8fu: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0e92u; break; }
        case 0x0e92u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0e93u; break; }
        case 0x0e93u: { /* call 0x1282 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0e96u);c->ip=0x1282u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0e96u)return PR_RES00BA_TRANSFER; break; }
        case 0x0e96u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0e97u; break; }
        case 0x0e97u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0e99u; break; }
        case 0x0e99u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0e9cu; break; }
        case 0x0e9cu: { /* mov al, byte ptr es:[di + 0x110] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x110))])); c->ip=0x0ea1u; break; }
        case 0x0ea1u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0ea3u; break; }
        case 0x0ea3u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0ea5u; break; }
        case 0x0ea5u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ea8u; break; }
        case 0x0ea8u: { /* mov byte ptr es:[di + 0x110], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x110))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0eadu; break; }
        case 0x0eadu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0eb0u; break; }
        case 0x0eb0u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0eb3u; break; }
        case 0x0eb3u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0eb4u; break; }
        case 0x0eb4u: { /* call 0x1325 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0eb7u);c->ip=0x1325u; status=pr_res00ba_helpers_cpu(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0eb7u)return PR_RES00BA_TRANSFER; break; }
        case 0x0eb7u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0eb8u; break; }
        case 0x0eb8u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0ebau; break; }
        case 0x0ebau: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ebdu; break; }
        case 0x0ebdu: { /* mov al, byte ptr es:[di + 0x119] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x119))])); c->ip=0x0ec2u; break; }
        case 0x0ec2u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0ec3u; break; }
        case 0x0ec3u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0ec5u; break; }
        case 0x0ec5u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ec8u; break; }
        case 0x0ec8u: { /* mov byte ptr es:[di + 0x119], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x119))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0ecdu; break; }
        case 0x0ecdu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0ed0u; break; }
        case 0x0ed0u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0ed3u; break; }
        case 0x0ed3u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0ed4u; break; }
        case 0x0ed4u: { /* call 0 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0ed7u);c->ip=0x0000u; status=core(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0ed7u)return PR_RES00BA_TRANSFER; break; }
        case 0x0ed7u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0edau; break; }
        case 0x0edau: { /* mov ax, word ptr es:[di + 0xc8] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xc8))); c->ip=0x0edfu; break; }
        case 0x0edfu: { /* mov dx, word ptr es:[di + 0xca] */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0xca))); c->ip=0x0ee4u; break; }
        case 0x0ee4u: { /* mov word ptr [bp - 6], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 6),(uint16_t)(c->ax)); c->ip=0x0ee7u; break; }
        case 0x0ee7u: { /* mov word ptr [bp - 4], dx */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->dx)); c->ip=0x0eeau; break; }
        case 0x0eeau: { /* mov ax, word ptr [bp - 6] */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0eedu; break; }
        case 0x0eedu: { /* or ax, word ptr [bp - 4] */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->ss,(uint16_t)(c->bp - 4))),16u)); c->ip=0x0ef0u; break; }
        case 0x0ef0u: { /* je 0xf39 */
            c->ip=(c->flags&0x40u)?0xf39u:0x0ef2u; break; }
        case 0x0ef2u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ef5u; break; }
        case 0x0ef5u: { /* cmp byte ptr es:[di + 0x34], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x34))],0u,0u,8u,1u); c->ip=0x0efau; break; }
        case 0x0efau: { /* je 0xf26 */
            c->ip=(c->flags&0x40u)?0xf26u:0x0efcu; break; }
        case 0x0efcu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0effu; break; }
        case 0x0effu: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0f02u; break; }
        case 0x0f02u: { /* push word ptr [bp - 4] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0f05u; break; }
        case 0x0f05u: { /* push word ptr [bp - 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0f08u; break; }
        case 0x0f08u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0f09u; break; }
        case 0x0f09u: { /* call 0x1fb */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0f0cu);c->ip=0x01fbu; status=core(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0f0cu)return PR_RES00BA_TRANSFER; break; }
        case 0x0f0cu: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0f0fu; break; }
        case 0x0f0fu: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0f12u; break; }
        case 0x0f12u: { /* push word ptr [bp - 4] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x0f15u; break; }
        case 0x0f15u: { /* push word ptr [bp - 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 6))); c->ip=0x0f18u; break; }
        case 0x0f18u: { /* lea di, [bp - 0xd] */
            c->di=(uint16_t)((uint16_t)(c->bp - 0xd)); c->ip=0x0f1bu; break; }
        case 0x0f1bu: { /* push ss */
            push(ram,c,c->ss); c->ip=0x0f1cu; break; }
        case 0x0f1cu: { /* push di */
            push(ram,c,c->di); c->ip=0x0f1du; break; }
        case 0x0f1du: { /* lea di, [bp - 7] */
            c->di=(uint16_t)((uint16_t)(c->bp - 7)); c->ip=0x0f20u; break; }
        case 0x0f20u: { /* push ss */
            push(ram,c,c->ss); c->ip=0x0f21u; break; }
        case 0x0f21u: { /* push di */
            push(ram,c,c->di); c->ip=0x0f22u; break; }
        case 0x0f22u: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0f23u; break; }
        case 0x0f23u: { /* call 0x282 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0f26u);c->ip=0x0282u; status=core(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0f26u)return PR_RES00BA_TRANSFER; break; }
        case 0x0f26u: { /* les di, ptr [bp - 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f29u; break; }
        case 0x0f29u: { /* mov ax, word ptr es:[di + 0x2a] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x2a))); c->ip=0x0f2du; break; }
        case 0x0f2du: { /* mov dx, word ptr es:[di + 0x2c] */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x2c))); c->ip=0x0f31u; break; }
        case 0x0f31u: { /* mov word ptr [bp - 6], ax */
            write16(ram,c->ss,(uint16_t)(c->bp - 6),(uint16_t)(c->ax)); c->ip=0x0f34u; break; }
        case 0x0f34u: { /* mov word ptr [bp - 4], dx */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->dx)); c->ip=0x0f37u; break; }
        case 0x0f37u: { /* jmp 0xeea */
            c->ip=0xeeau; break; }
        case 0x0f39u: { /* cmp byte ptr [bp - 7], 0 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 7))],0u,0u,8u,1u); c->ip=0x0f3du; break; }
        case 0x0f3du: { /* je 0xf43 */
            c->ip=(c->flags&0x40u)?0xf43u:0x0f3fu; break; }
        case 0x0f3fu: { /* mov byte ptr [bp - 0xa], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 0xa))]=(uint8_t)(0u); c->ip=0x0f43u; break; }
        case 0x0f43u: { /* cmp byte ptr [bp - 8], 0 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 8))],0u,0u,8u,1u); c->ip=0x0f47u; break; }
        case 0x0f47u: { /* je 0xf84 */
            c->ip=(c->flags&0x40u)?0xf84u:0x0f49u; break; }
        case 0x0f49u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f4cu; break; }
        case 0x0f4cu: { /* cmp word ptr es:[di + 0x102], 0x1388 */
            (void)arithmetic(c,read16(ram,c->es,(uint16_t)(c->di + 0x102)),0x1388u,0u,16u,1u); c->ip=0x0f53u; break; }
        case 0x0f53u: { /* jae 0xf61 */
            c->ip=(!(c->flags&1u))?0xf61u:0x0f55u; break; }
        case 0x0f55u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f58u; break; }
        case 0x0f58u: { /* xor ax, ax */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16u)); c->ip=0x0f5au; break; }
        case 0x0f5au: { /* mov word ptr es:[di + 0x102], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(c->ax)); c->ip=0x0f5fu; break; }
        case 0x0f5fu: { /* jmp 0xf6b */
            c->ip=0xf6bu; break; }
        case 0x0f61u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f64u; break; }
        case 0x0f64u: { /* sub word ptr es:[di + 0x102], 0x1388 */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(arithmetic(c,read16(ram,c->es,(uint16_t)(c->di + 0x102)),0x1388u,0u,16u,1u))); c->ip=0x0f6bu; break; }
        case 0x0f6bu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f6eu; break; }
        case 0x0f6eu: { /* mov ax, word ptr es:[di + 0x102] */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x102))); c->ip=0x0f73u; break; }
        case 0x0f73u: { /* cmp ax, word ptr [0x4846] */
            (void)arithmetic(c,c->ax,read16(ram,c->ds,(uint16_t)(0x4846)),0u,16u,1u); c->ip=0x0f77u; break; }
        case 0x0f77u: { /* jae 0xf84 */
            c->ip=(!(c->flags&1u))?0xf84u:0x0f79u; break; }
        case 0x0f79u: { /* mov ax, word ptr [0x4846] */
            c->ax=(uint16_t)(read16(ram,c->ds,(uint16_t)(0x4846))); c->ip=0x0f7cu; break; }
        case 0x0f7cu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f7fu; break; }
        case 0x0f7fu: { /* mov word ptr es:[di + 0x102], ax */
            write16(ram,c->es,(uint16_t)(c->di + 0x102),(uint16_t)(c->ax)); c->ip=0x0f84u; break; }
        case 0x0f84u: { /* push word ptr [bp + 8] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 8))); c->ip=0x0f87u; break; }
        case 0x0f87u: { /* push word ptr [bp + 6] */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp + 6))); c->ip=0x0f8au; break; }
        case 0x0f8au: { /* push cs */
            push(ram,c,c->cs); c->ip=0x0f8bu; break; }
        case 0x0f8bu: { /* call 0x3a0 */
            uint16_t cs=c->cs; pr_res00ba_status status; push(ram,c,0x0f8eu);c->ip=0x03a0u; status=core(ram,size,c,budget); if(status!=PR_RES00BA_OK)return status; if(c->cs!=cs || c->ip!=0x0f8eu)return PR_RES00BA_TRANSFER; break; }
        case 0x0f8eu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0f91u; break; }
        case 0x0f91u: { /* mov al, byte ptr es:[di + 0x111] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x111))])); c->ip=0x0f96u; break; }
        case 0x0f96u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0f98u; break; }
        case 0x0f98u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0f9au; break; }
        case 0x0f9au: { /* mov al, byte ptr [bp - 9] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 9))])); c->ip=0x0f9du; break; }
        case 0x0f9du: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0f9eu; break; }
        case 0x0f9eu: { /* cmp ax, dx */
            (void)arithmetic(c,c->ax,c->dx,0u,16u,1u); c->ip=0x0fa0u; break; }
        case 0x0fa0u: { /* jge 0xfad */
            c->ip=(!(((c->flags>>7u)^(c->flags>>11u))&1u))?0xfadu:0x0fa2u; break; }
        case 0x0fa2u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0fa5u; break; }
        case 0x0fa5u: { /* mov al, byte ptr es:[di + 0x111] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x111))])); c->ip=0x0faau; break; }
        case 0x0faau: { /* mov byte ptr [bp - 9], al */
            ram[physical(c->ss,(uint16_t)(c->bp - 9))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0fadu; break; }
        case 0x0fadu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0fb0u; break; }
        case 0x0fb0u: { /* mov byte ptr es:[di + 0x111], 0 */
            ram[physical(c->es,(uint16_t)(c->di + 0x111))]=(uint8_t)(0u); c->ip=0x0fb6u; break; }
        case 0x0fb6u: { /* mov byte ptr [bp - 2], 0 */
            ram[physical(c->ss,(uint16_t)(c->bp - 2))]=(uint8_t)(0u); c->ip=0x0fbau; break; }
        case 0x0fbau: { /* jmp 0xfbf */
            c->ip=0xfbfu; break; }
        case 0x0fbcu: { /* inc byte ptr [bp - 2] */
            ram[physical(c->ss,(uint16_t)(c->bp - 2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],8u,0u)); c->ip=0x0fbfu; break; }
        case 0x0fbfu: { /* mov al, byte ptr [bp - 2] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 2))])); c->ip=0x0fc2u; break; }
        case 0x0fc2u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0fc3u; break; }
        case 0x0fc3u: { /* mov di, ax */
            c->di=(uint16_t)(c->ax); c->ip=0x0fc5u; break; }
        case 0x0fc5u: { /* mov al, byte ptr [bp + di - 0xd] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp + c->di - 0xd))])); c->ip=0x0fc8u; break; }
        case 0x0fc8u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0fc9u; break; }
        case 0x0fc9u: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0fcbu; break; }
        case 0x0fcbu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0fceu; break; }
        case 0x0fceu: { /* mov al, byte ptr es:[di + 0x111] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x111))])); c->ip=0x0fd3u; break; }
        case 0x0fd3u: { /* xor ah, ah */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x0fd5u; break; }
        case 0x0fd5u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0fd7u; break; }
        case 0x0fd7u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0fdau; break; }
        case 0x0fdau: { /* mov byte ptr es:[di + 0x111], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x111))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x0fdfu; break; }
        case 0x0fdfu: { /* cmp byte ptr [bp - 2], 4 */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 2))],4u,0u,8u,1u); c->ip=0x0fe3u; break; }
        case 0x0fe3u: { /* jne 0xfbc */
            c->ip=(!(c->flags&0x40u))?0xfbcu:0x0fe5u; break; }
        case 0x0fe5u: { /* mov al, byte ptr [bp - 0xa] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0xa))])); c->ip=0x0fe8u; break; }
        case 0x0fe8u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0fe9u; break; }
        case 0x0fe9u: { /* mov cx, ax */
            c->cx=(uint16_t)(c->ax); c->ip=0x0febu; break; }
        case 0x0febu: { /* mov al, byte ptr [bp - 0xb] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 0xb))])); c->ip=0x0feeu; break; }
        case 0x0feeu: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0fefu; break; }
        case 0x0fefu: { /* mov dx, ax */
            c->dx=(uint16_t)(c->ax); c->ip=0x0ff1u; break; }
        case 0x0ff1u: { /* mov al, byte ptr [bp - 9] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 9))])); c->ip=0x0ff4u; break; }
        case 0x0ff4u: { /* cwde  */
            c->ax=(uint16_t)((c->ax&0x80u)?(c->ax&255u)|0xff00u:c->ax&255u); c->ip=0x0ff5u; break; }
        case 0x0ff5u: { /* add ax, dx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->dx,0u,16u,0u)); c->ip=0x0ff7u; break; }
        case 0x0ff7u: { /* add ax, cx */
            c->ax=(uint16_t)(arithmetic(c,c->ax,c->cx,0u,16u,0u)); c->ip=0x0ff9u; break; }
        case 0x0ff9u: { /* dec ax */
            c->ax=(uint16_t)(step(c,c->ax,16u,1u)); c->ip=0x0ffau; break; }
        case 0x0ffau: { /* dec ax */
            c->ax=(uint16_t)(step(c,c->ax,16u,1u)); c->ip=0x0ffbu; break; }
        case 0x0ffbu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x0ffeu; break; }
        case 0x0ffeu: { /* mov byte ptr es:[di + 0x112], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x112))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x1003u; break; }
        case 0x1003u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1006u; break; }
        case 0x1006u: { /* cmp byte ptr es:[di + 0x98], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x98))],0u,0u,8u,1u); c->ip=0x100cu; break; }
        case 0x100cu: { /* jle 0x1029 */
            c->ip=((c->flags&0x40u)||(((c->flags>>7u)^(c->flags>>11u))&1u))?0x1029u:0x100eu; break; }
        case 0x100eu: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1011u; break; }
        case 0x1011u: { /* cmp byte ptr es:[di + 0x2e], 0 */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x2e))],0u,0u,8u,1u); c->ip=0x1016u; break; }
        case 0x1016u: { /* jle 0x1029 */
            c->ip=((c->flags&0x40u)||(((c->flags>>7u)^(c->flags>>11u))&1u))?0x1029u:0x1018u; break; }
        case 0x1018u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x101bu; break; }
        case 0x101bu: { /* mov al, byte ptr es:[di + 0x98] */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x98))])); c->ip=0x1020u; break; }
        case 0x1020u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1023u; break; }
        case 0x1023u: { /* mov byte ptr es:[di + 0x6b], al */
            ram[physical(c->es,(uint16_t)(c->di + 0x6b))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x1027u; break; }
        case 0x1027u: { /* jmp 0x1031 */
            c->ip=0x1031u; break; }
        case 0x1029u: { /* les di, ptr [bp + 6] */
            size_t p=physical(c->ss,(uint16_t)(c->bp + 6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x102cu; break; }
        case 0x102cu: { /* mov byte ptr es:[di + 0x6b], 1 */
            ram[physical(c->es,(uint16_t)(c->di + 0x6b))]=(uint8_t)(1u); c->ip=0x1031u; break; }
        case 0x1031u: { /* mov sp, bp */
            c->sp=(uint16_t)(c->bp); c->ip=0x1033u; break; }
        case 0x1033u: { /* pop bp */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x1034u; break; }
        case 0x1034u: { /* retf 4 */
            c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+4u);return PR_RES00BA_OK; break; }
        default: return PR_RES00BA_TRANSFER;
        }
    }
}
pr_res00ba_status pr_res00ba_core_cpu(uint8_t *ram,size_t size,
                                    pr_res00ba_cpu *c,size_t *budget)
{
    if(!ram || !c || !budget || size!=(1u<<20))
        return PR_RES00BA_ARGUMENT;
    if(c->ip!=0x0000u && c->ip!=0x01fbu && c->ip!=0x0282u &&
       c->ip!=0x03a0u && c->ip!=0x0bb8u)return PR_RES00BA_ARGUMENT;
    return core(ram,size,c,budget);
}
pr_res00ba_status pr_res00ba_0bb8_cpu(uint8_t *ram,size_t size,
                                    pr_res00ba_cpu *c,size_t *budget)
{
    if(!ram || !c || !budget || size!=(1u<<20) || c->ip!=0x0bb8u)
        return PR_RES00BA_ARGUMENT;
    return core(ram,size,c,budget);
}
