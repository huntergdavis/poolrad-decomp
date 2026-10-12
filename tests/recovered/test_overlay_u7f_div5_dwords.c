/* Synthetic tests for the unit 0x7f entry 11 translation: the block is a
 * fabricated byte pattern; the expected quotient and remainder of 5 come
 * from an incremental counter that never uses the division operator; the
 * high words of all five dwords must be zero and every byte outside
 * 0x00..0x13 must be untouched. */
#include "poolrad/recovered/overlay_u7f_div5_dwords.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static void fill(uint8_t *buf, size_t n, unsigned seed)
{
    size_t i;
    for (i = 0; i < n; ++i) buf[i] = (uint8_t)(i * 7u + seed);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* The twenty bytes the routine owns, as five little-endian dwords. */
static int slots_ok(const uint8_t *b, unsigned q, unsigned rm)
{
    return rd32(b) == 0 && rd32(b + 4) == 0 && rd32(b + 8) == 0 && rd32(b + 12) == rm && rd32(b + 16) == q;
}

/* Everything outside 0x00..0x13 must equal the template. */
static int rest_untouched(const uint8_t *b, const uint8_t *tpl, size_t n)
{
    return n <= 0x14u || memcmp(b + 0x14u, tpl + 0x14u, n - 0x14u) == 0;
}

static int test_pinned(void)
{
    static const unsigned cases[][3] = {
        {0, 0, 0}, {4, 0, 4}, {5, 1, 0}, {65535, 13107, 0},
        {1, 0, 1}, {6, 1, 1}, {9, 1, 4}, {10, 2, 0}, {0x8000, 6553, 3},
        {65530, 13106, 0}, {65531, 13106, 1}, {65534, 13106, 4}, {0xFF, 51, 0}, {0x100, 51, 1}
    };
    uint8_t blk[0x40], tpl[0x40];
    pr_u7f_dwords_report rep;
    unsigned k;
    for (k = 0; k < sizeof cases / sizeof cases[0]; ++k) {
        fill(tpl, sizeof tpl, 3u); memcpy(blk, tpl, sizeof blk);
        /* every slot, both halves, starts non-zero so the clears and the zero high words are observable */
        memset(blk, 0xA5, 0x14);
        CHECK(pr_u7f_entry11(blk, sizeof blk, (uint16_t)cases[k][0], &rep) == PR_U7F_DWORDS_OK);
        CHECK(rep.quotient == cases[k][1] && rep.remainder == cases[k][2]);
        CHECK(rep.cleared_slots == 5 && rep.first_offset == 0 && rep.last_offset == 0x13);
        CHECK(slots_ok(blk, cases[k][1], cases[k][2]));
        CHECK(rest_untouched(blk, tpl, sizeof blk));
    }
    /* 65535: quotient 0x3333 at 0x10/0x11 with 0x12/0x13 zero; remainder 0 at 0x0C..0x0F (a signed
     * division would give 0xFFFF at 0x0C/0x0D) */
    fill(blk, sizeof blk, 9u);
    CHECK(pr_u7f_entry11(blk, sizeof blk, 65535u, NULL) == PR_U7F_DWORDS_OK);   /* report optional */
    CHECK(blk[0x0C] == 0 && blk[0x0D] == 0 && blk[0x0E] == 0 && blk[0x0F] == 0);
    CHECK(blk[0x10] == 0x33 && blk[0x11] == 0x33 && blk[0x12] == 0 && blk[0x13] == 0);
    return 0;
}

static int test_exhaustive(void)
{
    uint8_t blk[PR_U7F_DWORDS_BLOCK_SIZE], tpl[PR_U7F_DWORDS_BLOCK_SIZE];
    pr_u7f_dwords_report rep;
    unsigned v, q = 0, rm = 0;   /* incremental restatement: no '/' or '%' */
    fill(tpl, sizeof tpl, 0x40u);
    for (v = 0; v < 0x10000u; ++v) {
        if (v != 0) { if (++rm == 5u) { rm = 0; ++q; } }
        memcpy(blk, tpl, sizeof blk);
        blk[0x00] = (uint8_t)v; blk[0x0B] = (uint8_t)(v >> 8); blk[0x0E] = 0xFF; blk[0x13] = 0xFF;
        CHECK(pr_u7f_entry11(blk, sizeof blk, (uint16_t)v, &rep) == PR_U7F_DWORDS_OK);
        CHECK(rep.quotient == q && rep.remainder == rm);
        CHECK((unsigned)rep.quotient * 5u + rep.remainder == v && rep.remainder < 5u && rep.quotient <= 13107u);
        CHECK(slots_ok(blk, q, rm));
        CHECK(rest_untouched(blk, tpl, sizeof blk));   /* slots 5 and 6 of the 28-byte block are never touched */
    }
    return 0;
}

static int test_bounds(void)
{
    uint8_t blk[0x40], tpl[0x40];
    pr_u7f_dwords_report rep;
    size_t n;
    fill(tpl, sizeof tpl, 1u);
    /* null block: ARGUMENT, report cleared */
    memset(&rep, 0xEE, sizeof rep);
    CHECK(pr_u7f_entry11(NULL, PR_U7F_DWORDS_BLOCK_SIZE, 7, &rep) == PR_U7F_DWORDS_ARGUMENT);
    CHECK(rep.cleared_slots == 0 && rep.quotient == 0 && rep.remainder == 0);
    CHECK(pr_u7f_entry11(NULL, 0, 7, NULL) == PR_U7F_DWORDS_ARGUMENT);
    /* sizes 0..0x13: ARGUMENT and no write; exactly 0x14: accepted, all twenty bytes written */
    for (n = 0; n < 0x14u; ++n) {
        memcpy(blk, tpl, sizeof blk);
        CHECK(pr_u7f_entry11(blk, n, 12345, &rep) == PR_U7F_DWORDS_ARGUMENT);
        CHECK(memcmp(blk, tpl, sizeof blk) == 0);
    }
    memcpy(blk, tpl, sizeof blk);
    CHECK(pr_u7f_entry11(blk, 0x14u, 12345, &rep) == PR_U7F_DWORDS_OK);
    CHECK(slots_ok(blk, 2469, 0) && memcmp(blk + 0x14u, tpl + 0x14u, sizeof blk - 0x14u) == 0);
    /* the engine's 28-byte block and a larger host buffer: only the twenty bytes change */
    memcpy(blk, tpl, sizeof blk);
    CHECK(pr_u7f_entry11(blk, PR_U7F_DWORDS_BLOCK_SIZE, 0xFFFFu, &rep) == PR_U7F_DWORDS_OK);
    CHECK(slots_ok(blk, 13107, 0) && rest_untouched(blk, tpl, sizeof blk));
    memcpy(blk, tpl, sizeof blk);
    CHECK(pr_u7f_entry11(blk, sizeof blk, 0xFFFFu, &rep) == PR_U7F_DWORDS_OK);
    CHECK(slots_ok(blk, 13107, 0) && rest_untouched(blk, tpl, sizeof blk));
    /* repeated calls replace rather than accumulate */
    CHECK(pr_u7f_entry11(blk, sizeof blk, 0xFFFFu, &rep) == PR_U7F_DWORDS_OK && slots_ok(blk, 13107, 0));
    CHECK(pr_u7f_entry11(blk, sizeof blk, 4, &rep) == PR_U7F_DWORDS_OK && slots_ok(blk, 0, 4));
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_pinned();
    rc |= test_exhaustive();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x7f entry 11 translation tests passed");
    return rc;
}
