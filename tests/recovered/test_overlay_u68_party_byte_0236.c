/* Synthetic parity test for the unit 0x68 +0x0236 translation. Records and
 * the list are fabricated. An independent restatement on raw bytes is
 * compared with the routine for every argument byte 0..255, every byte
 * value at the tested index, and lists of 0..4 nodes. */
#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define REC 0x120u

static int sb(uint8_t b) { return b < 0x80u ? b : (int)b - 256; }
static uint8_t seg64[0x10000];

int main(void)
{
    static uint8_t rec[4][REC];
    pr_u68_0236_node node[4];
    unsigned k, v, len, i, j;
    uint8_t res;
    pr_u68_0236_report rep;

    /* every k, every byte value in the first node, tails of other values: the first positive byte wins */
    for (len = 0; len <= 4; len++) {
        for (k = 0; k < 256; k++) {
            int idx = 0x96 + sb((uint8_t)k);
            for (v = 0; v < 256; v++) {
                int expect = 0, hit = -1;
                for (i = 0; i < 4; i++) {
                    memset(rec[i], 0x80, REC);                  /* negative everywhere: not positive */
                    node[i].segment = rec[i]; node[i].segment_size = REC; node[i].offset = 0;
                    node[i].next = i + 1 < len ? &node[i + 1] : NULL;
                }
                for (j = 0; j < len; j++) rec[j][idx] = (uint8_t)(j == len - 1 ? v : (v + 0x55u * (j + 1)) & 0xFFu);
                for (j = 0; j < len; j++) if (sb(rec[j][idx]) > 0) { expect = 1; hit = (int)j; break; }
                res = 0xEE;
                CHECK(pr_u68_0236(len ? &node[0] : NULL, (uint8_t)k, &res, &rep) == PR_U68_0236_OK);
                CHECK(res == expect && rep.index == idx);
                CHECK(rep.visited == (hit >= 0 ? (unsigned)hit + 1u : len));
                CHECK(rep.hit == (hit >= 0 ? &node[hit] : NULL));
            }
        }
    }

    /* the byte at the neighbouring index does not matter; boundary values 0, 1, 0x7F, 0x80, 0xFF */
    {
        static const uint8_t vals[5] = { 0x00, 0x01, 0x7F, 0x80, 0xFF };
        static const uint8_t want[5] = { 0, 1, 1, 0, 0 };
        for (i = 0; i < 5; i++) {
            memset(rec[0], 0x01, REC);                           /* positive everywhere except the tested index */
            rec[0][0x9B] = vals[i];
            node[0].segment = rec[0]; node[0].segment_size = REC; node[0].offset = 0; node[0].next = NULL;
            CHECK(pr_u68_0236(&node[0], 5, &res, NULL) == PR_U68_0236_OK && res == want[i]);   /* entry 1's argument 5 -> +0x9B */
            rec[0][0x9C] = vals[i];
            CHECK(pr_u68_0236(&node[0], 6, &res, NULL) == PR_U68_0236_OK && res == want[i]);   /* argument 6 -> +0x9C */
        }
    }
    /* argument 0x80 (-128) reads +0x16, argument 0x7F reads +0x115 */
    memset(rec[0], 0, REC); rec[0][0x16] = 1;
    CHECK(pr_u68_0236(&node[0], 0x80, &res, &rep) == PR_U68_0236_OK && res == 1 && rep.index == 0x16);
    CHECK(pr_u68_0236(&node[0], 0x7F, &res, &rep) == PR_U68_0236_OK && res == 0 && rep.index == 0x115);
    rec[0][0x115] = 0x40;
    CHECK(pr_u68_0236(&node[0], 0x7F, &res, NULL) == PR_U68_0236_OK && res == 1);

    /* a later node is not read once an earlier one hits (a record too short for the index after the hit is fine) */
    {
        static uint8_t shortrec[4];
        memset(rec[0], 0, REC); rec[0][0x9B] = 9;
        node[0].next = &node[1];
        node[1].segment = shortrec; node[1].segment_size = sizeof shortrec; node[1].offset = 0; node[1].next = NULL;
        res = 0xEE;
        CHECK(pr_u68_0236(&node[0], 5, &res, &rep) == PR_U68_0236_OK && res == 1 && rep.visited == 1);
        rec[0][0x9B] = 0;                                       /* no hit: the short record is reached */
        res = 0xEE;
        CHECK(pr_u68_0236(&node[0], 5, &res, &rep) == PR_U68_0236_ARGUMENT && res == 0xEE);
        node[1].segment = NULL;
        CHECK(pr_u68_0236(&node[0], 5, &res, NULL) == PR_U68_0236_ARGUMENT);
        node[0].next = NULL;
    }

    /* 16-bit wrap: one near-FFFF case for each of the two additions (add di, ax; then the 0x96 displacement) */
    {
        pr_u68_0236_node w;
        memset(seg64, 0, sizeof seg64);
        memset(&w, 0, sizeof w);
        w.segment = seg64; w.segment_size = sizeof seg64; w.next = NULL;
        /* offset 0xFFF0, k = 0x7F: di = 0xFFF0 + 0x7F = 0x006F (wrapped), read at 0x006F + 0x96 = 0x0105 */
        w.offset = 0xFFF0u;
        seg64[0x0105] = 0x20;
        seg64[0x006F] = 0x30;                                                          /* di itself (no 0x96 added): not what is read */
        res = 0xEE;
        CHECK(pr_u68_0236(&w, 0x7F, &res, &rep) == PR_U68_0236_OK && res == 1 && rep.hit == &w && rep.hit_address == 0x0105);
        seg64[0x0105] = 0;
        CHECK(pr_u68_0236(&w, 0x7F, &res, &rep) == PR_U68_0236_OK && res == 0);       /* the unwrapped address is not what is read */
        /* offset 0xFFE0, k = 0: di = 0xFFE0 (no wrap), di + 0x96 = 0x10076 wraps to 0x0076 */
        w.offset = 0xFFE0u;
        memset(seg64, 0, sizeof seg64);
        seg64[0x0076] = 1;
        CHECK(pr_u68_0236(&w, 0, &res, &rep) == PR_U68_0236_OK && res == 1 && rep.hit_address == 0x0076);
        /* a negative k that wraps below zero: offset 0x0010, k = 0x80 (-128): di = 0xFF90, read at 0xFF90 + 0x96 = 0x0026 (wrapped) */
        w.offset = 0x0010u;
        memset(seg64, 0, sizeof seg64);
        seg64[0x0026] = 0x7F;
        CHECK(pr_u68_0236(&w, 0x80, &res, &rep) == PR_U68_0236_OK && res == 1 && rep.hit_address == 0x0026);
        seg64[0x0026] = 0x80;                                                          /* 0x80 is negative: not positive */
        CHECK(pr_u68_0236(&w, 0x80, &res, &rep) == PR_U68_0236_OK && res == 0);
        /* a view smaller than the wrapped address is a host guard, result untouched */
        w.segment_size = 0x20; res = 0xEE;
        CHECK(pr_u68_0236(&w, 0x80, &res, NULL) == PR_U68_0236_ARGUMENT && res == 0xEE);
    }

    /* cyclic list without a hit hits the cap; with a hit it terminates; null result */
    memset(rec[0], 0, REC);
    node[0].segment = rec[0]; node[0].segment_size = REC; node[0].offset = 0; node[0].next = &node[0];
    res = 0xEE;
    CHECK(pr_u68_0236(&node[0], 5, &res, NULL) == PR_U68_0236_ARGUMENT && res == 0xEE);
    rec[0][0x9B] = 1;
    CHECK(pr_u68_0236(&node[0], 5, &res, NULL) == PR_U68_0236_OK && res == 1);
    CHECK(pr_u68_0236(&node[0], 5, NULL, NULL) == PR_U68_0236_ARGUMENT);

    puts("overlay_u68_party_byte_0236: ok");
    return 0;
}
