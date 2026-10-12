/* Host model of the party-building part of GAME.OVR unit 0x6f entry 8; see
 * docs/re/SLOT_A_PARTY_INPUT.md. */
#include "poolrad/recovered/overlay_u6f_party_load.h"

#include <string.h>

#include "poolrad/recovered/overlay_u9b_record_incr_01af.h"

pr_u6f_party_status pr_u6f_party_plan_from_savgam(const uint8_t *savgam, size_t size, pr_u6f_party_plan *plan)
{
    unsigned k, count;
    if (!savgam || !plan || size != PR_U6F_SAVGAM_SIZE)
        return PR_U6F_PARTY_ARGUMENT;
    count = savgam[PR_U6F_PARTY_COUNT_OFFSET];
    if (count > PR_U6F_PARTY_SLOTS)
        return PR_U6F_PARTY_COUNT;
    memset(plan, 0, sizeof *plan);
    for (k = 0; k < count; k++) {
        const uint8_t *slot = savgam + PR_U6F_PARTY_NAMES_OFFSET + (size_t)k * PR_U6F_PARTY_NAME_STRIDE;
        if (slot[0] > PR_U6F_PARTY_NAME_MAX) {
            memset(plan, 0, sizeof *plan);
            return PR_U6F_PARTY_NAME;
        }
        plan->name_len[k] = slot[0];
        plan->name_text[k] = slot + 1;
    }
    plan->count = count;
    return PR_U6F_PARTY_OK;
}

pr_u6f_party_status pr_u6f_party_build(const uint8_t *const records[PR_U6F_PARTY_SLOTS], unsigned count,
                                       pr_u6f_party *party)
{
    unsigned k, used = 0u, n = 0u;
    pr_u68_0236_node *tail = NULL;
    if (!records || !party)
        return PR_U6F_PARTY_ARGUMENT;
    if (count > PR_U6F_PARTY_SLOTS)
        return PR_U6F_PARTY_COUNT;
    memset(party, 0, sizeof *party);
    for (k = 0; k < count; k++) {
        uint8_t *rec;
        unsigned s;
        if (records[k] == NULL)                                        /* +0x19A8: the file does not exist */
            continue;
        rec = party->record[n];
        memcpy(rec, records[k], PR_U6F_PARTY_RECORD_SIZE);             /* the 0x11D-byte read */
        memset(rec + 0x7F, 0, 4);                                      /* +0x0C54..+0x0C8A: far-pointer slots */
        memset(rec + 0xC8, 0, 4);
        memset(rec + 0x104, 0, 8);
        for (s = 0; s < PR_U6F_PARTY_SLOTS && (used & (1u << s)); s++) /* +0x153E..+0x1563: first free index */
            ;
        rec[PR_U6F_PARTY_SLOT_BYTE] = (uint8_t)s;
        if (s < PR_U6F_PARTY_SLOTS)
            used |= 1u << s;
        party->slot[n] = (uint8_t)s;
        party->node[n].segment = rec;
        party->node[n].segment_size = PR_U6F_PARTY_RECORD_SIZE;
        party->node[n].offset = 0;
        party->node[n].next = NULL;
        if (tail)
            tail->next = &party->node[n];                              /* +0x1487..+0x14B9: append at the tail */
        else
            party->head = &party->node[n];                             /* +0x145E..+0x1467: the empty list */
        tail = &party->node[n];
        {                                                              /* +0x1574: lcall 0xAC:0x25 = unit 0x9b +0x01AF */
            pr_u9b_record view;
            view.segment = rec; view.segment_size = PR_U6F_PARTY_RECORD_SIZE; view.offset = 0;
            (void)pr_u9b_record_incr(&view, NULL);                     /* +0x12 and +0xB2..+0xB4 lie inside the 285 bytes */
        }
        n++;
    }
    party->loaded = n;                                                 /* +0x1917 zero, +0x1569 once per member */
    return PR_U6F_PARTY_OK;
}
