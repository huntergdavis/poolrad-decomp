#include "poolrad/replay/slot_a_first_save_compose.h"
#include "poolrad/replay/slot_a_save_store_args.h"
pr_slot_a_first_save_status pr_slot_a_first_save_compose(uint8_t *ram,size_t size,
 const pr_slot_a_first_save_refs *r,pr_u1c_3171_cpu *cpu,pr_slot_a_first_save_report *report)
{
    uint16_t caller;
    if(!ram||!r||!cpu||!report||!r->slot_call||!r->slot_stub||!r->slot_body||!r->word_body||!r->store_args||!r->store_call||!r->store_stub||!r->store_body||!r->store_hook||!r->classifier) return PR_FIRST_SAVE_ARGUMENT;
    report->slot=PR_U25_SLOT_VALUE_ARGUMENT; report->store=PR_U25_STORE_WORD_ARGUMENT; caller=cpu->cs;
    if(size!=0x100000u || ram[(((size_t)cpu->ds<<4)+0x7006u)&0xfffffu]!=0u) return PR_FIRST_SAVE_SLOT_REFUSED;
    if(pr_slot_a_save_slot_value(ram,size,r->slot_call,8,r->slot_stub,5,r->slot_body,128,r->word_body,36,cpu,&report->slot)) return PR_FIRST_SAVE_SLOT_REFUSED;
    if(report->slot!=PR_U25_SLOT_VALUE_OK||cpu->cs!=caller||cpu->ip!=0x02bfu) return PR_FIRST_SAVE_SLOT_STOP;
    if(pr_slot_a_save_store_args(ram,size,r->store_args,9,cpu)) return PR_FIRST_SAVE_ARGS_REFUSED;
    if(pr_slot_a_first_save_store(ram,size,r->store_call,5,r->store_stub,5,r->store_body,405,r->store_hook,515,r->classifier,91,cpu,&report->store)) return PR_FIRST_SAVE_STORE_REFUSED;
    if(report->store!=PR_U25_STORE_WORD_OK||cpu->cs!=caller||cpu->ip!=0x02cdu) return PR_FIRST_SAVE_STORE_STOP;
    return PR_FIRST_SAVE_AT_02CD;
}
