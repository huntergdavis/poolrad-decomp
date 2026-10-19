#include "poolrad/recovered/resident_character_target_select.h"

#include <stddef.h>

static bool read_pair(const pr_rec_target_select_io *io,
                      pr_rec_select_far_ptr base, uint16_t field,
                      pr_rec_select_far_ptr *value)
{
    return io->read_far_word(io->context, base.segment,
                             (uint16_t)(base.offset + field),
                             &value->offset) &&
           io->read_far_word(io->context, base.segment,
                             (uint16_t)(base.offset + field + 2u),
                             &value->segment);
}

static bool write_pair(const pr_rec_target_select_io *io,
                       pr_rec_select_far_ptr slot,
                       pr_rec_select_far_ptr value)
{
    return io->write_far_word(io->context, slot.segment, slot.offset,
                              value.offset) &&
           io->write_far_word(io->context, slot.segment,
                              (uint16_t)(slot.offset + 2u), value.segment);
}

pr_rec_target_select_status pr_rec_res_00ba_2e8b(
    const pr_rec_target_select_io *io, pr_rec_select_far_ptr character,
    pr_rec_select_far_ptr output_slot, uint16_t *ax)
{
    pr_rec_select_far_ptr link, candidate, final_pointer;
    uint8_t flags = 0, index;
    uint16_t combined;

    if (io == NULL || io->read_far_word == NULL ||
        io->read_far_byte == NULL || io->read_global_byte == NULL ||
        io->write_far_word == NULL || ax == NULL)
        return PR_REC_TARGET_SELECT_ARGUMENT;

    /* 0x3A35..0x3A54: save the character link before clearing the
     * caller's output slot, preserving alias effects and word order. */
    if (!read_pair(io, character, 0xccu, &link) ||
        !write_pair(io, output_slot, (pr_rec_select_far_ptr){0, 0}))
        return PR_REC_TARGET_SELECT_MEMORY;

    if ((uint16_t)(link.offset | link.segment) != 0) {
        /* 0x3A60..0x3A73: linked byte +0x2E selects a 16-byte DS entry. */
        if (!io->read_far_byte(io->context, link.segment,
                               (uint16_t)(link.offset + 0x2eu), &index) ||
            !io->read_global_byte(io->context,
                                  (uint16_t)(0x5590u + ((uint16_t)index << 4)),
                                  &flags))
            return PR_REC_TARGET_SELECT_MEMORY;

        /* 0x3A76..0x3A8F: bit 0x10 selects the original link. */
        if ((flags & 0x10u) != 0 && !write_pair(io, output_slot, link))
            return PR_REC_TARGET_SELECT_MEMORY;

        if ((flags & 0x08u) != 0) {
            /* 0x3A98..0x3AB8: bit 0x01 replaces it with character +0xF8. */
            if ((flags & 0x01u) != 0) {
                if (!read_pair(io, character, 0xf8u, &candidate) ||
                    !write_pair(io, output_slot, candidate))
                    return PR_REC_TARGET_SELECT_MEMORY;
            }
            /* 0x3AB8..0x3AD8: bit 0x80 has the last-write priority. */
            if ((flags & 0x80u) != 0) {
                if (!read_pair(io, character, 0xfcu, &candidate) ||
                    !write_pair(io, output_slot, candidate))
                    return PR_REC_TARGET_SELECT_MEMORY;
            }
        }
    }

    /* 0x3AD8..0x3AF7: reread both output words. AL is true for a
     * non-null output or exactly flags 0x0A. AH is the high byte of OR. */
    if (!read_pair(io, output_slot, 0, &final_pointer))
        return PR_REC_TARGET_SELECT_MEMORY;
    combined = (uint16_t)(final_pointer.offset | final_pointer.segment);
    *ax = (uint16_t)((combined & 0xff00u) |
                     (combined != 0 || flags == 0x0au ? 1u : 0u));
    return PR_REC_TARGET_SELECT_OK;
}

/* Complete original instructions; docs/re/U38_TARGET_SELECTOR_CPU.md. */

