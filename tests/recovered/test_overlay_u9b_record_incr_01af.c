/* Synthetic test for the unit 0x9b +0x01AF translation. The record is fabricated. An independent
 * restatement (a wrap-point formula instead of stepwise increments) is compared over every value of the
 * +0x12 byte and boundary values of the three counters, including the 255 -> 0 wrap in the middle of a
 * two-step pair; untouched bytes, the 16-bit offset wrap, and the host-guard paths with retained partial
 * effects are checked individually. */
#include "poolrad/recovered/overlay_u9b_record_incr_01af.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* n increments, each only while the counter is nonzero: reaches 0 after (256 - b) of them and stays there */
static unsigned expect_counter(unsigned b, unsigned n)
{
    if (b == 0)
        return 0;
    return n >= 256u - b ? 0 : b + n;
}

static uint8_t seg[0x200];
static uint8_t big[0x10000];

int main(void)
{
    static const unsigned cv[] = { 0, 1, 2, 0x7F, 0x80, 0xFD, 0xFE, 0xFF };
    const unsigned ncv = sizeof cv / sizeof cv[0];
    unsigned t, i2, i3, i4, k;
    pr_u9b_record rec;
    pr_u9b_incr_report rep;
    uint8_t before[sizeof seg];

    for (t = 0; t < 256; t++) for (i2 = 0; i2 < ncv; i2++) for (i3 = 0; i3 < ncv; i3++) for (i4 = 0; i4 < ncv; i4++) {
        unsigned n2 = (t > 0x0C) + (t > 0x0D), n3 = (t > 0x0E) + (t > 0x0F), n4 = (t > 0x10);
        unsigned e2 = expect_counter(cv[i2], n2), e3 = expect_counter(cv[i3], n3), e4 = expect_counter(cv[i4], n4);
        for (k = 0; k < sizeof seg; k++) seg[k] = (uint8_t)(0x11 + 3 * k);
        seg[PR_U9B_INCR_T] = (uint8_t)t; seg[PR_U9B_INCR_B2] = (uint8_t)cv[i2]; seg[PR_U9B_INCR_B3] = (uint8_t)cv[i3]; seg[PR_U9B_INCR_B4] = (uint8_t)cv[i4];
        memcpy(before, seg, sizeof seg);
        rec.segment = seg; rec.segment_size = sizeof seg; rec.offset = 0;
        CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK);
        CHECK(seg[PR_U9B_INCR_B2] == e2 && seg[PR_U9B_INCR_B3] == e3 && seg[PR_U9B_INCR_B4] == e4 && seg[PR_U9B_INCR_T] == t);
        for (k = 0; k < sizeof seg; k++)
            if (k != PR_U9B_INCR_B2 && k != PR_U9B_INCR_B3 && k != PR_U9B_INCR_B4) CHECK(seg[k] == before[k]);
        {
            unsigned inc2 = 0, v = cv[i2], s;
            for (s = 0; s < n2; s++) if (v != 0) { v = (v + 1) & 0xFF; inc2++; }
            v = cv[i3]; for (s = 0; s < n3; s++) if (v != 0) { v = (v + 1) & 0xFF; inc2++; }
            v = cv[i4]; for (s = 0; s < n4; s++) if (v != 0) { v = (v + 1) & 0xFF; inc2++; }
            CHECK(rep.increments == inc2);
        }
    }
    /* named boundary cases: the second step is skipped once the first wraps the counter to zero */
    memset(seg, 0, sizeof seg); seg[PR_U9B_INCR_T] = 0x0E; seg[PR_U9B_INCR_B2] = 0xFF;
    rec.segment = seg; rec.segment_size = sizeof seg; rec.offset = 0;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK && seg[PR_U9B_INCR_B2] == 0 && rep.increments == 1 && rep.steps_taken == 1u);
    memset(seg, 0, sizeof seg); seg[PR_U9B_INCR_T] = 0x11; seg[PR_U9B_INCR_B2] = 5; seg[PR_U9B_INCR_B3] = 6; seg[PR_U9B_INCR_B4] = 7;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK && seg[PR_U9B_INCR_B2] == 7 && seg[PR_U9B_INCR_B3] == 8 && seg[PR_U9B_INCR_B4] == 8);
    CHECK(rep.increments == 5 && rep.steps_taken == 0x1Fu);
    memset(seg, 0, sizeof seg); seg[PR_U9B_INCR_T] = 0x0C; seg[PR_U9B_INCR_B2] = 5; seg[PR_U9B_INCR_B3] = 5; seg[PR_U9B_INCR_B4] = 5;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK && seg[PR_U9B_INCR_B2] == 5 && seg[PR_U9B_INCR_B3] == 5 && seg[PR_U9B_INCR_B4] == 5 && rep.increments == 0);
    memset(seg, 0, sizeof seg); seg[PR_U9B_INCR_T] = 0xFF; seg[PR_U9B_INCR_B2] = 0; seg[PR_U9B_INCR_B3] = 0; seg[PR_U9B_INCR_B4] = 0;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK && rep.increments == 0);           /* the t compare is unsigned: 0xFF is large */

    /* 16-bit offset wrap in a 64 KiB view: the record at far offset 0xFF80 has +0x12 at 0xFF92 and +0xB2 at 0x0032 */
    memset(big, 0, sizeof big);
    big[(uint16_t)(0xFF80 + PR_U9B_INCR_T)] = 0x20; big[(uint16_t)(0xFF80 + PR_U9B_INCR_B2)] = 9;
    big[(uint16_t)(0xFF80 + PR_U9B_INCR_B3)] = 1; big[(uint16_t)(0xFF80 + PR_U9B_INCR_B4)] = 2;
    rec.segment = big; rec.segment_size = sizeof big; rec.offset = 0xFF80;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK);
    CHECK(big[0x0032] == 11 && big[0x0033] == 3 && big[0x0034] == 3 && big[0xFF92] == 0x20);
    CHECK(big[PR_U9B_INCR_B2] == 0 && big[0xFF80 + 0x12 - 1] == 0);                           /* the unwrapped addresses are not touched */

    /* host guards: a null view or record, a view too small for a reached access (earlier increments stay) */
    CHECK(pr_u9b_record_incr(NULL, &rep) == PR_U9B_INCR_ARGUMENT);
    rec.segment = NULL; rec.segment_size = 0x200; rec.offset = 0;
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_ARGUMENT && rep.increments == 0);
    memset(seg, 0, sizeof seg); seg[PR_U9B_INCR_T] = 0x20; seg[PR_U9B_INCR_B2] = 1; seg[PR_U9B_INCR_B3] = 1;
    rec.segment = seg; rec.segment_size = 0x13; rec.offset = 0;                              /* only +0x12 readable */
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_ARGUMENT && rep.increments == 0 && seg[PR_U9B_INCR_B2] == 1);
    rec.segment_size = PR_U9B_INCR_B3;                                                       /* +0xB2 readable, +0xB3 not */
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_ARGUMENT && rep.increments == 2 && seg[PR_U9B_INCR_B2] == 3 && seg[PR_U9B_INCR_B3] == 1);
    rec.segment_size = 0x13; seg[PR_U9B_INCR_T] = 0x0C;                                      /* no step reaches a counter: OK in a tiny view */
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_OK && rep.increments == 0);
    rec.segment_size = 0x12;                                                                 /* even +0x12 is outside */
    CHECK(pr_u9b_record_incr(&rec, &rep) == PR_U9B_INCR_ARGUMENT);
    CHECK(pr_u9b_record_incr(&rec, NULL) == PR_U9B_INCR_ARGUMENT);

    puts("overlay_u9b_record_incr_01af: ok");
    return 0;
}
