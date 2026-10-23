/* Complete U86 +059C; docs/re/U86_LIST_BUILD_059C_CPU.md.
 * Ordered memory/flag primitives reuse accepted U86 +051C/+044E. */
#include "poolrad/recovered/overlay_u86_list_build_059c_cpu.h"
#include <string.h>
#define MASK (PR_U86_059C_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u86_059c_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u86_059c_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u86_059c_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u86_059c_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u86_059c_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram,pr_u86_059c_cpu *c,unsigned cleanup)
{
    size_t p=physical(c->ss,c->sp);
    c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u+cleanup);
}
static void ah(pr_u86_059c_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u86_059c_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint16_t step(pr_u86_059c_cpu *c,uint16_t v,unsigned bits,unsigned subtract)
{
    uint16_t cf=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,subtract);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
static uint16_t shift1(pr_u86_059c_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v<<1u);(void)logic(c,result,16u);c->flags|=0x10u; /* pinned: SHL sets AF */
    if(v&0x8000u)c->flags|=1u;
    if((v^result)&0x8000u)c->flags|=0x800u;
    return result;
}


static void clear_diagnostic(pr_u86_059c_diagnostic *d) {
    *d=(pr_u86_059c_diagnostic){-1,-1,{-1,-1},{-1,-1,-1,-1},{-1,-1,{-1,-1,-1,-1}}};
}
static pr_u86_059c_status call(uint8_t *ram,size_t size,pr_u86_059c_cpu *c,
    const pr_u86_059c_limits *l,pr_u86_059c_diagnostic *d,unsigned kind)
{
    uint16_t caller=c->cs,site=c->ip,next=(uint16_t)(site+3u);
    uint16_t target=kind==0u?0x028Du:kind==1u?0x044Eu:0x051Cu;
    if(ram[physical(caller,site)]!=0xE8u ||
       read_word(ram,caller,(uint16_t)(site+1u))!=(uint16_t)(target-next))
        return PR_U86_059C_CODE;
    push(ram,c,next);c->ip=target;clear_diagnostic(d);d->callee=(int)kind;
    if(kind==0u) {
        pr_u86_028d_cpu inner;
        _Static_assert(sizeof inner==sizeof *c,"append CPU layout");
        memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u86_string_node_028d(ram,size,&inner,l->gate.sidecar.max_list_visits,
                                             l->gate.sidecar.max_heap_steps,&d->append);
        memcpy(c,&inner,sizeof inner);
    } else if(kind==1u) {
        d->status=(int)pr_u86_sidecar_list_044e(ram,size,c,&l->gate.sidecar,&d->sidecar);
    } else {
        d->status=(int)pr_u86_sidecar_gate_051c_cpu(ram,size,c,&l->gate,&d->gate);
    }
    if(d->status!=0)return PR_U86_059C_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U86_059C_TRANSFER;
    return PR_U86_059C_OK;
}
pr_u86_059c_status pr_u86_list_build_059c_cpu(uint8_t *ram,size_t size,pr_u86_059c_cpu *c,
    const pr_u86_059c_limits *l,pr_u86_059c_diagnostic *d)
{
    pr_u86_059c_diagnostic local;pr_u86_059c_status status;
    size_t at,history,items;uint16_t caller;
    if(d==NULL)d=&local;
    clear_diagnostic(d);
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U86_059C_RAM_SIZE || c->ip!=0x059Cu ||
       c->sp<0x016Au || c->sp>0xFFFAu || l->max_history_visits==0u || l->max_item_visits==0u ||
       l->gate.max_clear_visits==0u || l->gate.max_record_visits==0u ||
       l->gate.sidecar.max_caller_visits==0u || l->gate.sidecar.max_list_visits==0u)
        return PR_U86_059C_ARGUMENT;
    history=l->max_history_visits;items=l->max_item_visits;caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U86_059C_TRANSFER;
        if(c->ip==0x05B7u) { if(history==0u)return PR_U86_059C_LOOP_LIMIT;--history; }
        if(c->ip==0x05D9u || c->ip==0x0627u || c->ip==0x0663u || c->ip==0x06BBu) {
            if(items==0u)return PR_U86_059C_LOOP_LIMIT;
            --items;
        }
        switch(c->ip) {
        case 0x059C: push(ram,c,c->bp);c->ip=0x059Du;break;
        case 0x059D: c->bp=c->sp;c->ip=0x059Fu;break;
        case 0x059F: c->sp=arithmetic(c,c->sp,0x0002u,0u,16u,1u);c->ip=0x05A2u;break;
        case 0x05A2: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(0x0000u);c->ip=0x05A6u;break;
        case 0x05A6: c->ax=logic(c,(c->ax)^(c->ax),16u);c->ip=0x05A8u;break;
        case 0x05A8: put_word(ram,c->ds,(uint16_t)(18278),c->ax);c->ip=0x05ABu;break;
        case 0x05AB: put_word(ram,c->ds,(uint16_t)(18280),c->ax);c->ip=0x05AEu;break;
        case 0x05AE: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x05B2u;break;
        case 0x05B2: c->ip=0x05B7u;break;
        case 0x05B4: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],8u,0u));c->ip=0x05B7u;break;
        case 0x05B7: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x05BAu;break;
        case 0x05BA: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x05BCu;break;
        case 0x05BC: c->di=c->ax;c->ip=0x05BEu;break;
        case 0x05BE: ram[physical(c->ds,(uint16_t)(c->di+18282))]=(uint8_t)(0x0000u);c->ip=0x05C3u;break;
        case 0x05C3: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0014u,0u,8u,1u);c->ip=0x05C7u;break;
        case 0x05C7: c->ip=((c->flags&0x40u)==0)?0x05B4u:0x05C9u;break;
        case 0x05C9: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x05CCu;break;
        case 0x05CC: (void)arithmetic(c,(uint8_t)c->ax,0x0000u,0u,8u,1u);c->ip=0x05CEu;break;
        case 0x05CE: c->ip=((c->flags&0x40u)==0)?0x061Au:0x05D0u;break;
        case 0x05D0: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x05D4u;break;
        case 0x05D4: c->ip=0x05D9u;break;
        case 0x05D6: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],8u,0u));c->ip=0x05D9u;break;
        case 0x05D9: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x05DCu;break;
        case 0x05DC: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x05DEu;break;
        case 0x05DE: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x05E2u;break;
        case 0x05E2: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x05E4u;break;
        case 0x05E4: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+23))],0x0000u,0u,8u,1u);c->ip=0x05E9u;break;
        case 0x05E9: c->ip=((c->flags&0x41u)!=0)?0x0611u:0x05EBu;break;
        case 0x05EB: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x05EEu;break;
        case 0x05EE: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x05F0u;break;
        case 0x05F0: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x05F4u;break;
        case 0x05F4: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x05F6u;break;
        case 0x05F6: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+23))],0x0080u,0u,8u,1u);c->ip=0x05FBu;break;
        case 0x05FB: c->ip=((c->flags&1u)==0)?0x0611u:0x05FDu;break;
        case 0x05FD: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0600u;break;
        case 0x0600: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x0602u;break;
        case 0x0602: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0606u;break;
        case 0x0606: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0608u;break;
        case 0x0608: al(c,(uint8_t)(ram[physical(c->es,(uint16_t)(c->di+23))]));c->ip=0x060Cu;break;
        case 0x060C: push(ram,c,c->ax);c->ip=0x060Du;break;
        case 0x060D: push(ram,c,c->cs);c->ip=0x060Eu;break;
        case 0x060E: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x0611: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0014u,0u,8u,1u);c->ip=0x0615u;break;
        case 0x0615: c->ip=((c->flags&0x40u)==0)?0x05D6u:0x0617u;break;
        case 0x0617: c->ip=0x0713u;break;
        case 0x061A: (void)arithmetic(c,(uint8_t)c->ax,0x0005u,0u,8u,1u);c->ip=0x061Cu;break;
        case 0x061C: c->ip=((c->flags&0x40u)==0)?0x0656u:0x061Eu;break;
        case 0x061E: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x0622u;break;
        case 0x0622: c->ip=0x0627u;break;
        case 0x0624: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],8u,0u));c->ip=0x0627u;break;
        case 0x0627: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x062Au;break;
        case 0x062A: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x062Cu;break;
        case 0x062C: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0630u;break;
        case 0x0630: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0632u;break;
        case 0x0632: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+23))],0x007Fu,0u,8u,1u);c->ip=0x0637u;break;
        case 0x0637: c->ip=((c->flags&0x41u)!=0)?0x064Du:0x0639u;break;
        case 0x0639: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x063Cu;break;
        case 0x063C: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x063Eu;break;
        case 0x063E: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0642u;break;
        case 0x0642: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0644u;break;
        case 0x0644: al(c,(uint8_t)(ram[physical(c->es,(uint16_t)(c->di+23))]));c->ip=0x0648u;break;
        case 0x0648: push(ram,c,c->ax);c->ip=0x0649u;break;
        case 0x0649: push(ram,c,c->cs);c->ip=0x064Au;break;
        case 0x064A: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x064D: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0014u,0u,8u,1u);c->ip=0x0651u;break;
        case 0x0651: c->ip=((c->flags&0x40u)==0)?0x0624u:0x0653u;break;
        case 0x0653: c->ip=0x0713u;break;
        case 0x0656: (void)arithmetic(c,(uint8_t)c->ax,0x0001u,0u,8u,1u);c->ip=0x0658u;break;
        case 0x0658: c->ip=((c->flags&0x40u)==0)?0x0686u:0x065Au;break;
        case 0x065A: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0001u);c->ip=0x065Eu;break;
        case 0x065E: c->ip=0x0663u;break;
        case 0x0660: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],8u,0u));c->ip=0x0663u;break;
        case 0x0663: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0666u;break;
        case 0x0666: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x0668u;break;
        case 0x0668: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x066Cu;break;
        case 0x066C: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x066Eu;break;
        case 0x066E: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+50))],0x0000u,0u,8u,1u);c->ip=0x0673u;break;
        case 0x0673: c->ip=((c->flags&0x40u)!=0)?0x067Du:0x0675u;break;
        case 0x0675: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0678u;break;
        case 0x0678: push(ram,c,c->ax);c->ip=0x0679u;break;
        case 0x0679: push(ram,c,c->cs);c->ip=0x067Au;break;
        case 0x067A: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x067D: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0037u,0u,8u,1u);c->ip=0x0681u;break;
        case 0x0681: c->ip=((c->flags&0x40u)==0)?0x0660u:0x0683u;break;
        case 0x0683: c->ip=0x0713u;break;
        case 0x0686: (void)arithmetic(c,(uint8_t)c->ax,0x0002u,0u,8u,1u);c->ip=0x0688u;break;
        case 0x0688: c->ip=((c->flags&0x40u)==0)?0x0694u:0x068Au;break;
        case 0x068A: al(c,(uint8_t)(0x0000u));c->ip=0x068Cu;break;
        case 0x068C: push(ram,c,c->ax);c->ip=0x068Du;break;
        case 0x068D: push(ram,c,c->cs);c->ip=0x068Eu;break;
        case 0x068E: status=call(ram,size,c,l,d,1);if(status!=0)return status;break;
        case 0x0691: c->ip=0x0713u;break;
        case 0x0694: (void)arithmetic(c,(uint8_t)c->ax,0x0003u,0u,8u,1u);c->ip=0x0696u;break;
        case 0x0696: c->ip=((c->flags&0x40u)==0)?0x06A1u:0x0698u;break;
        case 0x0698: al(c,(uint8_t)(0x0000u));c->ip=0x069Au;break;
        case 0x069A: push(ram,c,c->ax);c->ip=0x069Bu;break;
        case 0x069B: push(ram,c,c->cs);c->ip=0x069Cu;break;
        case 0x069C: status=call(ram,size,c,l,d,2);if(status!=0)return status;break;
        case 0x069F: c->ip=0x0713u;break;
        case 0x06A1: (void)arithmetic(c,(uint8_t)c->ax,0x0006u,0u,8u,1u);c->ip=0x06A3u;break;
        case 0x06A3: c->ip=((c->flags&0x40u)==0)?0x06AEu:0x06A5u;break;
        case 0x06A5: al(c,(uint8_t)(0x0001u));c->ip=0x06A7u;break;
        case 0x06A7: push(ram,c,c->ax);c->ip=0x06A8u;break;
        case 0x06A8: push(ram,c,c->cs);c->ip=0x06A9u;break;
        case 0x06A9: status=call(ram,size,c,l,d,2);if(status!=0)return status;break;
        case 0x06AC: c->ip=0x0713u;break;
        case 0x06AE: (void)arithmetic(c,(uint8_t)c->ax,0x0004u,0u,8u,1u);c->ip=0x06B0u;break;
        case 0x06B0: c->ip=((c->flags&0x40u)==0)?0x0713u:0x06B2u;break;
        case 0x06B2: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0001u);c->ip=0x06B6u;break;
        case 0x06B6: c->ip=0x06BBu;break;
        case 0x06B8: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],8u,0u));c->ip=0x06BBu;break;
        case 0x06BB: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x06BEu;break;
        case 0x06BE: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x06C0u;break;
        case 0x06C0: c->di=c->ax;c->ip=0x06C2u;break;
        case 0x06C2: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x0004u));c->ip=0x06C4u;break;
        case 0x06C4: c->di=(uint16_t)(c->di<<4u);/* CL=4; flags overwritten before observable boundary. */c->ip=0x06C6u;break;
        case 0x06C6: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+12795))]));c->ip=0x06CAu;break;
        case 0x06CA: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x06CBu;break;
        case 0x06CB: c->dx=c->ax;c->ip=0x06CDu;break;
        case 0x06CD: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x06D0u;break;
        case 0x06D0: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x06D2u;break;
        case 0x06D2: c->di=c->ax;c->ip=0x06D4u;break;
        case 0x06D4: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x0004u));c->ip=0x06D6u;break;
        case 0x06D6: c->di=(uint16_t)(c->di<<4u);/* CL=4; flags overwritten before observable boundary. */c->ip=0x06D8u;break;
        case 0x06D8: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+12794))]));c->ip=0x06DCu;break;
        case 0x06DC: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x06DDu;break;
        case 0x06DD: c->si=c->ax;c->ip=0x06DFu;break;
        case 0x06DF: c->ax=shift1(c,c->ax);c->ip=0x06E1u;break;
        case 0x06E1: c->ax=arithmetic(c,c->ax,c->si,0u,16u,0u);c->ip=0x06E3u;break;
        case 0x06E3: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x06E7u;break;
        case 0x06E7: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x06E9u;break;
        case 0x06E9: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x06EBu;break;
        case 0x06EB: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+177))],0x0000u,0u,8u,1u);c->ip=0x06F1u;break;
        case 0x06F1: c->ip=((c->flags&0x41u)!=0)?0x070Du:0x06F3u;break;
        case 0x06F3: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x06F6u;break;
        case 0x06F6: ah(c,(uint8_t)(logic(c,((uint8_t)(c->ax>>8u))^((uint8_t)(c->ax>>8u)),8u)));c->ip=0x06F8u;break;
        case 0x06F8: at=physical(c->ds,(uint16_t)(23954));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x06FCu;break;
        case 0x06FC: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x06FEu;break;
        case 0x06FE: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+50))],0x0000u,0u,8u,1u);c->ip=0x0703u;break;
        case 0x0703: c->ip=((c->flags&0x40u)==0)?0x070Du:0x0705u;break;
        case 0x0705: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0708u;break;
        case 0x0708: push(ram,c,c->ax);c->ip=0x0709u;break;
        case 0x0709: push(ram,c,c->cs);c->ip=0x070Au;break;
        case 0x070A: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x070D: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0038u,0u,8u,1u);c->ip=0x0711u;break;
        case 0x0711: c->ip=((c->flags&0x40u)==0)?0x06B8u:0x0713u;break;
        case 0x0713: c->ax=read_word(ram,c->ds,(uint16_t)(18278));c->ip=0x0716u;break;
        case 0x0716: c->ax=logic(c,(c->ax)|(read_word(ram,c->ds,(uint16_t)(18280))),16u);c->ip=0x071Au;break;
        case 0x071A: c->ip=((c->flags&0x40u)!=0)?0x0720u:0x071Cu;break;
        case 0x071C: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(0x0001u);c->ip=0x0720u;break;
        case 0x0720: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0723u;break;
        case 0x0723: c->sp=c->bp;c->ip=0x0725u;break;
        case 0x0725: c->bp=pop(ram,c);c->ip=0x0726u;break;
        case 0x0726: retf(ram,c,2);return PR_U86_059C_OK;
        default:return PR_U86_059C_TRANSFER;
        }
    }
}
