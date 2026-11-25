/* Bounded model of GAME.OVR unit 0x1c +0x2B7E (ECL opcode 0x36, ADD NPC); see docs/re/U1C_ADD_NPC_2B7E.md. */
#include "poolrad/recovered/overlay_u1c_add_npc_2b7e.h"

#include <string.h>

static pr_rec_ecl_handler_status window_to_handler(pr_rec_ecl_window_status ws)
{
    switch (ws) {
    case PR_REC_ECL_WINDOW_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_WINDOW_HOOK: return PR_REC_ECL_HANDLER_HOOK;
    case PR_REC_ECL_WINDOW_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    case PR_REC_ECL_WINDOW_UNRESOLVED: return PR_REC_ECL_HANDLER_UNRESOLVED;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
}

static int fatal(pr_rec_ecl_window_status ws)
{
    return ws == PR_REC_ECL_WINDOW_BOUNDS || ws == PR_REC_ECL_WINDOW_UNRESOLVED || ws == PR_REC_ECL_WINDOW_ARGUMENT;
}

pr_u1c_add_npc_status pr_u1c_op_add_npc(const pr_u1c_add_npc_hooks *h, const pr_u1c_add_npc_record *rec,
                                        pr_u1c_add_npc_report *report)
{
    pr_u1c_add_npc_report local;
    pr_u1c_add_npc_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t v1 = 0u, v2 = 0u;
    int pending_hook = 0;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status[0] = r->slot_status[1] = PR_REC_ECL_WINDOW_OK;
    if (!h || !h->load_operands || !h->slot_value || !h->add_npc || !h->resident_0bb8 || !h->resident_0767)
        return PR_U1C_ADD_NPC_ARGUMENT;
    if (!rec || !rec->record || rec->record_size < PR_U1C_ADD_NPC_RECORD_SIZE)
        return PR_U1C_ADD_NPC_ARGUMENT;

    ls = h->load_operands(h->context, PR_U1C_ADD_NPC_OPERANDS);       /* 0x2B:0x2A, loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_ADD_NPC_CORE;
    }

    vs = h->slot_value(h->context, 1u, &v1);                          /* 0x2B:0x25, slot 1 */
    r->slot_status[0] = vs;
    if (fatal(vs)) {
        r->core_status = window_to_handler(vs);
        return PR_U1C_ADD_NPC_CORE;
    }
    pending_hook = vs == PR_REC_ECL_WINDOW_HOOK;
    r->slot1_word = v1;
    h->add_npc(h->context, v1);                                       /* 0x7C:0x43, the AX word (low byte = id) */
    r->npc_called = 1u;

    vs = h->slot_value(h->context, 2u, &v2);                          /* 0x2B:0x25, slot 2 */
    r->slot_status[1] = vs;
    if (fatal(vs)) {
        r->core_status = window_to_handler(vs);                       /* the record bytes are not written */
        return PR_U1C_ADD_NPC_CORE;
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK)
        pending_hook = 1;
    r->slot2_word = v2;

    r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u);  /* xor ah; shr ax,1; or al,0x80 */
    rec->record[PR_U1C_ADD_NPC_OFF_CLASS] = r->class_byte;             /* les di,[0x5D92]; mov es:[di+0x84],al */
    r->flag_byte = (uint8_t)((v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u);  /* cmp byte [bp-5],0x18 */
    rec->record[PR_U1C_ADD_NPC_OFF_FLAG] = r->flag_byte;               /* es:[di+0x10E] */

    h->resident_0bb8(h->context, rec->record_seg, rec->record_off);   /* 0xBA:0x0BB8 with the far pointer */
    h->resident_0767(h->context, rec->record_seg, rec->record_off);   /* 0xBA:0x0767 with the far pointer */
    r->residents_called = 1u;

    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_ADD_NPC_CORE;
    }
    return PR_U1C_ADD_NPC_OK;
}
