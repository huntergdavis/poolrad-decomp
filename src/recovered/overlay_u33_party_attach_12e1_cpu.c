/* U33 party attachment [12E1,1403). Original order and real callee frames.
 * Resident arithmetic primitives reused from the composed +0BB8 model. */
#include "poolrad/recovered/overlay_u33_party_attach_12e1_cpu.h"
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
static void push(uint8_t *r,pr_u33_12e1_cpu *c,uint16_t v) {
    c->sp=(uint16_t)(c->sp-2u);write16(r,c->ss,c->sp,v);
}
static uint16_t pop(const uint8_t *r,pr_u33_12e1_cpu *c) {
    uint16_t v=read16(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);return v;
}
static unsigned parity(uint8_t v) {
    v^=(uint8_t)(v>>4u);v^=(uint8_t)(v>>2u);v^=(uint8_t)(v>>1u);
    return (v&1u)==0u;
}
static void szp(pr_u33_12e1_cpu *c,uint16_t v,unsigned bits) {
    if(parity((uint8_t)v))c->flags|=4u;
    if(v==0u)c->flags|=0x40u;
    if(v&(bits==8u?0x80u:0x8000u))c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u33_12e1_cpu *c,uint16_t a,uint16_t b,
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
static uint16_t logic(pr_u33_12e1_cpu *c,uint16_t v,unsigned bits) {
    c->flags&=(uint16_t)~0x08d5u;szp(c,v,bits);return v;
}
static uint16_t step(pr_u33_12e1_cpu *c,uint16_t v,unsigned bits,unsigned sub) {
    uint16_t carry=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,sub);c->flags=(uint16_t)((c->flags&~1u)|carry);
    return v;
}

#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define COPY_CPU(d,s) do { \
(d).ax=(s).ax; (d).bx=(s).bx; (d).cx=(s).cx; (d).dx=(s).dx; \
(d).si=(s).si; (d).di=(s).di; (d).bp=(s).bp; (d).sp=(s).sp; \
(d).cs=(s).cs; (d).ip=(s).ip; (d).ds=(s).ds; (d).es=(s).es; \
(d).ss=(s).ss; (d).flags=(s).flags; } while(0)
static uint16_t shr1(pr_u33_12e1_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v>>1u);
    c->flags&=(uint16_t)~0x08d5u;c->flags|=0x10u;
    if(v&1u)c->flags|=1u;
    if(v>0x8000u)c->flags|=0x800u; /* pinned SHRw count 1: OF = (var1 > 8000h) */
    szp(c,result,16u);return result;
}
/* DOSBox-X normal/386 IDIVW: these architecturally undefined bits
 * are deterministic in the supported native profile. The original
 * XOR AH; ADD AX,4; CWD; MOV CX,8 restricts dividend to 4..259. */
