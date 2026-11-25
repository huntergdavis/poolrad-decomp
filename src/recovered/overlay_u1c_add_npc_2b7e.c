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

/* One live use of the active pointer: read DS:0x5D94:DS:0x5D92 now, resolve the record, and report the pair.
 * Returns the record bytes, or NULL when the pointer cannot be read or resolved. */
static uint8_t *use_point(const pr_u1c_add_npc_hooks *h, unsigned use, pr_u1c_add_npc_report *r)
{
    uint16_t seg = 0u, off = 0u;
    uint8_t *bytes;
    if (h->read_active(h->context, &seg, &off) != 0)
        return NULL;
    r->use_segment[use] = seg;
    r->use_offset[use] = off;
    bytes = h->record_bytes(h->context, seg, off, PR_U1C_ADD_NPC_RECORD_SIZE);
    if (bytes == NULL)
        return NULL;
    r->use_resolved[use] = 1u;
    return bytes;
}

pr_u1c_add_npc_status pr_u1c_op_add_npc(const pr_u1c_add_npc_hooks *h, pr_u1c_add_npc_report *report)
{
    pr_u1c_add_npc_report local;
    pr_u1c_add_npc_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t v1 = 0u, v2 = 0u;
    uint8_t *bytes;
    int pending_hook = 0;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status[0] = r->slot_status[1] = PR_REC_ECL_WINDOW_OK;
    if (!h || !h->load_operands || !h->slot_value || !h->add_npc || !h->read_active || !h->record_bytes ||
        !h->resident_0bb8 || !h->resident_0767)
        return PR_U1C_ADD_NPC_ARGUMENT;

    ls = h->load_operands(h->context, PR_U1C_ADD_NPC_OPERANDS);      /* 0x2B:0x2A, loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_ADD_NPC_CORE;                                  /* no slot is read after a loader failure */
    }

    vs = h->slot_value(h->context, 1u, &v1);                         /* 0x2B:0x25, slot 1 */
    r->slot_status[0] = vs;
    if (fatal(vs)) {
        r->core_status = window_to_handler(vs);
        return PR_U1C_ADD_NPC_CORE;
    }
    pending_hook = vs == PR_REC_ECL_WINDOW_HOOK;
    r->slot1_word = v1;
    h->add_npc(h->context, v1);                                      /* 0x7C:0x43, the AX word (low byte = id) */
    r->npc_called = 1u;

    vs = h->slot_value(h->context, 2u, &v2);                         /* 0x2B:0x25, slot 2 */
    r->slot_status[1] = vs;
    if (fatal(vs)) {
        r->core_status = window_to_handler(vs);                      /* the add-NPC call ran; no record store */
        return PR_U1C_ADD_NPC_CORE;
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK)
        pending_hook = 1;
    r->slot2_word = v2;

    /* CLASS use point (+2BC0): the active pointer is read after the add-NPC call. */
    bytes = use_point(h, PR_U1C_ADD_NPC_USE_CLASS, r);
    if (bytes == NULL) {
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;
        return PR_U1C_ADD_NPC_CORE;
    }
    r->class_byte = (uint8_t)(((uint8_t)(v2 & 0xFFu) >> 1) | 0x80u); /* xor ah; shr ax,1; or al,0x80 */
    bytes[PR_U1C_ADD_NPC_OFF_CLASS] = r->class_byte;                 /* les di,[0x5D92]; mov es:[di+0x84],al */
    r->class_written = 1u;

    /* FLAG use point (+2BCF or +2BDB): the active pointer is read again. */
    bytes = use_point(h, PR_U1C_ADD_NPC_USE_FLAG, r);
    if (bytes == NULL) {
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;              /* the class store stands; no flag store */
        return PR_U1C_ADD_NPC_CORE;
    }
    r->flag_byte = (uint8_t)((v1 & 0xFFu) == PR_U1C_ADD_NPC_FLAG_ID ? 1u : 0u);   /* cmp byte [bp-5],0x18 */
    bytes[PR_U1C_ADD_NPC_OFF_FLAG] = r->flag_byte;                   /* es:[di+0x10E] */
    r->flag_written = 1u;

    /* RES_0BB8 use point (+2BE5/+2BE9): the pair is read again and validated before the call. */
    if (use_point(h, PR_U1C_ADD_NPC_USE_RES_0BB8, r) == NULL) {
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;
        return PR_U1C_ADD_NPC_CORE;
    }
    h->resident_0bb8(h->context, r->use_segment[PR_U1C_ADD_NPC_USE_RES_0BB8],
                     r->use_offset[PR_U1C_ADD_NPC_USE_RES_0BB8]);      /* 0xBA:0x0BB8 */
    r->residents_called++;

    /* RES_0767 use point (+2BF2/+2BF6): read and validated again, after the first resident may have retargeted it. */
    if (use_point(h, PR_U1C_ADD_NPC_USE_RES_0767, r) == NULL) {
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;
        return PR_U1C_ADD_NPC_CORE;
    }
    h->resident_0767(h->context, r->use_segment[PR_U1C_ADD_NPC_USE_RES_0767],
                     r->use_offset[PR_U1C_ADD_NPC_USE_RES_0767]);      /* 0xBA:0x0767 */
    r->residents_called++;

    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_ADD_NPC_CORE;
    }
    return PR_U1C_ADD_NPC_OK;
}
