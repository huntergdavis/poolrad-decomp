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

/* ---- operand loader ------------------------------------------------------ */

static void loader_setup(pr_rec_ecl_loader_state *st, const uint8_t *script, size_t size, uint16_t pc) {
    memset(st, 0, sizeof *st);
    st->script = script; st->script_size = size; st->pc = pc;
    pr_rec_ecl_loader_reset(st);
}

static int test_loader_paths(void) {
    /* virtual 0x9900 = script[0]. Instruction at 0x9900 with five operands:
     * 00 05 | 01 34 12 | 02 78 56 | 03 BC 9A | 80 03 04 20 C4 ("ABCD") */
    static const uint8_t s1[] = {0x2A, 0x00,0x05, 0x01,0x34,0x12, 0x02,0x78,0x56, 0x03,0xBC,0x9A, 0x80,0x03,0x04,0x20,0xC4, 0x3D};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s1, sizeof s1, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 5) == PR_REC_ECL_LOADER_OK);
    CHECK(st.tag[1] == 0x00 && st.low[1] == 0x05);
    CHECK(st.tag[2] == 0x01 && st.low[2] == 0x34 && st.high[2] == 0x12);
    CHECK(pr_rec_ecl_operand_word(st.low[2], st.high[2]) == 0x1234);
    CHECK(st.tag[3] == 0x02 && st.low[3] == 0x78 && st.high[3] == 0x56);
    CHECK(st.tag[4] == 0x03 && st.low[4] == 0xBC && st.high[4] == 0x9A);
    CHECK(st.tag[5] == 0x80 && st.low[5] == 0x03);
    CHECK(st.string_count == 1 && st.string_len[1] == 4 && memcmp(st.string_text[1], "ABCD", 4) == 0);
    CHECK(st.pc == 0x9900 + 17);                 /* next opcode: the trailing 0x3D */
    CHECK(s1[st.pc - 0x9900] == 0x3D);
    CHECK(st.tag[0] == 0 && st.tag[6] == 0);     /* slots are 1-based; untouched elsewhere */
    return 0;
}

static int test_loader_count_zero_and_unknown_tag(void) {
    static const uint8_t s2[] = {0x13, 0x05, 0x77, 0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s2, sizeof s2, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 0) == PR_REC_ECL_LOADER_OK);
    CHECK(st.pc == 0x9901);                      /* count 0: only the final PC increment */
    loader_setup(&st, s2, sizeof s2, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.tag[1] == 0x05 && st.low[1] == 0x77 && st.high[1] == 0);
    CHECK(st.pc == 0x9903);                      /* unknown tag: two bytes consumed, then +1 */
    return 0;
}

static int test_loader_empty_string_and_strings_by_address(void) {
    /* 80 00 (empty inline string) ; 81 03 99 (address 0x9903 -> class 3, raw bytes "HI" NUL) ;
     * 81 00 97 (class 2, engine window: unresolved) ; 81 00 4A (class 0: unresolved) ; 81 00 10 (class 4) */
    static const uint8_t s3[] = {0x11, 0x80,0x00, 'H','I',0x00, 0x81,0x03,0x99, 0x81,0x00,0x97, 0x81,0x00,0x4A, 0x81,0x00,0x10, 0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s3, sizeof s3, 0x9900);
    /* operand 1 is the empty string; then skip the raw bytes by loading only one operand */
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.tag[1] == 0x80 && st.string_count == 1 && st.string_len[1] == 0 && st.pc == 0x9903);
    /* now pretend the opcode is at 0x9905 (the byte before the first 0x81) */
    loader_setup(&st, s3, sizeof s3, 0x9905);
    CHECK(pr_rec_ecl_load_operands(&st, 4) == PR_REC_ECL_LOADER_OK);
    CHECK(st.tag[1] == 0x81 && pr_rec_ecl_operand_word(st.low[1], st.high[1]) == 0x9903);
    CHECK(st.string_count == 4);
    CHECK(st.string_len[1] == 2 && memcmp(st.string_text[1], "HI", 2) == 0 && st.string_unresolved[1] == 0);
    CHECK(st.string_address[2] == 0x9700 && st.string_unresolved[2] == 1 && st.string_len[2] == 0);
    CHECK(st.string_address[3] == 0x4A00 && st.string_unresolved[3] == 1);
    CHECK(st.string_address[4] == 0x1000 && st.string_unresolved[4] == 0 && st.string_len[4] == 0);
    CHECK(st.pc == 0x9905 + 12 + 1);
    return 0;
}

