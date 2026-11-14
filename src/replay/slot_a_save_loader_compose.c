#include "poolrad/replay/slot_a_save_loader_compose.h"
#include "poolrad/replay/slot_a_save_loader_stub.h"
#include <string.h>
pr_slot_a_load_status pr_slot_a_save_loader_compose(uint8_t *ram,size_t size,
    const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
    uint16_t system_cs,size_t max_operands,size_t max_strings,
    pr_u1c_3171_cpu *cpu,pr_u25_0000_status *loader_status)
{
    pr_u1c_3171_cpu at; pr_u25_0000_cpu loader; pr_u25_0000_status status; size_t pos;
    _Static_assert(sizeof loader==sizeof at,"CPU layout size");
#define SAME_FIELD(f) _Static_assert(offsetof(pr_u25_0000_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME_FIELD(ax); SAME_FIELD(bx); SAME_FIELD(cx); SAME_FIELD(dx); SAME_FIELD(si); SAME_FIELD(di); SAME_FIELD(bp); SAME_FIELD(sp); SAME_FIELD(cs); SAME_FIELD(ip); SAME_FIELD(ds); SAME_FIELD(es); SAME_FIELD(ss); SAME_FIELD(flags);
#undef SAME_FIELD
    if (!ram || !stub || !body || !cpu || !loader_status || size!=0x100000u || body_size!=310u || !max_operands || !max_strings || cpu->sp<0x022au || cpu->sp>0xfffau) return PR_SLOT_A_LOAD_REFUSED;
    at=*cpu; if(pr_slot_a_save_loader_stub(ram,size,stub,stub_size,&at)) return PR_SLOT_A_LOAD_REFUSED;
    pos=((size_t)at.cs<<4)&0xfffffu;
    if(pos>size-body_size || memcmp(ram+pos,body,body_size)) return PR_SLOT_A_LOAD_REFUSED;
    memcpy(&loader,&at,sizeof loader);
    status=pr_u25_load_operands_0000(ram,size,&loader,system_cs,max_operands,max_strings);
    memcpy(cpu,&loader,sizeof loader); *loader_status=status; return PR_SLOT_A_LOAD_REACHED;
}
