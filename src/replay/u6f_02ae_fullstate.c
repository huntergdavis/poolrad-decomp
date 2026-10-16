#include "poolrad/replay/u6f_02ae_fullstate.h"
#include "poolrad/replay/slot_a_saved_copy_1965.h"
#include "poolrad/replay/slot_a_string_0425_abi.h"
#include "poolrad/replay/system_b98_abi.h"

static uint8_t *span(pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, size_t length)
{
    size_t p = ((size_t)seg << 4) + off;
    if ((size_t)off + length > 0x10000u || p > s->ram_size ||
        length > s->ram_size - p) return NULL;
    return s->ram + p;
}

static void put(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

static uint8_t projected(const uint8_t *ram,
                         const pr_slot_a_directory_write *w,
                         size_t count, size_t p)
{
    while (count) {
        --count;
        if (w[count].linear == p) return w[count].new_byte;
    }
    return ram[p];
}

static uint16_t projected_word(const uint8_t *ram,
                               const pr_slot_a_directory_write *w,
                               size_t count, size_t p)
{
    return (uint16_t)(projected(ram,w,count,p) |
        ((uint16_t)projected(ram,w,count,p+1u) << 8));
}

static pr_u6f_02ae_full_status ui_event(
    pr_slot_a_directory_state *s, pr_u6f_02ae_ui_service service,
    void *context, pr_slot_a_directory_write *writes, size_t capacity)
{
    pr_slot_a_directory_cpu after;
    pr_u6f_02ae_full_status result;
    size_t count=0, i, frame;
    if (!service) return PR_U6F_02AE_FULL_BLOCKED_UI;
    if (!span(s,s->cpu.ss,s->cpu.sp,4u))
        return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
    frame=((size_t)s->cpu.ss << 4)+s->cpu.sp;
    after=s->cpu;
    result=service(context,&s->cpu,s->ram,s->ram_size,
                   &after,writes,capacity,&count);
    if (result==PR_U6F_02AE_FULL_BLOCKED_UI) return result;
    if (result!=PR_U6F_02AE_FULL_STEPPED || count>capacity ||
        after.cs!=0x2a8eu || after.ip!=0x031fu ||
        after.ss!=s->cpu.ss || after.sp!=(uint16_t)(s->cpu.sp+10u))
        return PR_U6F_02AE_FULL_BAD_EVENT;
    for (i=0; i<count; ++i)
        if (writes[i].linear>=s->ram_size ||
            writes[i].old_byte!=projected(s->ram,writes,i,writes[i].linear))
            return PR_U6F_02AE_FULL_BAD_EVENT;
    if (projected_word(s->ram,writes,count,frame)!=after.ip ||
        projected_word(s->ram,writes,count,frame+2u)!=after.cs)
        return PR_U6F_02AE_FULL_BAD_EVENT;
    for (i=0; i<count; ++i) s->ram[writes[i].linear]=writes[i].new_byte;
    s->cpu=after;
    return PR_U6F_02AE_FULL_STEPPED;
}

pr_u6f_02ae_full_status pr_u6f_02ae_fullstate_step(
    pr_slot_a_directory_state *s,
    pr_slot_a_directory_service dos, void *dos_context,
    pr_u6f_02ae_ui_service ui, void *ui_context,
    pr_slot_a_directory_write *writes, size_t capacity)
{
    pr_slot_a_directory_cpu *c;
    uint8_t *p;
    uint16_t next=0, value=0, target=0;
    unsigned push=0;
    if (!s || (capacity && !writes)) return PR_U6F_02AE_FULL_ARGUMENT;
    if (!s->ram || s->ram_size>0x100000u)
        return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs==0x0f2du && c->ip==0x0bf5u)
        return ui_event(s,ui,ui_context,writes,capacity);
    if (c->cs==0x131cu && c->ip>=0x0b0bu && c->ip<=0x0b22u) {
        pr_slot_a_saved_copy_report report;
        pr_slot_a_saved_copy_status r=pr_slot_a_saved_copy_1965_step(s,&report);
        if (r==PR_SLOT_A_SAVED_COPY_STEPPED) return PR_U6F_02AE_FULL_STEPPED;
        return r==PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY ?
            PR_U6F_02AE_FULL_BLOCKED_MEMORY : PR_U6F_02AE_FULL_BLOCKED_PC;
    }
    if (c->cs==0x131cu && c->ip>=0x0c3bu && c->ip<=0x0c4cu) {
        pr_slot_a_string_0425_report report;
        pr_slot_a_string_0425_status r=pr_slot_a_string_0425_step(s,&report);
        if (r==PR_SLOT_A_STRING_0425_STEPPED) return PR_U6F_02AE_FULL_STEPPED;
        return r==PR_SLOT_A_STRING_0425_BLOCKED_MEMORY ?
            PR_U6F_02AE_FULL_BLOCKED_MEMORY : PR_U6F_02AE_FULL_BLOCKED_PC;
    }
    if (c->cs==0x131cu && c->ip>=0x0b98u && c->ip<=0x0bc1u) {
        pr_system_b98_status r=pr_system_b98_step(s);
        if (r==PR_SYSTEM_B98_STEPPED) return PR_U6F_02AE_FULL_STEPPED;
        return r==PR_SYSTEM_B98_BLOCKED_MEMORY ?
            PR_U6F_02AE_FULL_BLOCKED_MEMORY : PR_U6F_02AE_FULL_BLOCKED_PC;
    }
    if (c->cs==0x2a8eu && c->ip>=0x02e9u && c->ip<0x031fu) {
        switch (c->ip) {
        case 0x02e9: c->ax=(uint16_t)((c->ax&0xff00u)|0x0eu); next=0x02eb; break;
        case 0x02eb: value=c->ax; push=1; next=0x02ec; break;
        case 0x02ec: c->di=(uint16_t)(c->bp-0x202u); next=0x02f0; break;
        case 0x02f0: value=c->ss; push=1; next=0x02f1; break;
        case 0x02f1: value=c->di; push=1; next=0x02f2; break;
        case 0x02f2: c->di=0x029au; next=0x02f5; break;
        case 0x02f5: value=c->cs; push=1; next=0x02f6; break;
        case 0x02f6: value=c->di; push=1; next=0x02f7; break;
        case 0x02f7: target=0x0b0bu; next=0x02fc; break;
        case 0x02fc: c->di=(uint16_t)(c->bp-0x102u); next=0x0300; break;
        case 0x0300: value=c->ss; push=1; next=0x0301; break;
        case 0x0301: value=c->di; push=1; next=0x0302; break;
        case 0x0302:
            p=span(s,c->ds,0x537bu,1u);
            if (!p) return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
            c->ax=(uint16_t)((c->ax&0xff00u)|*p); next=0x0305; break;
        case 0x0305: value=c->ax; push=1; next=0x0306; break;
        case 0x0306: target=0x0c3bu; next=0x030b; break;
        case 0x030b: target=0x0b98u; next=0x0310; break;
        case 0x0310: c->di=0x02acu; next=0x0313; break;
        case 0x0313: value=c->cs; push=1; next=0x0314; break;
        case 0x0314: value=c->di; push=1; next=0x0315; break;
        case 0x0315: target=0x0b98u; next=0x031a; break;
        case 0x031a: target=0x0bf5u; next=0x031f; break;
        default: return PR_U6F_02AE_FULL_BLOCKED_PC;
        }
        if (push) {
            p=span(s,c->ss,(uint16_t)(c->sp-2u),2u);
            if (!p) return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
            put(p,value); c->sp=(uint16_t)(c->sp-2u);
        } else if (target) {
            p=span(s,c->ss,(uint16_t)(c->sp-4u),4u);
            if (!p) return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
            put(p+2u,c->cs); put(p,next); c->sp=(uint16_t)(c->sp-4u);
            c->cs=c->ip==0x031au ? 0x0f2du : 0x131cu;
            next=target;
        }
        c->ip=next;
        return PR_U6F_02AE_FULL_STEPPED;
    }
    switch (pr_u6f_02ae_step(s,dos,dos_context,writes,capacity)) {
    case PR_U6F_02AE_STEPPED: return PR_U6F_02AE_FULL_STEPPED;
    case PR_U6F_02AE_RETURNED: return PR_U6F_02AE_FULL_RETURNED;
    case PR_U6F_02AE_BLOCKED_DOS_SERVICE: return PR_U6F_02AE_FULL_BLOCKED_DOS;
    case PR_U6F_02AE_BLOCKED_MEMORY: return PR_U6F_02AE_FULL_BLOCKED_MEMORY;
    case PR_U6F_02AE_BAD_EVENT: return PR_U6F_02AE_FULL_BAD_EVENT;
    case PR_U6F_02AE_ARGUMENT: return PR_U6F_02AE_FULL_ARGUMENT;
    default: return PR_U6F_02AE_FULL_BLOCKED_PC;
    }
}
