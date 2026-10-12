/* Portable C translation of GAME.OVR unit 0x68 +0x0236; see
 * docs/re/U68_PARTY_BYTE_0236.md. */
#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

pr_u68_0236_status pr_u68_0236(const pr_u68_0236_node *head, uint8_t k, uint8_t *result,
                               pr_u68_0236_report *report)
{
    pr_u68_0236_report local;
    pr_u68_0236_report *r = report ? report : &local;
    const pr_u68_0236_node *n;
    unsigned visited = 0;
    int idx = (int)PR_U68_0236_BYTE_BASE + (k < 0x80u ? (int)k : (int)k - 256);   /* cbw, add di, ax, disp 0x96 */
    const pr_u68_0236_node *hit = NULL;
    uint8_t found = 0;                      /* [bp-6] */
    r->visited = 0;
    r->hit = NULL;
    r->index = idx;
    if (!result)
        return PR_U68_0236_ARGUMENT;
    /* host guard pass first, so nothing is reported half-way: every node that will be tested must be readable */
    for (n = head; n; n = n->next) {
        if (!n->record || n->record_size <= (size_t)idx || ++visited > PR_U68_0236_LIST_CAP)
            return PR_U68_0236_ARGUMENT;
        if ((int8_t)n->record[idx] > 0)     /* +0x0264..+0x026A: cmp byte es:[di + 0x96], 0; jle */
            break;                          /* found: the loop ends at +0x0255 after one more next read */
    }
    visited = 0;
    /* +0x023C..+0x0283 */
    for (n = head; n && !found; n = n->next) {
        visited++;
        if ((int8_t)n->record[idx] > 0) {
            found = 1;
            hit = n;
        }
    }
    r->visited = visited;
    r->hit = hit;
    *result = found;                        /* +0x0285..+0x028B */
    return PR_U68_0236_OK;
}
