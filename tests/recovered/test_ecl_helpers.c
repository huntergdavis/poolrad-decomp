/* Synthetic tests for the recovered ECL helpers. No game data is used; the
 * expected values come from the disassembly transcriptions documented in
 * docs/re/FUNCTION_LEDGER.md and from independently constructed vectors. */

#include "poolrad/recovered/ecl_helpers.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int test_char_remap(void) {
    unsigned c;
    for (c = 0; c < 256; ++c) {
        uint8_t expect = c <= 0x1Fu ? (uint8_t)(c + 0x40u) : (uint8_t)c;
        CHECK(pr_rec_ecl_char_remap((uint8_t)c) == expect);
    }
    CHECK(pr_rec_ecl_char_remap(0x01) == 'A');
    CHECK(pr_rec_ecl_char_remap(0x1A) == 'Z');
    CHECK(pr_rec_ecl_char_remap(0x1F) == '_');
    CHECK(pr_rec_ecl_char_remap(0x20) == ' ');
    CHECK(pr_rec_ecl_char_remap(0x30) == '0');
    CHECK(pr_rec_ecl_char_remap(0x3F) == '?');
    return 0;
}

/* Independent transcription of the engine's compare chains, written as a
 * dense table rather than the switch used by the translation. */
static const unsigned char engine_counts[0x3E] = {
    /* 00 */ 0, 1, 1, 2, 3, 3, 3, 3, 2, 2, 1, 3, 2, 0, 1, 2,
    /* 10 */ 2, 1, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 1, 6, 2,
    /* 20 */ 1, 3, 2, 4, 0, 0, 0, 8, 3, 14, 3, 0, 6, 1, 5, 3,
    /* 30 */ 3, 0, 1, 0, 1, 3, 1, 3, 1, 1, 0, 3, 1, 0
};

static int test_operand_count(void) {
    unsigned op, total = 0;
    for (op = 0; op < 0x3E; ++op) {
        CHECK(pr_rec_ecl_operand_count((uint8_t)op) == engine_counts[op]);
        total += engine_counts[op];
    }
    CHECK(total == 117);                 /* checksum of the table above */
    for (op = 0x3E; op < 256; ++op) {
        CHECK(pr_rec_ecl_operand_count((uint8_t)op) == 0);
    }
    /* Differences from the Gold Box Explorer reference table, by design. */
    CHECK(pr_rec_ecl_operand_count(0x0C) == 2);  /* reference says 3 */
    CHECK(pr_rec_ecl_operand_count(0x34) == 1);  /* reference says 2 */
    CHECK(pr_rec_ecl_operand_count(0x36) == 1);  /* reference says 2 */
    return 0;
}

static int test_unpack_text(void) {
    char out[8];
    static const uint8_t ab_space[3] = {0x04, 0x28, 0x00}; /* codes 1,2,32,0 */
    static const uint8_t abcd[3] = {0x04, 0x20, 0xC4};     /* codes 1,2,3,4 */
    static const uint8_t two_bytes[2] = {0x04, 0x20};      /* codes 1,2 and a partial third */
    static const uint8_t zero_mid[3] = {0x04, 0x00, 0x03}; /* codes 1,0,0,3 */
    static const uint8_t one_byte[1] = {0x04};             /* code 1, partial second */

    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(ab_space, 3, out, sizeof out) == 3);
    CHECK(memcmp(out, "AB ", 3) == 0);

    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(abcd, 3, out, sizeof out) == 4);
    CHECK(memcmp(out, "ABCD", 4) == 0);

    /* A short group: the engine emits only the codes completed by consumed
     * bytes; phase 3 is never reached with two bytes, so "AB" results and
     * the partial third code is not emitted. */
    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(two_bytes, 2, out, sizeof out) == 2);
    CHECK(memcmp(out, "AB", 2) == 0);

    /* Zero codes are skipped, not terminators: codes 1,0,0,3 give "AC". */
    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(zero_mid, 3, out, sizeof out) == 2);
    CHECK(memcmp(out, "AC", 2) == 0);

    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(one_byte, 1, out, sizeof out) == 1);
    CHECK(out[0] == 'A');

    CHECK(pr_rec_ecl_unpack_text(abcd, 0, out, sizeof out) == 0);
    CHECK(pr_rec_ecl_unpack_text(NULL, 0, out, sizeof out) == 0);
    CHECK(pr_rec_ecl_unpack_text(NULL, 3, out, sizeof out) == 0);

    /* Capacity limits writing but not the reported count. */
    memset(out, 0, sizeof out);
    CHECK(pr_rec_ecl_unpack_text(abcd, 3, out, 2) == 4);
    CHECK(out[0] == 'A' && out[1] == 'B' && out[2] == 0);
    CHECK(pr_rec_ecl_unpack_text(abcd, 3, NULL, 0) == 4);
    return 0;
}