static void divide8(pr_u33_12e1_cpu *c) {
    uint16_t dividend=c->ax,quotient=(uint16_t)(dividend/8u);
    uint16_t remainder=(uint16_t)(dividend%8u),flags=0u;
    if(remainder==0u && (quotient&1u))flags|=0x40u;
    if((remainder&3u)==1u || (remainder&3u)==2u)flags|=1u;
    if(parity((uint8_t)(remainder^(remainder>>8u))) ^
       parity((uint8_t)(quotient^(quotient>>8u))) ^ 1u)flags|=4u;
    c->ax=quotient;c->dx=remainder;
    c->flags=(uint16_t)((c->flags&~0x08d5u)|flags);
}
static int binding(const uint8_t *ram,uint16_t cs,uint16_t ip,uint16_t offset,uint16_t seg) {
    return ram[physical(cs,ip)]==0x9au &&
        read16(ram,cs,(uint16_t)(ip+1u))==offset &&
        read16(ram,cs,(uint16_t)(ip+3u))==seg;
}
pr_u33_12e1_status pr_u33_party_attach_12e1_cpu(
    uint8_t *ram,size_t size,pr_u33_12e1_cpu *c,
    uint16_t resident_cs,uint16_t system_cs,size_t *budget,
    unsigned max_heap_steps,int *getmem_status)
{
    if(!ram || !c || !budget || size!=(1u<<20) || c->ip!=0x12e1u ||
       c->sp<0x0100u || c->sp>0xfff0u)return PR_U33_12E1_ARGUMENT;
    if(!binding(ram,c->cs,0x1309u,0x0bb8u,resident_cs) ||
       !binding(ram,c->cs,0x1315u,0x023fu,system_cs) ||
       !binding(ram,c->cs,0x1338u,0x16a8u,system_cs))
        return PR_U33_12E1_ARGUMENT;
    if(getmem_status)*getmem_status=-1;
    for (;;) {
        if(!*budget)return PR_U33_12E1_LIMIT;
        --*budget;
        switch(c->ip) {
        case 0x12e1u: { /* push */
            push(ram,c,c->bp); c->ip=0x12e2u; break; }
        case 0x12e2u: { /* mov */
            c->bp=(uint16_t)(c->sp); c->ip=0x12e4u; break; }
        case 0x12e4u: { /* sub */
            c->sp=(uint16_t)(arithmetic(c,c->sp,6u,0u,16u,1u)); c->ip=0x12e7u; break; }
        case 0x12e7u: { /* mov */
            c->ax=(uint16_t)(read16(ram,c->ds,(uint16_t)(0x5d96))); c->ip=0x12eau; break; }
        case 0x12eau: { /* mov */
            c->dx=(uint16_t)(read16(ram,c->ds,(uint16_t)(0x5d98))); c->ip=0x12eeu; break; }
        case 0x12eeu: { /* mov */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); c->ip=0x12f1u; break; }
        case 0x12f1u: { /* mov */
            write16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); c->ip=0x12f4u; break; }
        case 0x12f4u: { /* mov */
            ram[physical(c->ss,(uint16_t)(c->bp - 5))]=(uint8_t)(0u); c->ip=0x12f8u; break; }
        case 0x12f8u: { /* mov */
            c->ax=(uint16_t)(read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x12fbu; break; }
        case 0x12fbu: { /* or */
            c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|read16(ram,c->ss,(uint16_t)(c->bp - 2))),16u)); c->ip=0x12feu; break; }
        case 0x12feu: { /* jne */
            c->ip=(!(c->flags&0x40u))?0x1303u:0x1300u; break; }
        case 0x1300u: { /* jmp */
            c->ip=0x13ffu; break; }
        case 0x1303u: { /* push */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 2))); c->ip=0x1306u; break; }
        case 0x1306u: { /* push */
            push(ram,c,read16(ram,c->ss,(uint16_t)(c->bp - 4))); c->ip=0x1309u; break; }
        case 0x1309u: { /* lcall */
            uint16_t caller=c->cs; push(ram,c,caller);push(ram,c,0x130eu);c->cs=resident_cs;c->ip=0x0bb8u; pr_res00ba_status status=pr_res00ba_0bb8_cpu(ram,size,c,budget); if(status==PR_RES00BA_LIMIT)return PR_U33_12E1_LIMIT; if(status==PR_RES00BA_TRANSFER)return PR_U33_12E1_TRANSFER; if(status!=PR_RES00BA_OK)return PR_U33_12E1_CALLEE;if(c->cs!=caller || c->ip!=0x130eu)return PR_U33_12E1_TRANSFER; break; }
        case 0x130eu: { /* inc */
            ram[physical(c->ss,(uint16_t)(c->bp - 5))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp - 5))],8u,0u)); c->ip=0x1311u; break; }
        case 0x1311u: { /* mov */
            c->ax=(uint16_t)(0x16u); c->ip=0x1314u; break; }
        case 0x1314u: { /* push */
            push(ram,c,c->ax); c->ip=0x1315u; break; }
        case 0x1315u: { /* lcall */
            uint16_t caller=c->cs; push(ram,c,caller);push(ram,c,0x131au);c->cs=system_cs;c->ip=0x023fu; pr_system_getmem_cpu leaf; COPY_CPU(leaf,*c); pr_system_getmem_status status=pr_system_getmem_023f(ram,size,&leaf,max_heap_steps); COPY_CPU(*c,leaf); if(getmem_status)*getmem_status=(int)status; if(status==PR_SYSTEM_GETMEM_TRANSFER)return PR_U33_12E1_TRANSFER; if(status!=PR_SYSTEM_GETMEM_RETURNED)return PR_U33_12E1_CALLEE;if(c->cs!=caller || c->ip!=0x131au)return PR_U33_12E1_TRANSFER; break; }
        case 0x131au: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x131du; break; }
        case 0x131du: { /* mov */
            write16(ram,c->es,(uint16_t)(c->di + 0x108),(uint16_t)(c->ax)); c->ip=0x1322u; break; }
        case 0x1322u: { /* mov */
            write16(ram,c->es,(uint16_t)(c->di + 0x10a),(uint16_t)(c->dx)); c->ip=0x1327u; break; }
        case 0x1327u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x132au; break; }
        case 0x132au: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x132fu; break; }
        case 0x132fu: { /* push */
            push(ram,c,c->es); c->ip=0x1330u; break; }
        case 0x1330u: { /* push */
            push(ram,c,c->di); c->ip=0x1331u; break; }
        case 0x1331u: { /* mov */
            c->ax=(uint16_t)(0x16u); c->ip=0x1334u; break; }
        case 0x1334u: { /* push */
            push(ram,c,c->ax); c->ip=0x1335u; break; }
        case 0x1335u: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(0u)); c->ip=0x1337u; break; }
        case 0x1337u: { /* push */
            push(ram,c,c->ax); c->ip=0x1338u; break; }
        case 0x1338u: { /* lcall */
            uint16_t caller=c->cs; push(ram,c,caller);push(ram,c,0x133du);c->cs=system_cs;c->ip=0x16a8u; pr_system_leaf_cpu leaf; COPY_CPU(leaf,*c); pr_system_leaf_status status=pr_system_fillchar_16a8(ram,size,&leaf); COPY_CPU(*c,leaf); if(status!=PR_SYSTEM_LEAF_RETURNED)return PR_U33_12E1_CALLEE;if(c->cs!=caller || c->ip!=0x133du)return PR_U33_12E1_TRANSFER; break; }
        case 0x133du: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp - 5))])); c->ip=0x1340u; break; }
        case 0x1340u: { /* xor */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x1342u; break; }
        case 0x1342u: { /* les */
            size_t p=physical(c->ds,(uint16_t)(0x49d6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1346u; break; }
        case 0x1346u: { /* cmp */
            (void)arithmetic(c,c->ax,read16(ram,c->es,(uint16_t)(c->di + 0x67c)),0u,16u,1u); c->ip=0x134bu; break; }
        case 0x134bu: { /* jbe */
            c->ip=(c->flags&0x41u)?0x135au:0x134du; break; }
        case 0x134du: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1350u; break; }
        case 0x1350u: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1355u; break; }
        case 0x1355u: { /* mov */
            ram[physical(c->es,(uint16_t)(c->di + 0x13))]=(uint8_t)(1u); c->ip=0x135au; break; }
        case 0x135au: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(0x6aaf))])); c->ip=0x135du; break; }
        case 0x135du: { /* xor */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x135fu; break; }
        case 0x135fu: { /* shr */
            c->ax=shr1(c,c->ax); c->ip=0x1361u; break; }
        case 0x1361u: { /* mov */
            c->di=(uint16_t)(c->ax); c->ip=0x1363u; break; }
        case 0x1363u: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di + 0x2f2))])); c->ip=0x1367u; break; }
        case 0x1367u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x136au; break; }
        case 0x136au: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x136fu; break; }
        case 0x136fu: { /* mov */
            ram[physical(c->es,(uint16_t)(c->di + 9))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x1373u; break; }
        case 0x1373u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1376u; break; }
        case 0x1376u: { /* cmp */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x10e))],1u,0u,8u,1u); c->ip=0x137cu; break; }
        case 0x137cu: { /* jne */
            c->ip=(!(c->flags&0x40u))?0x13a2u:0x137eu; break; }
        case 0x137eu: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1381u; break; }
        case 0x1381u: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1386u; break; }
        case 0x1386u: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 9))])); c->ip=0x138au; break; }
        case 0x138au: { /* xor */
            c->ax=(uint16_t)((c->ax&0x00ffu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u)); c->ip=0x138cu; break; }
        case 0x138cu: { /* add */
            c->ax=(uint16_t)(arithmetic(c,c->ax,4u,0u,16u,0u)); c->ip=0x138fu; break; }
        case 0x138fu: { /* cdq */
            c->dx=(uint16_t)((c->ax&0x8000u)?0xffffu:0u); c->ip=0x1390u; break; }
        case 0x1390u: { /* mov */
            c->cx=(uint16_t)(8u); c->ip=0x1393u; break; }
        case 0x1393u: { /* idiv */
            divide8(c); c->ip=0x1395u; break; }
        case 0x1395u: { /* xchg */
            uint16_t v=c->dx; c->dx=(uint16_t)(c->ax); c->ax=(uint16_t)(v); c->ip=0x1396u; break; }
        case 0x1396u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x1399u; break; }
        case 0x1399u: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x139eu; break; }
        case 0x139eu: { /* mov */
            ram[physical(c->es,(uint16_t)(c->di + 9))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x13a2u; break; }
        case 0x13a2u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13a5u; break; }
        case 0x13a5u: { /* mov */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(ram[physical(c->es,(uint16_t)(c->di + 0x84))])); c->ip=0x13aau; break; }
        case 0x13aau: { /* and */
            c->ax=(uint16_t)((c->ax&0xff00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x7fu),8u))); c->ip=0x13acu; break; }
        case 0x13acu: { /* mov */
            ram[physical(c->ss,(uint16_t)(c->bp - 6))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x13afu; break; }
        case 0x13afu: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13b2u; break; }
        case 0x13b2u: { /* cmp */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x10e))],0u,0u,8u,1u); c->ip=0x13b8u; break; }
        case 0x13b8u: { /* jne */
            c->ip=(!(c->flags&0x40u))?0x13e9u:0x13bau; break; }
        case 0x13bau: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13bdu; break; }
        case 0x13bdu: { /* les */
            size_t p=physical(c->es,(uint16_t)(c->di + 0x108)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13c2u; break; }
        case 0x13c2u: { /* cmp */
            (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di + 0x13))],1u,0u,8u,1u); c->ip=0x13c7u; break; }
        case 0x13c7u: { /* jne */
            c->ip=(!(c->flags&0x40u))?0x13e9u:0x13c9u; break; }
        case 0x13c9u: { /* cmp */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 6))],0u,0u,8u,1u); c->ip=0x13cdu; break; }
        case 0x13cdu: { /* je */
            c->ip=(c->flags&0x40u)?0x13d5u:0x13cfu; break; }
        case 0x13cfu: { /* cmp */
            (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp - 6))],0x66u,0u,8u,1u); c->ip=0x13d3u; break; }
        case 0x13d3u: { /* jbe */
            c->ip=(c->flags&0x41u)?0x13e9u:0x13d5u; break; }
        case 0x13d5u: { /* les */
            size_t p=physical(c->ds,(uint16_t)(0x49d6)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13d9u; break; }
        case 0x13d9u: { /* mov */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x58c))); c->ip=0x13deu; break; }
        case 0x13deu: { /* add */
            c->ax=(uint16_t)(arithmetic(c,c->ax,0x80u,0u,16u,0u)); c->ip=0x13e1u; break; }
        case 0x13e1u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13e4u; break; }
        case 0x13e4u: { /* mov */
            ram[physical(c->es,(uint16_t)(c->di + 0x84))]=(uint8_t)((uint8_t)(c->ax)); c->ip=0x13e9u; break; }
        case 0x13e9u: { /* les */
            size_t p=physical(c->ss,(uint16_t)(c->bp - 4)); uint16_t v=word_at(ram,p); c->es=word_at(ram,(p+2u)&MASK); c->di=v; c->ip=0x13ecu; break; }
        case 0x13ecu: { /* mov */
            c->ax=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x104))); c->ip=0x13f1u; break; }
        case 0x13f1u: { /* mov */
            c->dx=(uint16_t)(read16(ram,c->es,(uint16_t)(c->di + 0x106))); c->ip=0x13f6u; break; }
        case 0x13f6u: { /* mov */
            write16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); c->ip=0x13f9u; break; }
        case 0x13f9u: { /* mov */
            write16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); c->ip=0x13fcu; break; }
        case 0x13fcu: { /* jmp */
            c->ip=0x12f8u; break; }
        case 0x13ffu: { /* mov */
            c->sp=(uint16_t)(c->bp); c->ip=0x1401u; break; }
        case 0x1401u: { /* pop */
            c->bp=(uint16_t)(pop(ram,c)); c->ip=0x1402u; break; }
        case 0x1402u: { /* retf */
            c->ip=pop(ram,c);c->cs=pop(ram,c);return PR_U33_12E1_RETURNED; break; }
        default: return PR_U33_12E1_TRANSFER;
        }
    }
}
