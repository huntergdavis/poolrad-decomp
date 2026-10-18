/* Complete U3C6 entry 5; docs/re/U3C6_REBUILD_03A0.md.
 * CPU/memory primitives adapted from accepted U3C2 +0909. */
#include "poolrad/recovered/overlay_u3c6_rebuild_03a0.h"
#include "poolrad/recovered/overlay_u3c6_table_pair_0000.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK (PR_U3C6_03A0_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4u) + off) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at+1u)&MASK] << 8u));
}
static uint16_t read_word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram,physical(seg,off));
}
static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at=physical(seg,off);
    ram[at]=(uint8_t)value;ram[(at+1u)&MASK]=(uint8_t)(value >> 8u);
}
static void push(uint8_t *ram, pr_u3c6_03a0_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u3c6_03a0_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u3c6_03a0_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void cbw(pr_u3c6_03a0_cpu *c)
{
    c->ax=(uint16_t)((c->ax&0x80u) ? 0xFF00u|(c->ax&255u) : c->ax&255u);
}
static void result_flags(pr_u3c6_03a0_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u3c6_03a0_cpu *c, uint16_t left, uint16_t right,
                           unsigned carry, unsigned bits, unsigned subtract)
{
    uint32_t mask=bits==8u ? 255u : 65535u;
    uint16_t sign=bits==8u ? 0x80u : 0x8000u;
    uint32_t wide=subtract ? (uint32_t)left-right-carry : (uint32_t)left+right+carry;
    uint16_t result=(uint16_t)(wide&mask);
    c->flags&=(uint16_t)~0x08D5u;
    if (subtract ? (uint32_t)left<(uint32_t)right+carry : wide>mask)c->flags|=1u;
    if ((left^right^result)&0x10u)c->flags|=0x10u;
    if ((subtract ? (left^right)&(left^result) : (uint16_t)~(left^right)&(left^result))&sign)
        c->flags|=0x800u;
    result_flags(c,result,sign);return result;
}
static uint16_t shl(pr_u3c6_03a0_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* Intermediate AF follows the accepted U7F +0A01 normal/386 convention. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u3c6_03a0_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u3c6_03a0_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static void dl(pr_u3c6_03a0_cpu *c,uint8_t v) { c->dx=(uint16_t)((c->dx&0xFF00u)|v); }
static uint16_t logic(pr_u3c6_03a0_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint8_t increment(pr_u3c6_03a0_cpu *c,uint8_t v) { uint16_t cf=c->flags&1u;v=(uint8_t)arithmetic(c,v,1u,0u,8u,0u);c->flags=(uint16_t)((c->flags&~1u)|cf);return v; }

static void bl(pr_u3c6_03a0_cpu *c,uint8_t v) { c->bx=(uint16_t)((c->bx&0xFF00u)|v); }
static void multiply(pr_u3c6_03a0_cpu *c,uint16_t operand)
{
    uint32_t product=(uint32_t)c->ax*operand;
    c->ax=(uint16_t)product;c->dx=(uint16_t)(product>>16u);
    /* Undefined MUL flags never escape: ADD DI,CX defines all six
     * arithmetic flags before any store/call/return/budget boundary. */
    c->flags=(uint16_t)((c->flags&~0x0801u)|(c->dx?0x0801u:0u));
}

#define COPY_CPU(dst,src) do { (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; } while (0)
static int pair(uint8_t *ram,size_t size,pr_u3c6_03a0_cpu *c)
{
    pr_u3c6_0000_cpu inner;int status;COPY_CPU(&inner,c);
    status=(int)pr_u3c6_table_pair_0000(ram,size,&inner,PR_U3C6_0000_RETF_WRAPPED_SS);
    COPY_CPU(c,&inner);return status;
}
static int fill(uint8_t *ram,size_t size,pr_u3c6_03a0_cpu *c)
{
    pr_system_leaf_cpu inner;int status;COPY_CPU(&inner,c);
    status=(int)pr_system_fillchar_16a8(ram,size,&inner);COPY_CPU(c,&inner);return status;
}
#undef COPY_CPU
pr_u3c6_03a0_status pr_u3c6_rebuild_03a0(uint8_t *ram,size_t size,
    pr_u3c6_03a0_cpu *c,uint16_t system_cs,size_t max_slots,size_t max_cells)
{
    uint16_t code_segment;int status;
    if(ram==NULL || c==NULL || size!=PR_U3C6_03A0_RAM_SIZE ||
       c->ip!=0x03A0u || c->sp<0x001Cu || c->sp>0xFFFCu ||
       max_slots==0u || max_cells==0u)return PR_U3C6_03A0_ARGUMENT;
    code_segment=c->cs;
    for (;;) {
        if(c->cs!=code_segment)return PR_U3C6_03A0_TRANSFER;
        if(c->ip==0x03CFu) {
            if(max_slots==0u)return PR_U3C6_03A0_SLOT_LIMIT;
            --max_slots;
        }
        if(c->ip==0x03EDu) {
            if(max_cells==0u)return PR_U3C6_03A0_CELL_LIMIT;
            --max_cells;
        }
        switch(c->ip) {
        case 0x03A0: push(ram,c,c->bp);c->ip=0x03A1u;break;
        case 0x03A1: c->bp=c->sp;c->ip=0x03A3u;break;
        case 0x03A3: c->sp=arithmetic(c,c->sp,0x0006u,0u,16u,1u);c->ip=0x03A6u;break;
        case 0x03A6: c->di=0x60DBu;c->ip=0x03A9u;break;
        case 0x03A9: push(ram,c,c->ds);c->ip=0x03AAu;break;
        case 0x03AA: push(ram,c,c->di);c->ip=0x03ABu;break;
        case 0x03AB: c->ax=0x04E2u;c->ip=0x03AEu;break;
        case 0x03AE: push(ram,c,c->ax);c->ip=0x03AFu;break;
        case 0x03AF: al(c,(uint8_t)(0x0000u));c->ip=0x03B1u;break;
        case 0x03B1: push(ram,c,c->ax);c->ip=0x03B2u;break;
        case 0x03B2: push(ram,c,c->cs);push(ram,c,0x03B7u);c->cs=system_cs;c->ip=0x16A8u;status=fill(ram,size,c);if(status!=0)return PR_U3C6_03A0_CALLEE;if(c->cs!=code_segment || c->ip!=0x03B7u)return PR_U3C6_03A0_TRANSFER;break;
        case 0x03B7: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(24362))]));c->ip=0x03BAu;break;
        case 0x03BA: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)((uint8_t)c->ax);c->ip=0x03BDu;break;
        case 0x03BD: al(c,(uint8_t)(0x0001u));c->ip=0x03BFu;break;
        case 0x03BF: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-5))],0u,8u,1u);c->ip=0x03C2u;break;
        case 0x03C2: c->ip=((c->flags&0x41u)!=0u)?0x03C7u:0x03C4u;break;
        case 0x03C4: c->ip=0x04BAu;break;
        case 0x03C7: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x03CAu;break;
        case 0x03CA: c->ip=0x03CFu;break;
        case 0x03CC: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x03CFu;break;
        case 0x03CF: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x03D2u;break;
        case 0x03D2: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x03D4u;break;
        case 0x03D4: c->di=c->ax;c->ip=0x03D6u;break;
        case 0x03D6: c->di=shl(c,c->di);c->ip=0x03D8u;break;
        case 0x03D8: c->di=shl(c,c->di);c->ip=0x03DAu;break;
        case 0x03DA: (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(c->di+24362))],0x0000u,0u,8u,1u);c->ip=0x03DFu;break;
        case 0x03DF: c->ip=((c->flags&0x41u)==0u)?0x03E4u:0x03E1u;break;
        case 0x03E1: c->ip=0x04AFu;break;
        case 0x03E4: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x03E8u;break;
        case 0x03E8: c->ip=0x03EDu;break;
        case 0x03EA: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x03EDu;break;
        case 0x03ED: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x03F0u;break;
        case 0x03F0: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x03F2u;break;
        case 0x03F2: c->di=c->ax;c->ip=0x03F4u;break;
        case 0x03F4: c->di=shl(c,c->di);c->ip=0x03F6u;break;
        case 0x03F6: c->di=shl(c,c->di);c->ip=0x03F8u;break;
        case 0x03F8: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24362))]));c->ip=0x03FCu;break;
        case 0x03FC: push(ram,c,c->ax);c->ip=0x03FDu;break;
        case 0x03FD: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0400u;break;
        case 0x0400: push(ram,c,c->ax);c->ip=0x0401u;break;
        case 0x0401: c->di=(uint16_t)(c->bp+-3);c->ip=0x0404u;break;
        case 0x0404: push(ram,c,c->ss);c->ip=0x0405u;break;
        case 0x0405: push(ram,c,c->di);c->ip=0x0406u;break;
        case 0x0406: c->di=(uint16_t)(c->bp+-4);c->ip=0x0409u;break;
        case 0x0409: push(ram,c,c->ss);c->ip=0x040Au;break;
        case 0x040A: push(ram,c,c->di);c->ip=0x040Bu;break;
        case 0x040B: push(ram,c,c->cs);c->ip=0x040Cu;break;
        case 0x040C: push(ram,c,0x040Fu);c->ip=0u;status=pair(ram,size,c);if(status!=0)return PR_U3C6_03A0_CALLEE;if(c->cs!=code_segment || c->ip!=0x040Fu)return PR_U3C6_03A0_TRANSFER;break;
        case 0x040F: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x0411u;break;
        case 0x0411: c->ip=((c->flags&0x40u)!=0u)?0x0455u:0x0413u;break;
        case 0x0413: bl(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0416u;break;
        case 0x0416: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0419u;break;
        case 0x0419: cbw(c);c->ip=0x041Au;break;
        case 0x041A: c->dx=c->ax;c->ip=0x041Cu;break;
        case 0x041C: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x041Fu;break;
        case 0x041F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0421u;break;
        case 0x0421: c->di=c->ax;c->ip=0x0423u;break;
        case 0x0423: c->di=shl(c,c->di);c->ip=0x0425u;break;
        case 0x0425: c->di=shl(c,c->di);c->ip=0x0427u;break;
        case 0x0427: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24359))]));c->ip=0x042Bu;break;
        case 0x042B: cbw(c);c->ip=0x042Cu;break;
        case 0x042C: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x042Eu;break;
        case 0x042E: c->cx=c->ax;c->ip=0x0430u;break;
        case 0x0430: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0433u;break;
        case 0x0433: cbw(c);c->ip=0x0434u;break;
        case 0x0434: c->dx=c->ax;c->ip=0x0436u;break;
        case 0x0436: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0439u;break;
        case 0x0439: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x043Bu;break;
        case 0x043B: c->di=c->ax;c->ip=0x043Du;break;
        case 0x043D: c->di=shl(c,c->di);c->ip=0x043Fu;break;
        case 0x043F: c->di=shl(c,c->di);c->ip=0x0441u;break;
        case 0x0441: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24360))]));c->ip=0x0445u;break;
        case 0x0445: cbw(c);c->ip=0x0446u;break;
        case 0x0446: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x0448u;break;
        case 0x0448: c->dx=0x0032u;c->ip=0x044Bu;break;
        case 0x044B: multiply(c,c->dx);c->ip=0x044Du;break;
        case 0x044D: c->di=c->ax;c->ip=0x044Fu;break;
        case 0x044F: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0451u;break;
        case 0x0451: ram[physical(c->ds,(uint16_t)(c->di+24795))]=(uint8_t)((uint8_t)c->bx);c->ip=0x0455u;break;
        case 0x0455: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0003u,0u,8u,1u);c->ip=0x0459u;break;
        case 0x0459: c->ip=((c->flags&0x40u)==0u)?0x03EAu:0x045Bu;break;
        case 0x045B: c->di=read_word(ram,c->ds,0x6716u);c->es=read_word(ram,c->ds,0x6718u);c->ip=0x045Fu;break;
        case 0x045F: al(c,(uint8_t)(ram[physical(c->es,(uint16_t)(c->di+2))]));c->ip=0x0463u;break;
        case 0x0463: cbw(c);c->ip=0x0464u;break;
        case 0x0464: c->dx=c->ax;c->ip=0x0466u;break;
        case 0x0466: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0469u;break;
        case 0x0469: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x046Bu;break;
        case 0x046B: c->di=c->ax;c->ip=0x046Du;break;
        case 0x046D: c->di=shl(c,c->di);c->ip=0x046Fu;break;
        case 0x046F: c->di=shl(c,c->di);c->ip=0x0471u;break;
        case 0x0471: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24359))]));c->ip=0x0475u;break;
        case 0x0475: cbw(c);c->ip=0x0476u;break;
        case 0x0476: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x0478u;break;
        case 0x0478: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x047Au;break;
        case 0x047A: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x047Du;break;
        case 0x047D: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x047Fu;break;
        case 0x047F: c->di=c->ax;c->ip=0x0481u;break;
        case 0x0481: ram[physical(c->ds,(uint16_t)(c->di+24650))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0485u;break;
        case 0x0485: c->di=read_word(ram,c->ds,0x6716u);c->es=read_word(ram,c->ds,0x6718u);c->ip=0x0489u;break;
        case 0x0489: al(c,(uint8_t)(ram[physical(c->es,(uint16_t)(c->di+3))]));c->ip=0x048Du;break;
        case 0x048D: cbw(c);c->ip=0x048Eu;break;
        case 0x048E: c->dx=c->ax;c->ip=0x0490u;break;
        case 0x0490: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0493u;break;
        case 0x0493: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0495u;break;
        case 0x0495: c->di=c->ax;c->ip=0x0497u;break;
        case 0x0497: c->di=shl(c,c->di);c->ip=0x0499u;break;
        case 0x0499: c->di=shl(c,c->di);c->ip=0x049Bu;break;
        case 0x049B: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24360))]));c->ip=0x049Fu;break;
        case 0x049F: cbw(c);c->ip=0x04A0u;break;
        case 0x04A0: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x04A2u;break;
        case 0x04A2: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x04A4u;break;
        case 0x04A4: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x04A7u;break;
        case 0x04A7: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x04A9u;break;
        case 0x04A9: c->di=c->ax;c->ip=0x04ABu;break;
        case 0x04AB: ram[physical(c->ds,(uint16_t)(c->di+24722))]=(uint8_t)((uint8_t)c->dx);c->ip=0x04AFu;break;
        case 0x04AF: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x04B2u;break;
        case 0x04B2: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-5))],0u,8u,1u);c->ip=0x04B5u;break;
        case 0x04B5: c->ip=((c->flags&0x40u)!=0u)?0x04BAu:0x04B7u;break;
        case 0x04B7: c->ip=0x03CCu;break;
        case 0x04BA: c->sp=c->bp;c->ip=0x04BCu;break;
        case 0x04BC: c->bp=pop(ram,c);c->ip=0x04BDu;break;
        case 0x04BD: retf(ram,c,0u);return PR_U3C6_03A0_OK;
        default:return PR_U3C6_03A0_TRANSFER;
        }
    }
}