/* The engine's string[255] limit: 200 groups of "ABCD" would be 800 chars. */
static int test_unpack_limit(void) {
    uint8_t packed[600];
    size_t i;
    for (i = 0; i < 600; i += 3) {
        packed[i] = 0x04; packed[i + 1] = 0x20; packed[i + 2] = 0xC4;
    }
    CHECK(pr_rec_ecl_unpack_text(packed, 600, NULL, 0) == PR_REC_ECL_TEXT_MAX);
    return 0;
}

static int test_operand_word(void) {
    unsigned lo, hi;
    uint8_t le[2];
    for (hi = 0; hi < 256; ++hi) {
        for (lo = 0; lo < 256; ++lo) {
            CHECK(pr_rec_ecl_operand_word((uint8_t)lo, (uint8_t)hi) == (uint16_t)((hi << 8) | lo));
        }
    }
    /* A tag-01 operand "01 LO HI" stores LO then HI; the loader passes
     * (LO, HI) and must obtain the little-endian word. */
    le[0] = 0x34; le[1] = 0x12;
    CHECK(pr_rec_ecl_operand_word(le[0], le[1]) == 0x1234);
    CHECK(pr_rec_ecl_operand_word(0xFF, 0xFF) == 0xFFFF);
    CHECK(pr_rec_ecl_operand_word(0x00, 0x00) == 0x0000);
    return 0;
}

static int test_address_class(void) {
    static const struct { uint16_t lo, hi; uint8_t cls; } ranges[4] = {
        {0x4900, 0x4CFF, 0}, {0x6B00, 0x6EFF, 1}, {0x9700, 0x98FF, 2}, {0x9900, 0xB6FF, 3}
    };
    unsigned i;
    unsigned long a;
    for (i = 0; i < 4; ++i) {
        CHECK(pr_rec_ecl_address_class(ranges[i].lo) == ranges[i].cls);
        CHECK(pr_rec_ecl_address_class(ranges[i].hi) == ranges[i].cls);
        CHECK(pr_rec_ecl_address_class((uint16_t)(ranges[i].lo - 1)) != ranges[i].cls);
        CHECK(pr_rec_ecl_address_class((uint16_t)(ranges[i].hi + 1)) != ranges[i].cls);
    }
    /* Exhaustive: every address maps to exactly the range that contains it. */
    for (a = 0; a < 0x10000ul; ++a) {
        uint8_t expect = 4;
        for (i = 0; i < 4; ++i) {
            if (a >= ranges[i].lo && a <= ranges[i].hi) expect = ranges[i].cls;
        }
        CHECK(pr_rec_ecl_address_class((uint16_t)a) == expect);
    }
    CHECK(pr_rec_ecl_address_class(0x98FF) == 2 && pr_rec_ecl_address_class(0x9900) == 3);
    CHECK(pr_rec_ecl_address_class(0x0000) == 4 && pr_rec_ecl_address_class(0xFFFF) == 4);
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_char_remap();
    rc |= test_operand_count();
    rc |= test_unpack_text();
    rc |= test_unpack_limit();
    rc |= test_operand_word();
    rc |= test_address_class();
    if (rc == 0) {
        puts("recovered ECL helper tests passed");
    }
    return rc;
}