#define MASK (PR_TARGET_CPU_RAM_SIZE - 1u)
static size_t phys(uint16_t s,uint16_t o) {return (((size_t)s<<4)+o)&MASK;}
static uint16_t word_at(const uint8_t *m,size_t p) {return (uint16_t)(m[p&MASK]|((uint16_t)m[(p+1u)&MASK]<<8));}
static uint16_t get(const uint8_t *m,uint16_t s,uint16_t o,unsigned bits)
{size_t p=phys(s,o);return bits==8?m[p]:word_at(m,p);}
static void put(uint8_t *m,uint16_t s,uint16_t o,unsigned bits,uint16_t v)
{size_t p=phys(s,o);m[p]=(uint8_t)v;if(bits==16)m[(p+1u)&MASK]=(uint8_t)(v>>8);}
static void push(uint8_t *m,pr_target_cpu *c,uint16_t v)
{c->sp=(uint16_t)(c->sp-2u);put(m,c->ss,c->sp,16,v);}
static uint16_t logic(pr_target_cpu *c,uint16_t v,unsigned bits)
{
    unsigned p=v&255u;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags&=(uint16_t)~0x08D5u;
    if(!(p&1u))c->flags|=4u;
    if(!v)c->flags|=64u;
    if(v&(bits==8?128u:32768u))c->flags|=128u;
    return v;
}
static uint16_t sub(pr_target_cpu *c,uint16_t a,uint16_t b,unsigned bits)
{
    uint16_t r=(uint16_t)((a-b)&(bits==8?255u:65535u));
    (void)logic(c,r,bits);
    if(a<b)c->flags|=1u;
    if((a^b^r)&16u)c->flags|=16u;
    if((a^b)&(a^r)&(bits==8?128u:32768u))c->flags|=0x800u;
    return r;
}

