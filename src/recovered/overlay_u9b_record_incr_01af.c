/* Translation of GAME.OVR unit 0x9b +0x01AF; see docs/re/U9B_RECORD_INCR_01AF.md. */
#include "poolrad/recovered/overlay_u9b_record_incr_01af.h"

#include <string.h>

static uint8_t *at(const pr_u9b_record *r, unsigned k)
{
    size_t eff = (size_t)(uint16_t)(r->offset + k);           /* es:[di + k]: 16-bit wrap */
    if (r->segment == NULL || eff >= r->segment_size)
        return NULL;
    return r->segment + eff;
}

pr_u9b_incr_status pr_u9b_record_incr(const pr_u9b_record *rec, pr_u9b_incr_report *report)
{
    static const struct { uint8_t above; uint8_t counter; } step[PR_U9B_INCR_STEPS] = {
        { 0x0Cu, PR_U9B_INCR_B2 },      /* +0x01C1..+0x01D6 */
        { 0x0Du, PR_U9B_INCR_B2 },      /* +0x01DE..+0x01F3 */
        { 0x0Eu, PR_U9B_INCR_B3 },      /* +0x01FB..+0x0210 */
        { 0x0Fu, PR_U9B_INCR_B3 },      /* +0x0218..+0x022D */
        { 0x10u, PR_U9B_INCR_B4 }       /* +0x0235..+0x024A */
    };
    pr_u9b_incr_report local;
    pr_u9b_incr_report *r = report ? report : &local;
    unsigned i;
    memset(r, 0, sizeof *r);
    if (!rec)
        return PR_U9B_INCR_ARGUMENT;
    for (i = 0; i < PR_U9B_INCR_STEPS; i++) {
        uint8_t *t = at(rec, PR_U9B_INCR_T);                   /* re-read before every step */
        uint8_t *c;
        if (!t)
            return PR_U9B_INCR_ARGUMENT;
        if (*t <= step[i].above)                               /* cmp byte [+0x12], n ; jbe skip (unsigned) */
            continue;
        c = at(rec, step[i].counter);
        if (!c)
            return PR_U9B_INCR_ARGUMENT;
        if (*c == 0u)                                          /* cmp byte [counter], 0 ; jbe skip */
            continue;
        *c = (uint8_t)(*c + 1u);                               /* inc byte: wraps modulo 256 */
        r->increments++;
        r->steps_taken |= 1u << i;
    }
    return PR_U9B_INCR_OK;
}
