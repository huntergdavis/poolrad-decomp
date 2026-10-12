/* Synthetic tests for the unit 0x7f entry 10 translation: the record is a
 * fabricated byte pattern; the expected quotient and remainder of 5 come
 * from an incremental counter that never uses the division operator, and
 * every byte outside 0x88..0x91 is checked to be untouched. */
#include "poolrad/recovered/overlay_u7f_div5.h"

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

/* The ten bytes the routine owns, as the translation must leave them. */
static int ten_bytes_ok(const uint8_t *rec, unsigned q, unsigned rm)
{
    return rec[0x88] == 0 && rec[0x89] == 0 && rec[0x8A] == 0 && rec[0x8B] == 0 &&
           rec[0x8C] == 0 && rec[0x8D] == 0 &&
           rec[0x8E] == (rm & 0xFFu) && rec[0x8F] == (rm >> 8) &&
           rec[0x90] == (q & 0xFFu) && rec[0x91] == (q >> 8);
}

/* Everything outside 0x88..0x91 must equal the template. */
static int rest_untouched(const uint8_t *rec, const uint8_t *tpl, size_t n)
{
    return memcmp(rec, tpl, 0x88u) == 0 && (n <= 0x92u || memcmp(rec + 0x92u, tpl + 0x92u, n - 0x92u) == 0);
}

static int test_pinned(void)
{
    /* value, quotient, remainder: hand-computed; 65535 and 0x8000 show the unsigned division */
    static const unsigned cases[][3] = {
        {0, 0, 0}, {4, 0, 4}, {5, 1, 0}, {65535, 13107, 0},
        {1, 0, 1}, {6, 1, 1}, {9, 1, 4}, {10, 2, 0}, {0x8000, 6553, 3},
        {65530, 13106, 0}, {65531, 13106, 1}, {65534, 13106, 4}, {0xFF, 51, 0}, {0x100, 51, 1}
    };
    uint8_t rec[PR_U7F_DIV5_RECORD_SIZE], tpl[PR_U7F_DIV5_RECORD_SIZE];
    pr_u7f_div5_report rep;
    unsigned k;
    for (k = 0; k < sizeof cases / sizeof cases[0]; ++k) {
        fill(tpl, sizeof tpl, 3u); memcpy(rec, tpl, sizeof rec);
        /* the five word slots start non-zero so the loop's clearing is observable */
        memset(rec + 0x88, 0xA5, 10); memset(tpl + 0x88, 0xA5, 10);
        CHECK(pr_u7f_entry10(rec, sizeof rec, (uint16_t)cases[k][0], &rep) == PR_U7F_DIV5_OK);
        CHECK(rep.quotient == cases[k][1] && rep.remainder == cases[k][2]);
        CHECK(rep.cleared_words == 5 && rep.first_offset == 0x88 && rep.last_offset == 0x91);
        CHECK(ten_bytes_ok(rec, cases[k][1], cases[k][2]));
        CHECK(rest_untouched(rec, tpl, sizeof rec));
    }
    /* 65535: the quotient 13107 = 0x3333 lands little-endian at 0x90/0x91 and the remainder 0
     * replaces the loop's zero at 0x8E/0x8F (a signed reading would give 0xFFFF there) */
    fill(rec, sizeof rec, 9u);
    CHECK(pr_u7f_entry10(rec, sizeof rec, 65535u, NULL) == PR_U7F_DIV5_OK);   /* report optional */
    CHECK(rec[0x8E] == 0 && rec[0x8F] == 0 && rec[0x90] == 0x33 && rec[0x91] == 0x33);
    return 0;
}

static int test_exhaustive(void)
{
    uint8_t rec[PR_U7F_DIV5_RECORD_SIZE], tpl[PR_U7F_DIV5_RECORD_SIZE];
    pr_u7f_div5_report rep;
    unsigned v, q = 0, rm = 0;   /* incremental restatement: no '/' or '%' */
    fill(tpl, sizeof tpl, 0x40u);
    for (v = 0; v < 0x10000u; ++v) {
        if (v != 0) { if (++rm == 5u) { rm = 0; ++q; } }
        memcpy(rec, tpl, sizeof rec);
        rec[0x88] = (uint8_t)v; rec[0x8D] = (uint8_t)(v >> 8); rec[0x8F] = 0xFF; rec[0x91] = 0xFF;
        CHECK(pr_u7f_entry10(rec, sizeof rec, (uint16_t)v, &rep) == PR_U7F_DIV5_OK);
        CHECK(rep.quotient == q && rep.remainder == rm);
        CHECK((unsigned)rep.quotient * 5u + rep.remainder == v && rep.remainder < 5u && rep.quotient <= 13107u);
        CHECK(ten_bytes_ok(rec, q, rm));
        CHECK(rest_untouched(rec, tpl, sizeof rec));
    }
    return 0;
}

static int test_bounds(void)
{
    uint8_t rec[0x100], tpl[0x100];
    pr_u7f_div5_report rep;
    size_t n;
    fill(tpl, sizeof tpl, 1u);
    /* null record: ARGUMENT, report cleared */
    memset(&rep, 0xEE, sizeof rep);
    CHECK(pr_u7f_entry10(NULL, PR_U7F_DIV5_RECORD_SIZE, 7, &rep) == PR_U7F_DIV5_ARGUMENT);
    CHECK(rep.cleared_words == 0 && rep.quotient == 0 && rep.remainder == 0);
    CHECK(pr_u7f_entry10(NULL, 0, 7, NULL) == PR_U7F_DIV5_ARGUMENT);
    /* too short by one byte (0x91 bytes: the last store would need 0x91): ARGUMENT and no write;
     * exactly 0x92 bytes: accepted, all ten bytes written, 0x00..0x87 untouched */
    for (n = 0; n <= 0x91u; n += (n < 0x80u ? 0x10u : 1u)) {
        memcpy(rec, tpl, sizeof rec);
        CHECK(pr_u7f_entry10(rec, n, 12345, &rep) == PR_U7F_DIV5_ARGUMENT);
        CHECK(memcmp(rec, tpl, sizeof rec) == 0);
    }
    memcpy(rec, tpl, sizeof rec);
    CHECK(pr_u7f_entry10(rec, 0x92u, 12345, &rep) == PR_U7F_DIV5_OK);
    CHECK(ten_bytes_ok(rec, 2469, 0) && memcmp(rec, tpl, 0x88u) == 0 && memcmp(rec + 0x92u, tpl + 0x92u, sizeof rec - 0x92u) == 0);
    /* larger than the engine frame is also accepted; only the ten bytes change */
    memcpy(rec, tpl, sizeof rec);
    CHECK(pr_u7f_entry10(rec, sizeof rec, 0xFFFFu, &rep) == PR_U7F_DIV5_OK);
    CHECK(ten_bytes_ok(rec, 13107, 0) && rest_untouched(rec, tpl, sizeof rec));
    /* repeat on the same record: idempotent for the same value, and the previous quotient and
     * remainder are replaced, not accumulated */
    CHECK(pr_u7f_entry10(rec, sizeof rec, 0xFFFFu, &rep) == PR_U7F_DIV5_OK && ten_bytes_ok(rec, 13107, 0));
    CHECK(pr_u7f_entry10(rec, sizeof rec, 4, &rep) == PR_U7F_DIV5_OK && ten_bytes_ok(rec, 0, 4));
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_pinned();
    rc |= test_exhaustive();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x7f entry 10 translation tests passed");
    return rc;
}