static int test_loader_bounds_and_count(void) {
    static const uint8_t s4[] = {0x01, 0x01, 0x34};   /* word operand truncated: no high byte */
    static const uint8_t s5[] = {0x11, 0x80, 0x05, 0x04, 0x20}; /* inline string claims 5 bytes, 2 present */
    static const uint8_t s6[] = {0x11, 0x81, 0x03, 0x99, 'X', 'Y'}; /* class-3 string without NUL */
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s4, sizeof s4, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_BOUNDS);
    loader_setup(&st, s5, sizeof s5, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_BOUNDS);
    loader_setup(&st, s6, sizeof s6, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_BOUNDS);
    loader_setup(&st, s4, sizeof s4, 0x98FF);          /* opcode byte below the base is never read: */
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK && st.pc == 0x9903);
    loader_setup(&st, s4, sizeof s4, 0x98F0);          /* operand bytes below the base */
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_BOUNDS);
    loader_setup(&st, s4, sizeof s4, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, PR_REC_ECL_SLOT_COUNT) == PR_REC_ECL_LOADER_COUNT);
    CHECK(st.pc == 0x9900);                            /* unchanged */
    CHECK(pr_rec_ecl_load_operands(NULL, 1) == PR_REC_ECL_LOADER_ARGUMENT);
    return 0;
}

/* Per-operand advance must equal the format layer's span sizes:
 * tag 00 -> 2 bytes, tags 01/02/03/81 -> 3 bytes, tag 80 -> 2 + LL. */
static int test_loader_matches_span_sizes(void) {
    static const uint8_t ops[] = {0x11, 0x00,0x01, 0x01,0x00,0x99, 0x02,0x01,0x00, 0x03,0x02,0x00, 0x81,0x00,0x10, 0x80,0x02,0x04,0x20, 0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, ops, sizeof ops, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 6) == PR_REC_ECL_LOADER_OK);
    CHECK(st.pc == 0x9900 + 1 + 2 + 3 + 3 + 3 + 3 + 4);
    return 0;
}

/* Regression (PR #20 review): string metadata must not leak between
 * sequential calls that reuse the same state and the same slot k. */
static int test_loader_sequential_calls_reset_metadata(void) {
    /* instruction A at 0x9900: 81 00 97 (class 2 -> unresolved, address 0x9700)
     * instruction B at 0x9904: 80 02 04 20 (inline "AB")
     * instruction C at 0x9909: 81 0D 99 (class 3 -> raw "Z" NUL at 0x990D) */
    static const uint8_t s[] = {0x11, 0x81,0x00,0x97, 0x11, 0x80,0x02,0x04,0x20, 0x11, 0x81,0x0D,0x99, 'Z',0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s, sizeof s, 0x9900);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 1 && st.string_address[1] == 0x9700 && st.pc == 0x9904);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 0 && st.string_address[1] == 0);
    CHECK(st.string_len[1] == 2 && memcmp(st.string_text[1], "AB", 2) == 0 && st.pc == 0x9909);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 0 && st.string_address[1] == 0x990D);
    CHECK(st.string_len[1] == 1 && st.string_text[1][0] == 'Z' && st.pc == 0x990D);
    /* and back to an unresolved one: the raw-string length must be cleared */
    loader_setup(&st, s, sizeof s, 0x9909);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK && st.string_len[1] == 1);
    st.pc = 0x9900;
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_unresolved[1] == 1 && st.string_len[1] == 0 && st.string_address[1] == 0x9700);
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
    rc |= test_loader_paths();
    rc |= test_loader_count_zero_and_unknown_tag();
    rc |= test_loader_empty_string_and_strings_by_address();
    rc |= test_loader_bounds_and_count();
    rc |= test_loader_matches_span_sizes();
    rc |= test_loader_sequential_calls_reset_metadata();
    if (rc == 0) {
        puts("recovered ECL helper tests passed");
    }
    return rc;
}