pr_target_status pr_res_target_select_2e8b_cpu(uint8_t *m,size_t size,pr_target_cpu *c)
{
    size_t at;
    if(m==NULL||c==NULL||size!=PR_TARGET_CPU_RAM_SIZE||c->ip!=0x2E8Bu||c->sp<0x0008u||c->sp>0xFFF4u) return PR_TARGET_ARGUMENT;
    for(;;) switch(c->ip) {
    case 0x2E8Bu: /* push bp */
        push(m,c,c->bp);
        c->ip=0x2E8Cu;
        break;
    case 0x2E8Cu: /* mov bp, sp */
        c->bp=(uint16_t)(c->sp);
        c->ip=0x2E8Eu;
        break;
    case 0x2E8Eu: /* sub sp, 6 */
        c->sp=(uint16_t)(sub(c,c->sp,0x6u,16));
        c->ip=0x2E91u;
        break;
    case 0x2E91u: /* mov byte ptr [bp - 1], 0 */
        put(m,c->ss,(uint16_t)(c->bp - 0x1u),8,0x0u);
        c->ip=0x2E95u;
        break;
    case 0x2E95u: /* les di, ptr [bp + 0xa] */
        at=phys(c->ss,(uint16_t)(c->bp + 0xAu));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2E98u;
        break;
    case 0x2E98u: /* mov ax, word ptr es:[di + 0xcc] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCCu),16));
        c->ip=0x2E9Du;
        break;
    case 0x2E9Du: /* mov dx, word ptr es:[di + 0xce] */
        c->dx=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xCEu),16));
        c->ip=0x2EA2u;
        break;
    case 0x2EA2u: /* mov word ptr [bp - 5], ax */
        put(m,c->ss,(uint16_t)(c->bp - 0x5u),16,c->ax);
        c->ip=0x2EA5u;
        break;
    case 0x2EA5u: /* mov word ptr [bp - 3], dx */
        put(m,c->ss,(uint16_t)(c->bp - 0x3u),16,c->dx);
        c->ip=0x2EA8u;
        break;
    case 0x2EA8u: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2EABu;
        break;
    case 0x2EABu: /* xor ax, ax */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax^c->ax),16));
        c->ip=0x2EADu;
        break;
    case 0x2EADu: /* mov word ptr es:[di], ax */
        put(m,c->es,(uint16_t)(c->di + 0x0u),16,c->ax);
        c->ip=0x2EB0u;
        break;
    case 0x2EB0u: /* mov word ptr es:[di + 2], ax */
        put(m,c->es,(uint16_t)(c->di + 0x2u),16,c->ax);
        c->ip=0x2EB4u;
        break;
    case 0x2EB4u: /* mov byte ptr [bp - 6], 0 */
        put(m,c->ss,(uint16_t)(c->bp - 0x6u),8,0x0u);
        c->ip=0x2EB8u;
        break;
    case 0x2EB8u: /* mov ax, word ptr [bp - 5] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x5u),16));
        c->ip=0x2EBBu;
        break;
    case 0x2EBBu: /* or ax, word ptr [bp - 3] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->ss,(uint16_t)(c->bp - 0x3u),16)),16));
        c->ip=0x2EBEu;
        break;
    case 0x2EBEu: /* je 0x2f38 */
        c->ip=(c->flags&64u)?0x2F38u:0x2EC0u;
        break;
    case 0x2EC0u: /* les di, ptr [bp - 5] */
        at=phys(c->ss,(uint16_t)(c->bp - 0x5u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2EC3u;
        break;
    case 0x2EC3u: /* mov al, byte ptr es:[di + 0x2e] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->es,(uint16_t)(c->di + 0x2Eu),8)));
        c->ip=0x2EC7u;
        break;
    case 0x2EC7u: /* xor ah, ah */
        c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax >> 8)^(uint8_t)(c->ax >> 8)),8))<<8));
        c->ip=0x2EC9u;
        break;
    case 0x2EC9u: /* mov di, ax */
        c->di=(uint16_t)(c->ax);
        c->ip=0x2ECBu;
        break;
    case 0x2ECBu: /* mov cl, 4 */
        c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x4u));
        c->ip=0x2ECDu;
        break;
    case 0x2ECDu: /* shl di, cl */
        /* CL=4; flags are overwritten by subsequent AND/OR before observation. */
        c->di=(uint16_t)(c->di<<4);
        c->ip=0x2ECFu;
        break;
    case 0x2ECFu: /* mov al, byte ptr [di + 0x5590] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ds,(uint16_t)(c->di + 0x5590u),8)));
        c->ip=0x2ED3u;
        break;
    case 0x2ED3u: /* mov byte ptr [bp - 6], al */
        put(m,c->ss,(uint16_t)(c->bp - 0x6u),8,(uint8_t)(c->ax));
        c->ip=0x2ED6u;
        break;
    case 0x2ED6u: /* mov al, byte ptr [bp - 6] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x6u),8)));
        c->ip=0x2ED9u;
        break;
    case 0x2ED9u: /* and al, 0x10 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x10u),8)));
        c->ip=0x2EDBu;
        break;
    case 0x2EDBu: /* or al, al */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8)));
        c->ip=0x2EDDu;
        break;
    case 0x2EDDu: /* je 0x2eef */
        c->ip=(c->flags&64u)?0x2EEFu:0x2EDFu;
        break;
    case 0x2EDFu: /* mov ax, word ptr [bp - 5] */
        c->ax=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x5u),16));
        c->ip=0x2EE2u;
        break;
    case 0x2EE2u: /* mov dx, word ptr [bp - 3] */
        c->dx=(uint16_t)(get(m,c->ss,(uint16_t)(c->bp - 0x3u),16));
        c->ip=0x2EE5u;
        break;
    case 0x2EE5u: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2EE8u;
        break;
    case 0x2EE8u: /* mov word ptr es:[di], ax */
        put(m,c->es,(uint16_t)(c->di + 0x0u),16,c->ax);
        c->ip=0x2EEBu;
        break;
    case 0x2EEBu: /* mov word ptr es:[di + 2], dx */
        put(m,c->es,(uint16_t)(c->di + 0x2u),16,c->dx);
        c->ip=0x2EEFu;
        break;
    case 0x2EEFu: /* mov al, byte ptr [bp - 6] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x6u),8)));
        c->ip=0x2EF2u;
        break;
    case 0x2EF2u: /* and al, 8 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x8u),8)));
        c->ip=0x2EF4u;
        break;
    case 0x2EF4u: /* or al, al */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8)));
        c->ip=0x2EF6u;
        break;
    case 0x2EF6u: /* je 0x2f38 */
        c->ip=(c->flags&64u)?0x2F38u:0x2EF8u;
        break;
    case 0x2EF8u: /* mov al, byte ptr [bp - 6] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x6u),8)));
        c->ip=0x2EFBu;
        break;
    case 0x2EFBu: /* and al, 1 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x1u),8)));
        c->ip=0x2EFDu;
        break;
    case 0x2EFDu: /* or al, al */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8)));
        c->ip=0x2EFFu;
        break;
    case 0x2EFFu: /* je 0x2f18 */
        c->ip=(c->flags&64u)?0x2F18u:0x2F01u;
        break;
    case 0x2F01u: /* les di, ptr [bp + 0xa] */
        at=phys(c->ss,(uint16_t)(c->bp + 0xAu));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2F04u;
        break;
    case 0x2F04u: /* mov ax, word ptr es:[di + 0xf8] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xF8u),16));
        c->ip=0x2F09u;
        break;
    case 0x2F09u: /* mov dx, word ptr es:[di + 0xfa] */
        c->dx=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xFAu),16));
        c->ip=0x2F0Eu;
        break;
    case 0x2F0Eu: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2F11u;
        break;
    case 0x2F11u: /* mov word ptr es:[di], ax */
        put(m,c->es,(uint16_t)(c->di + 0x0u),16,c->ax);
        c->ip=0x2F14u;
        break;
    case 0x2F14u: /* mov word ptr es:[di + 2], dx */
        put(m,c->es,(uint16_t)(c->di + 0x2u),16,c->dx);
        c->ip=0x2F18u;
        break;
    case 0x2F18u: /* mov al, byte ptr [bp - 6] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x6u),8)));
        c->ip=0x2F1Bu;
        break;
    case 0x2F1Bu: /* and al, 0x80 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)&0x80u),8)));
        c->ip=0x2F1Du;
        break;
    case 0x2F1Du: /* or al, al */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax)|(uint8_t)(c->ax)),8)));
        c->ip=0x2F1Fu;
        break;
    case 0x2F1Fu: /* je 0x2f38 */
        c->ip=(c->flags&64u)?0x2F38u:0x2F21u;
        break;
    case 0x2F21u: /* les di, ptr [bp + 0xa] */
        at=phys(c->ss,(uint16_t)(c->bp + 0xAu));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2F24u;
        break;
    case 0x2F24u: /* mov ax, word ptr es:[di + 0xfc] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xFCu),16));
        c->ip=0x2F29u;
        break;
    case 0x2F29u: /* mov dx, word ptr es:[di + 0xfe] */
        c->dx=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0xFEu),16));
        c->ip=0x2F2Eu;
        break;
    case 0x2F2Eu: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2F31u;
        break;
    case 0x2F31u: /* mov word ptr es:[di], ax */
        put(m,c->es,(uint16_t)(c->di + 0x0u),16,c->ax);
        c->ip=0x2F34u;
        break;
    case 0x2F34u: /* mov word ptr es:[di + 2], dx */
        put(m,c->es,(uint16_t)(c->di + 0x2u),16,c->dx);
        c->ip=0x2F38u;
        break;
    case 0x2F38u: /* les di, ptr [bp + 6] */
        at=phys(c->ss,(uint16_t)(c->bp + 0x6u));
        c->di=(uint16_t)(word_at(m,at));
        c->es=word_at(m,at+2u);
        c->ip=0x2F3Bu;
        break;
    case 0x2F3Bu: /* mov ax, word ptr es:[di] */
        c->ax=(uint16_t)(get(m,c->es,(uint16_t)(c->di + 0x0u),16));
        c->ip=0x2F3Eu;
        break;
    case 0x2F3Eu: /* or ax, word ptr es:[di + 2] */
        c->ax=(uint16_t)(logic(c,(uint16_t)(c->ax|get(m,c->es,(uint16_t)(c->di + 0x2u),16)),16));
        c->ip=0x2F42u;
        break;
    case 0x2F42u: /* jne 0x2f4a */
        c->ip=!(c->flags&64u)?0x2F4Au:0x2F44u;
        break;
    case 0x2F44u: /* cmp byte ptr [bp - 6], 0xa */
        (void)sub(c,get(m,c->ss,(uint16_t)(c->bp - 0x6u),8),0xAu,8);
        c->ip=0x2F48u;
        break;
    case 0x2F48u: /* jne 0x2f4e */
        c->ip=!(c->flags&64u)?0x2F4Eu:0x2F4Au;
        break;
    case 0x2F4Au: /* mov byte ptr [bp - 1], 1 */
        put(m,c->ss,(uint16_t)(c->bp - 0x1u),8,0x1u);
        c->ip=0x2F4Eu;
        break;
    case 0x2F4Eu: /* mov al, byte ptr [bp - 1] */
        c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(get(m,c->ss,(uint16_t)(c->bp - 0x1u),8)));
        c->ip=0x2F51u;
        break;
    case 0x2F51u: /* mov sp, bp */
        c->sp=(uint16_t)(c->bp);
        c->ip=0x2F53u;
        break;
    case 0x2F53u: /* pop bp */
        c->bp=(uint16_t)(get(m,c->ss,c->sp,16));
        c->sp=(uint16_t)(c->sp+2u);
        c->ip=0x2F54u;
        break;
    case 0x2F54u: /* retf 8 */
        at=phys(c->ss,c->sp);
        c->ip=word_at(m,at);
        c->cs=word_at(m,at+2u);
        c->sp=(uint16_t)(c->sp+12u);
        return PR_TARGET_RETURNED;
    default:return PR_TARGET_CODE;
    }
}
