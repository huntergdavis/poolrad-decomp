/* Synthetic tests for the recovered ECL helpers. No game data is used; the
 * expected values come from the disassembly transcriptions documented in
 * docs/re/FUNCTION_LEDGER.md and from independently constructed vectors. */

#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_ecl_store_hook.h"

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
    /* windows are zero-filled, so engine-window strings read as empty and resolved */
    CHECK(st.string_address[2] == 0x9700 && st.string_unresolved[2] == 0 && st.string_len[2] == 0);
    CHECK(st.string_address[3] == 0x4A00 && st.string_unresolved[3] == 0 && st.string_len[3] == 0);
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
    /* instruction A at 0x9900: 81 00 97 (class 2 window string at 0x9700)
     * instruction B at 0x9904: 80 02 04 20 (inline "AB")
     * instruction C at 0x9909: 81 0D 99 (class 3 -> raw "Z" NUL at 0x990D) */
    static const uint8_t s[] = {0x11, 0x81,0x00,0x97, 0x11, 0x80,0x02,0x04,0x20, 0x11, 0x81,0x0D,0x99, 'Z',0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s, sizeof s, 0x9900);
    pr_rec_ecl_store_string(&st, 0x9700, "QQ", 2);      /* make the class-2 target non-empty */
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 0 && st.string_address[1] == 0x9700 && st.pc == 0x9904);
    CHECK(st.string_len[1] == 2 && memcmp(st.string_text[1], "QQ", 2) == 0);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 0 && st.string_address[1] == 0);
    CHECK(st.string_len[1] == 2 && memcmp(st.string_text[1], "AB", 2) == 0 && st.pc == 0x9909);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_count == 1 && st.string_unresolved[1] == 0 && st.string_address[1] == 0x990D);
    CHECK(st.string_len[1] == 1 && st.string_text[1][0] == 'Z' && st.pc == 0x990D);
    /* and back to an empty window string: the raw-string length must be cleared */
    loader_setup(&st, s, sizeof s, 0x9909);
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK && st.string_len[1] == 1);
    st.pc = 0x9900;
    pr_rec_ecl_store_string(&st, 0x9700, "", 0);        /* empty again: terminator only */
    CHECK(pr_rec_ecl_load_operands(&st, 1) == PR_REC_ECL_LOADER_OK);
    CHECK(st.string_unresolved[1] == 0 && st.string_len[1] == 0 && st.string_address[1] == 0x9700);
    return 0;
}

/* ---- engine string windows (issue #22) ---------------------------------- */

static int test_windows_round_trip(void) {
    static const uint8_t script[] = {0x11, 'R','A','W',0x00, 0x11};
    uint8_t rw[sizeof script];
    pr_rec_ecl_loader_state st;
    uint16_t v = 0xFFFF;
    loader_setup(&st, script, sizeof script, 0x9900);
    pr_rec_ecl_windows_clear(&st);
    CHECK(st.window0[0] == 0 && st.window1[1023] == 0 && st.window2[511] == 0);
    /* class 0, 1 and 2: one character per word, zero word terminator */
    CHECK(pr_rec_ecl_store_string(&st, 0x4900, "AB", 2) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.window0[0] == 'A' && st.window0[1] == 'B' && st.window0[2] == 0);
    CHECK(pr_rec_ecl_string_by_address(&st, 1, 0x4900) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.string_len[1] == 2 && memcmp(st.string_text[1], "AB", 2) == 0 && st.string_unresolved[1] == 0);
    CHECK(pr_rec_ecl_store_string(&st, 0x6EFD, "XY", 2) == PR_REC_ECL_WINDOW_OK);   /* last three words of window 1 */
    CHECK(st.window1[1021] == 'X' && st.window1[1022] == 'Y' && st.window1[1023] == 0);
    CHECK(pr_rec_ecl_string_by_address(&st, 2, 0x6EFD) == PR_REC_ECL_WINDOW_OK && st.string_len[2] == 2);
    CHECK(pr_rec_ecl_store_string(&st, 0x982C, "NAME", 4) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.window2[0x12C] == 'N' && st.window2[0x130] == 0);
    CHECK(pr_rec_ecl_string_by_address(&st, 3, 0x982C) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.string_len[3] == 4 && memcmp(st.string_text[3], "NAME", 4) == 0);
    /* only the low byte of a word is a character */
    st.window2[0x12C] = 0x0141;
    CHECK(pr_rec_ecl_string_by_address(&st, 3, 0x982C) == PR_REC_ECL_WINDOW_OK && st.string_text[3][0] == 'A');
    /* class 3: raw bytes in the script; stores need script_rw */
    CHECK(pr_rec_ecl_string_by_address(&st, 4, 0x9901) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.string_len[4] == 3 && memcmp(st.string_text[4], "RAW", 3) == 0);
    CHECK(pr_rec_ecl_store_string(&st, 0x9901, "Z", 1) == PR_REC_ECL_WINDOW_UNRESOLVED);
    memcpy(rw, script, sizeof script); st.script = rw; st.script_rw = rw;
    CHECK(pr_rec_ecl_store_string(&st, 0x9901, "Z", 1) == PR_REC_ECL_WINDOW_OK && rw[1] == 'Z' && rw[2] == 0);
    /* class 4: nothing stored, empty string read */
    CHECK(pr_rec_ecl_store_string(&st, 0x1000, "Q", 1) == PR_REC_ECL_WINDOW_OK);
    CHECK(pr_rec_ecl_string_by_address(&st, 5, 0x1000) == PR_REC_ECL_WINDOW_OK && st.string_len[5] == 0);
    /* word load/store cores */
    CHECK(pr_rec_ecl_store_word(&st, 0x9710, 0xBEEF) == PR_REC_ECL_WINDOW_OK);
    CHECK(pr_rec_ecl_load_word(&st, 0x9710, &v) == PR_REC_ECL_WINDOW_OK && v == 0xBEEF);
    CHECK(pr_rec_ecl_store_word(&st, 0x6B40, 7) == PR_REC_ECL_WINDOW_HOOK && st.window1[0x40] == 7);
    CHECK(pr_rec_ecl_load_word(&st, 0x6B40, &v) == PR_REC_ECL_WINDOW_HOOK && v == 7);
    CHECK(pr_rec_ecl_load_word(&st, 0x9901, &v) == PR_REC_ECL_WINDOW_OK && v == 'Z');
    CHECK(pr_rec_ecl_load_word(&st, 0x9902, &v) == PR_REC_ECL_WINDOW_OK && v == 0);
    CHECK(pr_rec_ecl_load_word(&st, 0x00B2, &v) == PR_REC_ECL_WINDOW_UNRESOLVED);
    CHECK(pr_rec_ecl_store_word(&st, 0xC04B, 1) == PR_REC_ECL_WINDOW_UNRESOLVED);
    return 0;
}

static int test_windows_bounds_and_name(void) {
    static const uint8_t script[] = {0x11, 'A', 'B'};   /* no terminator in the block */
    pr_rec_ecl_loader_state st;
    unsigned i;
    loader_setup(&st, script, sizeof script, 0x9900);
    pr_rec_ecl_windows_clear(&st);
    /* a string running off the end of window 2 is a bounds error, not a heap overrun */
    CHECK(pr_rec_ecl_store_string(&st, 0x98FE, "AB", 2) == PR_REC_ECL_WINDOW_BOUNDS);
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) st.window2[i] = 'x';        /* no terminator */
    CHECK(pr_rec_ecl_string_by_address(&st, 1, 0x9700) == PR_REC_ECL_WINDOW_BOUNDS);
    CHECK(pr_rec_ecl_string_by_address(&st, 2, 0x9901) == PR_REC_ECL_WINDOW_BOUNDS);
    CHECK(pr_rec_ecl_load_word(&st, 0x4D00, &st.window0[0]) == PR_REC_ECL_WINDOW_UNRESOLVED); /* class 4 */
    /* 0x6B00: active character name, string[15] */
    CHECK(pr_rec_ecl_string_by_address(&st, 3, 0x6B00) == PR_REC_ECL_WINDOW_UNRESOLVED && st.string_unresolved[3] == 1);
    CHECK(pr_rec_ecl_store_string(&st, 0x6B00, "ABCDEFGHIJKLMNOPQRS", 19) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.active_name_valid == 1 && st.active_name_len == 15);
    CHECK(pr_rec_ecl_string_by_address(&st, 3, 0x6B00) == PR_REC_ECL_WINDOW_OK);
    CHECK(st.string_len[3] == 15 && memcmp(st.string_text[3], "ABCDEFGHIJKLMNO", 15) == 0 && st.string_unresolved[3] == 0);
    CHECK(st.window1[0] == 0);                         /* the name is not a window word */
    CHECK(pr_rec_ecl_string_by_address(&st, 0, 0x9700) == PR_REC_ECL_WINDOW_ARGUMENT);
    CHECK(pr_rec_ecl_store_string(NULL, 0x9700, "a", 1) == PR_REC_ECL_WINDOW_ARGUMENT);
    return 0;
}

/* Regression (PR #25 review): clearing a reused state must also drop the
 * active name, as the initializer nulls [0x5D92]. */
static int test_windows_clear_drops_active_name(void) {
    static const uint8_t script[] = {0x11};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, script, sizeof script, 0x9900);
    pr_rec_ecl_windows_clear(&st);
    CHECK(pr_rec_ecl_store_string(&st, 0x6B00, "HERO", 4) == PR_REC_ECL_WINDOW_OK);
    CHECK(pr_rec_ecl_string_by_address(&st, 1, 0x6B00) == PR_REC_ECL_WINDOW_OK && st.string_len[1] == 4);
    pr_rec_ecl_store_string(&st, 0x9700, "W", 1);
    pr_rec_ecl_windows_clear(&st);
    CHECK(st.active_name_valid == 0 && st.active_name_len == 0 && st.active_name[0] == 0);
    CHECK(st.window2[0] == 0 && st.string_len[1] == 0);
    CHECK(pr_rec_ecl_string_by_address(&st, 1, 0x6B00) == PR_REC_ECL_WINDOW_UNRESOLVED);
    CHECK(st.string_unresolved[1] == 1 && st.string_len[1] == 0);
    return 0;
}

/* ---- short ECL handlers (issue #34) ---------------------------------------- */

/* Transcribed from the handler bytes: GOTO = loader(1) then PC := word(slot 1);
 * GOSUB = loader(1) then push PC and PC := word(slot 1); PRINT RETURN = PC += 1,
 * [0x5E24] = 1, [0x5E25] += 1, and [0x84DF] cleared if it was set. */
static int test_op_goto(void) {
    /* 0x9900: 01 01 10 99  -> GOTO 0x9910 ; trailing bytes must not be consumed */
    static const uint8_t s[] = {0x01, 0x01, 0x10, 0x99, 0x02, 0x01, 0x00, 0x99};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s, sizeof s, 0x9900);
    pr_rec_ecl_windows_clear(&st);
    CHECK(pr_rec_ecl_op_goto(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.pc == 0x9910);
    CHECK(st.tag[1] == 0x01 && st.low[1] == 0x10 && st.high[1] == 0x99 && st.tag[2] == 0);
    CHECK(st.call_depth == 0);
    /* an immediate-byte operand (tag 00) still yields a PC: word(low, high=0) */
    {
        static const uint8_t s2[] = {0x01, 0x00, 0x7F};
        loader_setup(&st, s2, sizeof s2, 0x9900);
        pr_rec_ecl_windows_clear(&st);
        CHECK(pr_rec_ecl_op_goto(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x007F);
    }
    /* truncated operand propagates the loader's bounds failure */
    {
        static const uint8_t s3[] = {0x01, 0x01, 0x10};
        loader_setup(&st, s3, sizeof s3, 0x9900);
        CHECK(pr_rec_ecl_op_goto(&st) == PR_REC_ECL_HANDLER_BOUNDS);
    }
    CHECK(pr_rec_ecl_op_goto(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static int test_op_gosub_and_push_frame(void) {
    /* 0x9900: 02 01 20 99 (GOSUB 0x9920) ; at 0x9920: 02 01 08 99 (GOSUB 0x9908) */
    uint8_t s[0x30];
    pr_rec_ecl_loader_state st;
    memset(s, 0x3D, sizeof s);
    s[0] = 0x02; s[1] = 0x01; s[2] = 0x20; s[3] = 0x99;
    s[0x20] = 0x02; s[0x21] = 0x01; s[0x22] = 0x08; s[0x23] = 0x99;
    loader_setup(&st, s, sizeof s, 0x9900);
    pr_rec_ecl_windows_clear(&st);
    CHECK(pr_rec_ecl_op_gosub(&st) == PR_REC_ECL_HANDLER_OK);
    /* the saved PC is the address after the operand (the loader's final increment) */
    CHECK(st.call_depth == 1 && st.call_stack[0] == 0x9904 && st.pc == 0x9920);
    CHECK(pr_rec_ecl_op_gosub(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.call_depth == 2 && st.call_stack[1] == 0x9924 && st.call_stack[0] == 0x9904 && st.pc == 0x9908);
    /* push_call_frame alone: uses the given slot and the current PC */
    st.pc = 0x9ABC; st.low[3] = 0x34; st.high[3] = 0x12;
    CHECK(pr_rec_ecl_push_call_frame(&st, 3) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.call_depth == 3 && st.call_stack[2] == 0x9ABC && st.pc == 0x1234);
    CHECK(pr_rec_ecl_push_call_frame(&st, 0) == PR_REC_ECL_HANDLER_ARGUMENT);
    CHECK(pr_rec_ecl_push_call_frame(&st, PR_REC_ECL_SLOT_COUNT) == PR_REC_ECL_HANDLER_ARGUMENT);
    /* model bound */
    st.call_depth = PR_REC_ECL_CALL_STACK_MAX;
    CHECK(pr_rec_ecl_push_call_frame(&st, 1) == PR_REC_ECL_HANDLER_STACK_FULL && st.pc == 0x1234);
    /* truncated GOSUB operand: bounds, nothing pushed */
    {
        static const uint8_t s2[] = {0x02, 0x01};
        loader_setup(&st, s2, sizeof s2, 0x9900);
        pr_rec_ecl_windows_clear(&st);
        CHECK(pr_rec_ecl_op_gosub(&st) == PR_REC_ECL_HANDLER_BOUNDS && st.call_depth == 0);
    }
    /* windows_clear empties the list as the initializer does */
    st.call_depth = 5; pr_rec_ecl_windows_clear(&st); CHECK(st.call_depth == 0);
    return 0;
}

static int test_op_print_return(void) {
    static const uint8_t s[] = {0x33, 0x00};
    pr_rec_ecl_loader_state st;
    loader_setup(&st, s, sizeof s, 0x9900);
    /* branch 1: flag clear -> PC+1, b5e24 = 1, b5e25 += 1, flag stays clear */
    st.b84df = 0; st.b5e24 = 0; st.b5e25 = 7;
    CHECK(pr_rec_ecl_op_print_return(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.pc == 0x9901 && st.b5e24 == 1 && st.b5e25 == 8 && st.b84df == 0);
    /* branch 2: flag set -> same window effect and the flag is cleared */
    st.b84df = 0x5A; st.b5e24 = 0;
    CHECK(pr_rec_ecl_op_print_return(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.pc == 0x9902 && st.b5e24 == 1 && st.b5e25 == 9 && st.b84df == 0);
    /* b5e25 is a byte: it wraps like the engine's inc byte */
    st.b5e25 = 0xFF;
    CHECK(pr_rec_ecl_op_print_return(&st) == PR_REC_ECL_HANDLER_OK && st.b5e25 == 0 && st.pc == 0x9903);
    /* no script read happens: PC past the buffer is still fine */
    st.pc = 0xFFFF;
    CHECK(pr_rec_ecl_op_print_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x0000);
    CHECK(pr_rec_ecl_op_print_return(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

/* ---- character-effect dispatch entry 8 (issue #39) ------------------------ */

/* Transcribed from the 15 instructions: three equality tests on record byte
 * 0xA0 (2, 5, 8) lead to the modify block; any other value skips it. */
static int test_u38_entry8(void) {
    uint8_t rec[PR_REC_CHAR_RECORD_SIZE];
    pr_rec_modifier_state m;
    unsigned v, applied;
    memset(rec, 0x11, sizeof rec);
    for (v = 0; v < 256; ++v) {
        unsigned expect = (v == 2 || v == 5 || v == 8) ? 1u : 0u;
        rec[PR_REC_CHAR_OFFSET_A0] = (uint8_t)v;
        m.b6816 = 10; m.b6822 = 10;
        CHECK(pr_rec_u38_entry8(&m, rec, &applied) == PR_REC_EFFECT_OK);
        CHECK(applied == expect);
        CHECK(m.b6816 == (expect ? 12 : 10) && m.b6822 == (expect ? 8 : 10));
    }
    /* byte wraparound of add/sub byte */
    rec[PR_REC_CHAR_OFFSET_A0] = 5; m.b6816 = 0xFF; m.b6822 = 0x01;
    CHECK(pr_rec_u38_entry8(&m, rec, NULL) == PR_REC_EFFECT_OK && m.b6816 == 0x01 && m.b6822 == 0xFF);
    /* repeated application accumulates; other record bytes are irrelevant */
    rec[PR_REC_CHAR_OFFSET_A0] = 8; m.b6816 = 0; m.b6822 = 0;
    memset(rec, 0x02, PR_REC_CHAR_OFFSET_A0);   /* bytes before 0xA0 set to a trigger value: must not matter */
    CHECK(pr_rec_u38_entry8(&m, rec, &applied) == PR_REC_EFFECT_OK && applied == 1);
    CHECK(pr_rec_u38_entry8(&m, rec, &applied) == PR_REC_EFFECT_OK && applied == 1);
    CHECK(m.b6816 == 4 && m.b6822 == 0xFC);
    /* null contracts: nothing changes, applied cleared */
    applied = 7;
    CHECK(pr_rec_u38_entry8(NULL, rec, &applied) == PR_REC_EFFECT_ARGUMENT && applied == 0);
    CHECK(pr_rec_u38_entry8(&m, NULL, &applied) == PR_REC_EFFECT_ARGUMENT && applied == 0);
    CHECK(m.b6816 == 4 && m.b6822 == 0xFC);
    return 0;
}

/* ---- character-effect dispatch entry 9 (issue #43) ------------------------ */

/* Transcribed independently from the 15 instructions at +0x03AC: equality
 * tests against 0, 3, 6 on record byte 0xA0 lead to the modify block. */
static int test_u38_entry9(void) {
    uint8_t rec[PR_REC_CHAR_RECORD_SIZE];
    pr_rec_modifier_state m;
    unsigned v, applied;
    memset(rec, 0x11, sizeof rec);
    for (v = 0; v < 256; ++v) {
        unsigned expect = (v == 0 || v == 3 || v == 6) ? 1u : 0u;
        rec[PR_REC_CHAR_OFFSET_A0] = (uint8_t)v;
        m.b6816 = 10; m.b6822 = 10;
        CHECK(pr_rec_u38_entry9(&m, rec, &applied) == PR_REC_EFFECT_OK);
        CHECK(applied == expect);
        CHECK(m.b6816 == (expect ? 12 : 10) && m.b6822 == (expect ? 8 : 10));
    }
    /* the two siblings partition their trigger sets: no byte value triggers both */
    for (v = 0; v < 256; ++v) {
        unsigned a8, a9;
        rec[PR_REC_CHAR_OFFSET_A0] = (uint8_t)v; m.b6816 = 0; m.b6822 = 0;
        pr_rec_u38_entry8(&m, rec, &a8); pr_rec_u38_entry9(&m, rec, &a9);
        CHECK(!(a8 && a9));
    }
    /* byte wraparound */
    rec[PR_REC_CHAR_OFFSET_A0] = 3; m.b6816 = 0xFE; m.b6822 = 0x00;
    CHECK(pr_rec_u38_entry9(&m, rec, NULL) == PR_REC_EFFECT_OK && m.b6816 == 0x00 && m.b6822 == 0xFE);
    /* accumulation and insensitivity to other bytes */
    rec[PR_REC_CHAR_OFFSET_A0] = 6; memset(rec, 0x00, PR_REC_CHAR_OFFSET_A0); m.b6816 = 1; m.b6822 = 1;
    CHECK(pr_rec_u38_entry9(&m, rec, &applied) == PR_REC_EFFECT_OK && applied == 1);
    CHECK(pr_rec_u38_entry9(&m, rec, &applied) == PR_REC_EFFECT_OK && applied == 1);
    CHECK(m.b6816 == 5 && m.b6822 == 0xFD);
    /* host invalid input */
    applied = 9;
    CHECK(pr_rec_u38_entry9(NULL, rec, &applied) == PR_REC_EFFECT_ARGUMENT && applied == 0);
    CHECK(pr_rec_u38_entry9(&m, NULL, &applied) == PR_REC_EFFECT_ARGUMENT && applied == 0);
    CHECK(m.b6816 == 5 && m.b6822 == 0xFD);
    return 0;
}

/* ---- character-effect dispatch entry 10 (issue #46) ----------------------- */

/* Transcribed from the 13 instructions at +0x03E1: only bit 1 of [0x6819]
 * selects the effect; [0x6818] is halved as a byte and [0x6816] gains 3. */
static int test_u38_entry10(void) {
    pr_rec_modifier_state m;
    unsigned v, w, applied;
    for (v = 0; v < 256; ++v) {
        unsigned expect = (v & 2u) ? 1u : 0u;
        memset(&m, 0, sizeof m);
        m.b6819 = (uint8_t)v; m.b6818 = 0x81; m.b6816 = 0x10; m.b6822 = 0x55;
        CHECK(pr_rec_u38_entry10(&m, &applied) == PR_REC_EFFECT_OK);
        CHECK(applied == expect);
        CHECK(m.b6818 == (expect ? 0x40 : 0x81) && m.b6816 == (expect ? 0x13 : 0x10));
        CHECK(m.b6822 == 0x55 && m.b6819 == (uint8_t)v);   /* untouched */
    }
    /* the shift is on the zero-extended byte: every value halves without a carry-in */
    for (w = 0; w < 256; ++w) {
        memset(&m, 0, sizeof m); m.b6819 = 2; m.b6818 = (uint8_t)w;
        CHECK(pr_rec_u38_entry10(&m, NULL) == PR_REC_EFFECT_OK && m.b6818 == (uint8_t)(w >> 1));
    }
    /* byte wraparound of the +3 and repeated halving to zero */
    memset(&m, 0, sizeof m); m.b6819 = 0xFF; m.b6816 = 0xFE; m.b6818 = 0x05;
    CHECK(pr_rec_u38_entry10(&m, &applied) == PR_REC_EFFECT_OK && applied == 1 && m.b6816 == 0x01 && m.b6818 == 0x02);
    CHECK(pr_rec_u38_entry10(&m, &applied) == PR_REC_EFFECT_OK && m.b6816 == 0x04 && m.b6818 == 0x01);
    CHECK(pr_rec_u38_entry10(&m, &applied) == PR_REC_EFFECT_OK && m.b6816 == 0x07 && m.b6818 == 0x00);
    CHECK(pr_rec_u38_entry10(&m, &applied) == PR_REC_EFFECT_OK && m.b6816 == 0x0A && m.b6818 == 0x00);
    /* host invalid input */
    applied = 3;
    CHECK(pr_rec_u38_entry10(NULL, &applied) == PR_REC_EFFECT_ARGUMENT && applied == 0);
    /* entries 8 and 9 leave the new bytes alone */
    {
        uint8_t rec[PR_REC_CHAR_RECORD_SIZE]; memset(rec, 0, sizeof rec); rec[PR_REC_CHAR_OFFSET_A0] = 2;
        memset(&m, 0, sizeof m); m.b6818 = 0x33; m.b6819 = 0x44;
        CHECK(pr_rec_u38_entry8(&m, rec, NULL) == PR_REC_EFFECT_OK && m.b6818 == 0x33 && m.b6819 == 0x44);
        rec[PR_REC_CHAR_OFFSET_A0] = 3;
        CHECK(pr_rec_u38_entry9(&m, rec, NULL) == PR_REC_EFFECT_OK && m.b6818 == 0x33 && m.b6819 == 0x44);
    }
    return 0;
}

/* ---- character-effect dispatch entry 31 (issue #49) ----------------------- */

/* Transcribed from the 6 instructions at +0x0C15: unconditional byte
 * subtractions of 4 from [0x6822] and [0x6816]; nothing else changes. */
static int test_u38_entry31(void) {
    pr_rec_modifier_state m;
    unsigned v;
    for (v = 0; v < 256; ++v) {
        memset(&m, 0, sizeof m);
        m.b6816 = (uint8_t)v; m.b6822 = (uint8_t)(255 - v); m.b6818 = 0x77; m.b6819 = 0x88;
        CHECK(pr_rec_u38_entry31(&m) == PR_REC_EFFECT_OK);
        CHECK(m.b6816 == (uint8_t)(v - 4) && m.b6822 == (uint8_t)(255 - v - 4));
        CHECK(m.b6818 == 0x77 && m.b6819 == 0x88);
    }
    /* wraparound through zero and accumulation */
    memset(&m, 0, sizeof m); m.b6816 = 3; m.b6822 = 0;
    CHECK(pr_rec_u38_entry31(&m) == PR_REC_EFFECT_OK && m.b6816 == 0xFF && m.b6822 == 0xFC);
    CHECK(pr_rec_u38_entry31(&m) == PR_REC_EFFECT_OK && m.b6816 == 0xFB && m.b6822 == 0xF8);
    /* interaction with entry 8: +2 then -4 nets -2 */
    {
        uint8_t rec[PR_REC_CHAR_RECORD_SIZE]; memset(rec, 0, sizeof rec); rec[PR_REC_CHAR_OFFSET_A0] = 5;
        memset(&m, 0, sizeof m); m.b6816 = 10; m.b6822 = 10;
        CHECK(pr_rec_u38_entry8(&m, rec, NULL) == PR_REC_EFFECT_OK);
        CHECK(pr_rec_u38_entry31(&m) == PR_REC_EFFECT_OK && m.b6816 == 8 && m.b6822 == 4);
    }
    CHECK(pr_rec_u38_entry31(NULL) == PR_REC_EFFECT_ARGUMENT);
    return 0;
}

/* ---- character-effect dispatch entry 30 (issue #51) ----------------------- */

/* Transcribed from the 6 instructions at +0x0C06: unconditional byte
 * increments of [0x6816] and [0x6822]; nothing else changes. */
static int test_u38_entry30(void) {
    pr_rec_modifier_state m;
    unsigned v;
    for (v = 0; v < 256; ++v) {
        memset(&m, 0, sizeof m);
        m.b6816 = (uint8_t)v; m.b6822 = (uint8_t)(255 - v); m.b6818 = 0x55; m.b6819 = 0x66;
        CHECK(pr_rec_u38_entry30(&m) == PR_REC_EFFECT_OK);
        CHECK(m.b6816 == (uint8_t)(v + 1) && m.b6822 == (uint8_t)(255 - v + 1));
        CHECK(m.b6818 == 0x55 && m.b6819 == 0x66);
    }
    /* wraparound at 0xFF and accumulation */
    memset(&m, 0, sizeof m); m.b6816 = 0xFF; m.b6822 = 0xFE;
    CHECK(pr_rec_u38_entry30(&m) == PR_REC_EFFECT_OK && m.b6816 == 0x00 && m.b6822 == 0xFF);
    CHECK(pr_rec_u38_entry30(&m) == PR_REC_EFFECT_OK && m.b6816 == 0x01 && m.b6822 == 0x00);
    /* composition with entry 31: +1 then -4 nets -3 on both bytes */
    memset(&m, 0, sizeof m); m.b6816 = 10; m.b6822 = 20;
    CHECK(pr_rec_u38_entry30(&m) == PR_REC_EFFECT_OK);
    CHECK(pr_rec_u38_entry31(&m) == PR_REC_EFFECT_OK && m.b6816 == 7 && m.b6822 == 17);
    CHECK(pr_rec_u38_entry30(NULL) == PR_REC_EFFECT_ARGUMENT);
    return 0;
}

/* ---- issue #56: view-parameter setter and script-state initializer --------- */

static int test_u3bd_entry1(void) {
    pr_rec_view_state v;
    unsigned i;
    memset(&v, 0xEE, sizeof v);
    /* the program body's call (image 0x012C..0x0138): pushes 0x0B, 0, 7, 6 */
    CHECK(pr_rec_u3bd_entry1(&v, 0x0B, 0, 7, 6) == PR_REC_EFFECT_OK);
    CHECK(v.b6a60 == 0x0B && v.b6a61 == 0 && v.b6a62 == 7 && v.b6a63 == 6);
    for (i = 0; i < 256; ++i) {
        uint8_t a = (uint8_t)i, b = (uint8_t)(255 - i), c = (uint8_t)(i ^ 0x5A), d = (uint8_t)(i * 3);
        CHECK(pr_rec_u3bd_entry1(&v, a, b, c, d) == PR_REC_EFFECT_OK);
        CHECK(v.b6a60 == a && v.b6a61 == b && v.b6a62 == c && v.b6a63 == d);
    }
    /* the view refresh pushes the four bytes back in the same order: identity */
    CHECK(pr_rec_u3bd_entry1(&v, v.b6a60, v.b6a61, v.b6a62, v.b6a63) == PR_REC_EFFECT_OK);
    CHECK(v.b6a60 == 255 && v.b6a61 == 0 && v.b6a62 == (255 ^ 0x5A) && v.b6a63 == (uint8_t)(255 * 3));
    CHECK(pr_rec_u3bd_entry1(NULL, 1, 2, 3, 4) == PR_REC_EFFECT_ARGUMENT);
    return 0;
}

/* Five block-head entries as ECL_CONTRACTS.md section 1 describes them:
 * opcode 0x01, tag 0x01, word, at unpacked offsets 2, 6, 10, 14, 18. */
static const uint8_t k_block_head[] = {
    0x01, 0x01, 0x34, 0x12,  0x01, 0x01, 0x78, 0x56,  0x01, 0x01, 0xBC, 0x9A,
    0x01, 0x01, 0x00, 0x00,  0x01, 0x01, 0xFF, 0xFF,  0x00
};

static void init_fill_state(pr_rec_ecl_loader_state *st, const uint8_t *script, size_t size) {
    unsigned i;
    memset(st, 0, sizeof *st);
    st->script = script; st->script_size = size; st->pc = 0x1234;
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) st->window0[i] = 0xAAAA;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) st->window1[i] = 0xAAAA;
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) st->window2[i] = 0xAAAA;
    for (i = 0; i < 5; ++i) st->entry_addr[i] = 0x5555;
    memset(st->cmp_flags, 0xAA, sizeof st->cmp_flags);
    st->call_depth = 3; st->w6f74 = 0xAAAA; st->w6f76 = 0xAAAA;
    st->b84e0 = st->b84e2 = st->b84d2 = st->b84d3 = st->b84e5 = 0xAA;
    st->b6f68 = st->b6f69 = st->b84c8 = st->b84c9 = st->b6f83 = st->b70c6 = 0xAA;
    st->b84df = 0x11; st->b5e24 = 0x22; st->b5e25 = 0x33;
}

static int check_init_fixed_writes(const pr_rec_ecl_loader_state *st) {
    unsigned i;
    CHECK(st->entry_addr[0] == 0x1234 && st->entry_addr[1] == 0x5678 && st->entry_addr[2] == 0x9ABC);
    CHECK(st->entry_addr[3] == 0x0000 && st->entry_addr[4] == 0xFFFF);
    CHECK(st->pc == PR_REC_ECL_SCRIPT_BASE + 20u);      /* five 4-byte entries consumed */
    CHECK(st->call_depth == 0 && st->w6f74 == 0 && st->w6f76 == 0);
    for (i = 0; i < 6; ++i) CHECK(st->cmp_flags[i] == 0);
    CHECK(st->b84e0 == 0 && st->b84e2 == 0 && st->b84d2 == 0 && st->b84d3 == 0);
    CHECK(st->b6f68 == 0x41 && st->b6f69 == 9 && st->b84e5 == 1);
    CHECK(st->b84c8 == 0 && st->b84c9 == 0 && st->b6f83 == 8 && st->b70c6 == 0);
    CHECK(st->window1[0x2E1] == 0xFF && st->window1[0x2D2] == 0 && st->window1[0x2D3] == 0);
    CHECK(st->window0[0xE5] == 0 && st->window0[0xE6] == 1);
    CHECK(st->b84df == 0x11 && st->b5e24 == 0x22 && st->b5e25 == 0x33);   /* not touched */
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) CHECK(st->window2[i] == 0xAAAA);
    return 0;
}

static int test_script_init(void) {
    pr_rec_ecl_loader_state st;
    unsigned i;
    /* new game: restored == 0, the two window ranges are cleared */
    init_fill_state(&st, k_block_head, sizeof k_block_head);
    st.restored = 0;
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(check_init_fixed_writes(&st) == 0);
    CHECK(st.restored == 0);
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) {
        uint16_t want = 0xAAAA;
        if (i >= 0x100 && i < 0x120) want = 0;
        if (i == 0xE5) want = 0;
        if (i == 0xE6) want = 1;
        CHECK(st.window0[i] == want);
    }
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) {
        uint16_t want = 0xAAAA;
        if (i >= 0x379 && i < 0x383) want = 0;
        if (i == 0x2E1) want = 0xFF;
        if (i == 0x2D2 || i == 0x2D3) want = 0;
        CHECK(st.window1[i] == want);
    }
    /* restored game: the two ranges are kept and the flag is consumed */
    init_fill_state(&st, k_block_head, sizeof k_block_head);
    st.restored = 1;
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(check_init_fixed_writes(&st) == 0);
    CHECK(st.restored == 0);
    for (i = 0x100; i < 0x120; ++i) CHECK(st.window0[i] == 0xAAAA);
    for (i = 0x379; i < 0x383; ++i) CHECK(st.window1[i] == 0xAAAA);
    /* a truncated block head: the loader's bounds status propagates after three entries */
    init_fill_state(&st, k_block_head, 14);
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_BOUNDS);
    CHECK(st.entry_addr[0] == 0x1234 && st.entry_addr[1] == 0x5678 && st.entry_addr[2] == 0x9ABC);
    CHECK(st.entry_addr[3] == 0x5555 && st.entry_addr[4] == 0x5555);
    CHECK(st.window0[0xE6] == 0xAAAA);                    /* the write after the reads did not happen */
    CHECK(pr_rec_ecl_script_init(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

/* ---- issue #68: ECL block load ------------------------------------------- */

typedef struct {
    char log[64];                 /* call sequence: C clear_row, W show_wait, L load, R release */
    unsigned n;
    char names[8][PR_REC_ECL_BLOCK_NAME_MAX];
    char exts[8][8];
    uint8_t ids[8];
    unsigned loads;
    /* scripted results per load call */
    uint16_t sizes[8];
    const uint8_t *blocks[8];
    int rcs[8];
    /* release record */
    unsigned releases;
    const uint8_t *released_block;
    uint16_t released_size;
} fake_io_t;

static void fio_log(fake_io_t *f, char c) { if (f->n < sizeof f->log - 1) { f->log[f->n++] = c; f->log[f->n] = 0; } }
static int fio_load(void *ctx, const char *name, const char *ext, uint8_t id, uint16_t *size, const uint8_t **block) {
    fake_io_t *f = (fake_io_t *)ctx; unsigned i = f->loads < 8 ? f->loads : 7;
    fio_log(f, 'L');
    strncpy(f->names[i], name, PR_REC_ECL_BLOCK_NAME_MAX - 1); f->names[i][PR_REC_ECL_BLOCK_NAME_MAX - 1] = 0;
    strncpy(f->exts[i], ext, 7); f->exts[i][7] = 0; f->ids[i] = id;
    *size = f->sizes[i]; *block = f->blocks[i]; f->loads++;
    return f->rcs[i];
}
static void fio_release(void *ctx, const uint8_t *block, uint16_t size) {
    fake_io_t *f = (fake_io_t *)ctx; fio_log(f, 'R'); f->releases++; f->released_block = block; f->released_size = size;
}
static void fio_clear(void *ctx) { fio_log((fake_io_t *)ctx, 'C'); }
static void fio_wait(void *ctx) { fio_log((fake_io_t *)ctx, 'W'); }

static void fio_reset(fake_io_t *f, pr_rec_ecl_block_io *io) {
    memset(f, 0, sizeof *f);
    io->ctx = f; io->load_block = fio_load; io->release = fio_release; io->clear_row = fio_clear; io->show_wait = fio_wait;
}

static int test_block_name(void) {
    char name[PR_REC_ECL_BLOCK_NAME_MAX];
    pr_rec_ecl_block_name(3, name);   CHECK(strcmp(name, "ECL3") == 0);
    pr_rec_ecl_block_name(0, name);   CHECK(strcmp(name, "ECL0") == 0);
    pr_rec_ecl_block_name(10, name);  CHECK(strcmp(name, "ECL10") == 0);
    pr_rec_ecl_block_name(255, name); CHECK(strcmp(name, "ECL255") == 0);
    CHECK(strcmp(PR_REC_ECL_BLOCK_EXT, ".dax") == 0);
    pr_rec_ecl_block_name(1, NULL);   /* tolerated */
    return 0;
}

static int test_block_load(void) {
    static uint8_t buf[PR_REC_ECL_SCRIPT_BUFFER_SIZE + 16];
    static uint8_t blk[PR_REC_ECL_SCRIPT_BUFFER_SIZE + 2 + 8];
    static uint8_t tiny[1];
    pr_rec_ecl_loader_state st;
    fake_io_t f; pr_rec_ecl_block_io io;
    unsigned attempts, i;
    for (i = 0; i < sizeof blk; ++i) blk[i] = (uint8_t)(0x11 + i * 7);
    /* name selection, copy offset and length, release order, state binding */
    memset(buf, 0xCC, sizeof buf);
    memset(&st, 0, sizeof st); st.file_number = 3; st.b557c = 0xAA;
    fio_reset(&f, &io); f.sizes[0] = 100; f.blocks[0] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 0x12, &io, 4, &attempts) == PR_REC_ECL_BLOCK_OK);
    CHECK(attempts == 1 && f.loads == 1 && f.ids[0] == 0x12);
    CHECK(strcmp(f.names[0], "ECL3") == 0 && strcmp(f.exts[0], ".dax") == 0);
    CHECK(strcmp(f.log, "CWLRC") == 0);                       /* clear, wait, load, release, clear */
    CHECK(f.releases == 1 && f.released_block == blk && f.released_size == 100);
    for (i = 0; i < 98; ++i) CHECK(buf[i] == blk[2 + i]);      /* size - 2 bytes from offset 2 */
    for (i = 98; i < PR_REC_ECL_SCRIPT_BUFFER_SIZE; ++i) CHECK(buf[i] == 0);   /* the fill */
    for (i = PR_REC_ECL_SCRIPT_BUFFER_SIZE; i < sizeof buf; ++i) CHECK(buf[i] == 0xCC); /* nothing past 0x1E00 */
    CHECK(st.script == buf && st.script_rw == buf && st.script_size == PR_REC_ECL_SCRIPT_BUFFER_SIZE);
    CHECK(st.b557c == 0);
    /* retry: size 0, then size 1 with a block (not released), then success */
    memset(&st, 0, sizeof st); st.file_number = 7;
    fio_reset(&f, &io);
    f.sizes[0] = 0; f.blocks[0] = NULL; f.sizes[1] = 1; f.blocks[1] = tiny; f.sizes[2] = 2; f.blocks[2] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 5, &io, 8, &attempts) == PR_REC_ECL_BLOCK_OK);
    CHECK(attempts == 3 && f.loads == 3);
    CHECK(strcmp(f.log, "CWLCWLCWLRC") == 0);
    CHECK(f.releases == 1 && f.released_block == blk && f.released_size == 2);
    CHECK(strcmp(f.names[2], "ECL7") == 0 && f.ids[2] == 5);
    for (i = 0; i < PR_REC_ECL_SCRIPT_BUFFER_SIZE; ++i) CHECK(buf[i] == 0);   /* size 2 copies nothing */
    /* a failing callback counts as size 0 */
    memset(&st, 0, sizeof st);
    fio_reset(&f, &io); f.rcs[0] = -1; f.sizes[0] = 50; f.blocks[0] = blk; f.sizes[1] = 10; f.blocks[1] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 8, &attempts) == PR_REC_ECL_BLOCK_OK && attempts == 2);
    CHECK(f.releases == 1 && f.released_size == 10);
    /* retry limit: the engine would loop forever */
    memset(&st, 0, sizeof st);
    fio_reset(&f, &io);
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 3, &attempts) == PR_REC_ECL_BLOCK_RETRY_LIMIT);
    CHECK(attempts == 4 && f.loads == 3 && f.releases == 0 && strcmp(f.log, "CWLCWLCWLCW") == 0);
    CHECK(st.script == NULL);
    /* retry limit with attempts == NULL: the bound must still hold (PR #72 review) */
    memset(&st, 0, sizeof st);
    fio_reset(&f, &io);
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 3, NULL) == PR_REC_ECL_BLOCK_RETRY_LIMIT);
    CHECK(f.loads == 3 && f.releases == 0 && strcmp(f.log, "CWLCWLCWLCW") == 0);
    fio_reset(&f, &io);
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 0, NULL) == PR_REC_ECL_BLOCK_RETRY_LIMIT && f.loads == 0);
    /* bounds: exactly the buffer fits; one byte more does not */
    memset(&st, 0, sizeof st); memset(buf, 0xCC, sizeof buf);
    fio_reset(&f, &io); f.sizes[0] = PR_REC_ECL_SCRIPT_BUFFER_SIZE + 2; f.blocks[0] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 2, &attempts) == PR_REC_ECL_BLOCK_OK);
    for (i = 0; i < PR_REC_ECL_SCRIPT_BUFFER_SIZE; ++i) CHECK(buf[i] == blk[2 + i]);
    CHECK(buf[PR_REC_ECL_SCRIPT_BUFFER_SIZE] == 0xCC);
    memset(&st, 0, sizeof st); memset(buf, 0xCC, sizeof buf);
    fio_reset(&f, &io); f.sizes[0] = PR_REC_ECL_SCRIPT_BUFFER_SIZE + 3; f.blocks[0] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 2, &attempts) == PR_REC_ECL_BLOCK_OVERFLOW);
    CHECK(strcmp(f.log, "CWLRC") == 0 && f.releases == 1 && f.released_size == PR_REC_ECL_SCRIPT_BUFFER_SIZE + 3);
    for (i = 0; i < PR_REC_ECL_SCRIPT_BUFFER_SIZE; ++i) CHECK(buf[i] == 0);   /* filled, not copied */
    CHECK(st.script == NULL);
    /* optional display callbacks absent */
    memset(&st, 0, sizeof st);
    fio_reset(&f, &io); io.clear_row = NULL; io.show_wait = NULL; f.sizes[0] = 4; f.blocks[0] = blk;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 2, NULL) == PR_REC_ECL_BLOCK_OK && strcmp(f.log, "LR") == 0);
    /* composition with the initializer: a block whose payload is the five-entry head */
    {
        static uint8_t head_block[2 + sizeof k_block_head];
        memcpy(head_block + 2, k_block_head, sizeof k_block_head);
        memset(&st, 0, sizeof st); st.file_number = 3;
        fio_reset(&f, &io); f.sizes[0] = (uint16_t)sizeof head_block; f.blocks[0] = head_block;
        CHECK(pr_rec_ecl_load_block(&st, buf, 0, &io, 2, NULL) == PR_REC_ECL_BLOCK_OK);
        CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
        CHECK(st.entry_addr[0] == 0x1234 && st.entry_addr[4] == 0xFFFF && st.pc == PR_REC_ECL_SCRIPT_BASE + 20u);
    }
    /* arguments */
    fio_reset(&f, &io);
    CHECK(pr_rec_ecl_load_block(NULL, buf, 1, &io, 2, &attempts) == PR_REC_ECL_BLOCK_ARGUMENT && attempts == 0);
    CHECK(pr_rec_ecl_load_block(&st, NULL, 1, &io, 2, NULL) == PR_REC_ECL_BLOCK_ARGUMENT);
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, NULL, 2, NULL) == PR_REC_ECL_BLOCK_ARGUMENT);
    io.load_block = NULL;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 2, NULL) == PR_REC_ECL_BLOCK_ARGUMENT);
    fio_reset(&f, &io); io.release = NULL;
    CHECK(pr_rec_ecl_load_block(&st, buf, 1, &io, 2, NULL) == PR_REC_ECL_BLOCK_ARGUMENT);
    return 0;
}

/* ---- issue #75: run loop and dispatcher -------------------------------------- */

typedef struct {
    unsigned calls;
    uint8_t last_opcode;          /* latch seen by the handler */
    uint8_t last_prev;
    uint16_t last_pc;
    unsigned slot_hits[PR_REC_ECL_OPCODE_COUNT];
    uint8_t stop_after;           /* when calls reaches this, set [0x442E] */
    uint8_t stop_with_49ff;
    pr_rec_ecl_handler_status rc;
} rec_ctx_t;

static pr_rec_ecl_handler_status rec_handler(pr_rec_ecl_loader_state *st, void *ctx) {
    rec_ctx_t *r = (rec_ctx_t *)ctx;
    r->calls++; r->last_opcode = st->latch; r->last_prev = st->prev_opcode; r->last_pc = st->pc;
    if (st->latch < PR_REC_ECL_OPCODE_COUNT) r->slot_hits[st->latch]++;
    st->pc++;                                                   /* a zero-operand handler advances past its opcode */
    if (r->stop_after != 0 && r->calls >= r->stop_after) { if (r->stop_with_49ff) st->b49ff = 1; else st->b442e = 1; }
    return r->rc;
}
static pr_rec_ecl_handler_status goto_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_goto(st); }
static pr_rec_ecl_handler_status exit_adapter(pr_rec_ecl_loader_state *st, void *ctx) {
    rec_ctx_t *r = (rec_ctx_t *)ctx; r->calls++; r->last_pc = st->pc; st->b442e = 1; return PR_REC_ECL_HANDLER_OK; }

static void fill_table(pr_rec_ecl_dispatch_table *tb, rec_ctx_t *r) {
    unsigned i; memset(tb, 0, sizeof *tb); tb->ctx = r;
    for (i = 0; i < PR_REC_ECL_OPCODE_COUNT; ++i) tb->handler[i] = rec_handler;
}

static int test_handler_offsets(void) {
    unsigned i, distinct = 0; uint16_t seen[PR_REC_ECL_OPCODE_COUNT]; unsigned n = 0;
    static const struct { uint8_t op; uint16_t off; } spot[] = {
        {0x00, 0x0001}, {0x01, 0x00F4}, {0x02, 0x0111}, {0x03, 0x0126}, {0x08, 0x0234}, {0x13, 0x0A9F},
        {0x20, 0x0BD1}, {0x24, 0x179B}, {0x2D, 0x2CCF}, {0x33, 0x2AEB}, {0x38, 0x2E10}, {0x3D, 0x2B14} };
    CHECK(pr_rec_ecl_handler_offset[0x1F] == 0);
    for (i = 0; i < PR_REC_ECL_OPCODE_COUNT; ++i) {
        if (i != 0x1F) CHECK(pr_rec_ecl_handler_offset[i] != 0);
    }
    for (i = 0; i < sizeof spot / sizeof spot[0]; ++i) CHECK(pr_rec_ecl_handler_offset[spot[i].op] == spot[i].off);
    /* shared handlers, as the chain's je ladders show */
    CHECK(pr_rec_ecl_handler_offset[0x04] == pr_rec_ecl_handler_offset[0x05] && pr_rec_ecl_handler_offset[0x05] == pr_rec_ecl_handler_offset[0x06] && pr_rec_ecl_handler_offset[0x06] == pr_rec_ecl_handler_offset[0x07]);
    CHECK(pr_rec_ecl_handler_offset[0x11] == pr_rec_ecl_handler_offset[0x12]);
    for (i = 0x16; i <= 0x1B; ++i) CHECK(pr_rec_ecl_handler_offset[i] == pr_rec_ecl_handler_offset[0x16]);
    CHECK(pr_rec_ecl_handler_offset[0x21] == pr_rec_ecl_handler_offset[0x37]);
    CHECK(pr_rec_ecl_handler_offset[0x25] == pr_rec_ecl_handler_offset[0x26]);
    CHECK(pr_rec_ecl_handler_offset[0x2F] == pr_rec_ecl_handler_offset[0x30]);
    for (i = 0; i < PR_REC_ECL_OPCODE_COUNT; ++i) {
        unsigned j, dup = 0; uint16_t v = pr_rec_ecl_handler_offset[i];
        if (v == 0) continue;
        for (j = 0; j < n; ++j) if (seen[j] == v) dup = 1;
        if (!dup) seen[n++] = v;
    }
    distinct = n;
    CHECK(distinct == 49);
    return 0;
}

static int test_dispatch_all_opcodes(void) {
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned v;
    pr_rec_ecl_handler_status hs;
    memset(&st, 0, sizeof st); memset(&r, 0, sizeof r); fill_table(&tb, &r);
    for (v = 0; v < 256; ++v) {
        unsigned before = r.calls;
        st.latch = (uint8_t)v; st.pc = 0x9900;
        if (v == 0x1F || v > 0x3D) {
            CHECK(pr_rec_ecl_dispatch(&st, &tb, &hs) == PR_REC_ECL_RUN_NO_CASE);
            CHECK(r.calls == before && st.pc == 0x9900 && hs == PR_REC_ECL_HANDLER_OK);
        } else {
            CHECK(pr_rec_ecl_dispatch(&st, &tb, &hs) == PR_REC_ECL_RUN_OK);
            CHECK(r.calls == before + 1 && r.last_opcode == v && r.slot_hits[v] == 1);
        }
    }
    /* an unmodelled slot and a failing handler */
    tb.handler[0x24] = NULL; st.latch = 0x24;
    CHECK(pr_rec_ecl_dispatch(&st, &tb, NULL) == PR_REC_ECL_RUN_UNMODELLED);
    r.rc = PR_REC_ECL_HANDLER_BOUNDS; st.latch = 0x03;
    CHECK(pr_rec_ecl_dispatch(&st, &tb, &hs) == PR_REC_ECL_RUN_HANDLER && hs == PR_REC_ECL_HANDLER_BOUNDS);
    CHECK(pr_rec_ecl_dispatch(NULL, &tb, &hs) == PR_REC_ECL_RUN_ARGUMENT);
    CHECK(pr_rec_ecl_dispatch(&st, NULL, NULL) == PR_REC_ECL_RUN_ARGUMENT);
    return 0;
}

static int test_run_loop(void) {
    /* zero-operand opcodes in a row: 0x0D, 0x13, 0x31, 0x3D, then 0x00 */
    static const uint8_t script[] = { 0x0D, 0x13, 0x31, 0x3D, 0x00, 0x0D, 0x0D, 0x0D };
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps;
    pr_rec_ecl_handler_status hs;
    /* PC set, stop byte cleared on entry, fetch bias, latch and previous opcode, stop by [0x442E] */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    st.latch = 0x77; st.b442e = 1;                              /* a stale stop byte is cleared first */
    memset(&r, 0, sizeof r); fill_table(&tb, &r); r.stop_after = 4;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 100, &steps, &hs) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 4 && r.calls == 4);
    CHECK(r.slot_hits[0x0D] == 1 && r.slot_hits[0x13] == 1 && r.slot_hits[0x31] == 1 && r.slot_hits[0x3D] == 1);
    CHECK(r.last_opcode == 0x3D && r.last_prev == 0x31 && r.last_pc == 0x9903);
    CHECK(st.latch == 0x3D && st.prev_opcode == 0x31 && st.pc == 0x9904);
    CHECK(st.b442e == 0);                                       /* cleared again on exit */
    /* the first dispatch sees the stale latch as previous opcode */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script; st.latch = 0x77;
    memset(&r, 0, sizeof r); fill_table(&tb, &r); r.stop_after = 1;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9901, 100, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 1 && r.last_opcode == 0x13 && r.last_prev == 0x77 && r.last_pc == 0x9901);
    /* stop by [0x49FF]: the loop ends and [0x442E] stays 0; [0x49FF] is left set */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    memset(&r, 0, sizeof r); fill_table(&tb, &r); r.stop_after = 2; r.stop_with_49ff = 1;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 100, &steps, NULL) == PR_REC_ECL_RUN_OK && steps == 2);
    CHECK(st.b49ff == 1 && st.b442e == 0);
    /* a stop byte already set by the gameplay loop: no dispatch at all */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script; st.b49ff = 1;
    memset(&r, 0, sizeof r); fill_table(&tb, &r);
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 100, &steps, NULL) == PR_REC_ECL_RUN_OK && steps == 0 && r.calls == 0);
    CHECK(st.pc == 0x9900);
    /* step limit */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    memset(&r, 0, sizeof r); fill_table(&tb, &r);
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 3, &steps, NULL) == PR_REC_ECL_RUN_STEP_LIMIT && steps == 3 && r.calls == 3);
    /* fetch bounds: entry past the script */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    memset(&r, 0, sizeof r); fill_table(&tb, &r);
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900 + sizeof script, 10, &steps, NULL) == PR_REC_ECL_RUN_FETCH_BOUNDS && r.calls == 0);
    /* no case: opcode 0x1F in the stream stops the model where the engine would spin */
    {
        static const uint8_t gap[] = { 0x0D, 0x1F, 0x0D };
        memset(&st, 0, sizeof st); st.script = gap; st.script_size = sizeof gap;
        memset(&r, 0, sizeof r); fill_table(&tb, &r);
        CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 10, &steps, NULL) == PR_REC_ECL_RUN_NO_CASE);
        CHECK(steps == 2 && r.calls == 1 && st.pc == 0x9901 && st.latch == 0x1F && st.prev_opcode == 0x0D);
    }
    /* unmodelled slot and handler failure surface with their statuses */
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    memset(&r, 0, sizeof r); fill_table(&tb, &r); tb.handler[0x13] = NULL;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 10, &steps, NULL) == PR_REC_ECL_RUN_UNMODELLED && steps == 2);
    memset(&r, 0, sizeof r); fill_table(&tb, &r); r.rc = PR_REC_ECL_HANDLER_STACK_FULL;
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 10, &steps, &hs) == PR_REC_ECL_RUN_HANDLER && hs == PR_REC_ECL_HANDLER_STACK_FULL && steps == 1);
    CHECK(pr_rec_ecl_run(NULL, &tb, 0x9900, 10, &steps, NULL) == PR_REC_ECL_RUN_ARGUMENT && steps == 0);
    CHECK(pr_rec_ecl_run(&st, NULL, 0x9900, 10, NULL, NULL) == PR_REC_ECL_RUN_ARGUMENT);
    return 0;
}

/* Composition: block load -> initializer -> run of the initial entry with the
 * real GOTO handler bound and an EXIT stub at the target. */
static int test_run_composition(void) {
    static uint8_t block[2 + 20 + 8];
    static uint8_t buf[PR_REC_ECL_SCRIPT_BUFFER_SIZE];
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps;
    fake_io_t f; pr_rec_ecl_block_io io;
    unsigned i;
    /* five entries; the fifth (initial entry) points at 0x9914, the byte after the head */
    memset(block, 0, sizeof block);
    for (i = 0; i < 5; ++i) { block[2 + 4 * i] = 0x01; block[3 + 4 * i] = 0x01; block[4 + 4 * i] = 0x14; block[5 + 4 * i] = 0x99; }
    block[2 + 20] = 0x0D;                                       /* at 0x9914: a zero-operand opcode */
    block[2 + 21] = 0x00;                                       /* at 0x9915: EXIT */
    memset(&st, 0, sizeof st); st.file_number = 3;
    fio_reset(&f, &io); f.sizes[0] = (uint16_t)sizeof block; f.blocks[0] = block;
    CHECK(pr_rec_ecl_load_block(&st, buf, 0, &io, 2, NULL) == PR_REC_ECL_BLOCK_OK);
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.entry_addr[4] == 0x9914 && st.pc == 0x9914);
    memset(&r, 0, sizeof r); fill_table(&tb, &r);
    tb.handler[0x01] = goto_adapter; tb.handler[0x00] = exit_adapter;
    /* run the initial entry as the gameplay loop does (+0x3362: push [0x49EB]) */
    CHECK(pr_rec_ecl_run(&st, &tb, st.entry_addr[4], 100, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 2 && r.calls == 2 && r.slot_hits[0x0D] == 1 && r.last_pc == 0x9915);
    CHECK(st.latch == 0x00 && st.prev_opcode == 0x0D && st.pc == 0x9915 && st.b442e == 0);
    /* run the first head entry: GOTO 0x9914, then the same two opcodes */
    memset(&r, 0, sizeof r); fill_table(&tb, &r); tb.handler[0x01] = goto_adapter; tb.handler[0x00] = exit_adapter;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 100, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 3 && r.calls == 2 && st.prev_opcode == 0x0D && st.latch == 0x00);
    return 0;
}

/* ---- issue #84: slot value and SAVE --------------------------------------- */

static int test_slot_value(void) {
    pr_rec_ecl_loader_state st; uint16_t v = 0x5555;
    static const uint8_t script[] = { 0x7B, 0x00 };
    memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script;
    st.window0[0x1C4] = 0xBEEF; st.window1[0x020] = 0x1234; st.window2[0x010] = 0x4321;
    /* tag 0: the low byte, high ignored */
    st.tag[1] = 0x00; st.low[1] = 0x7E; st.high[1] = 0xFF;
    CHECK(pr_rec_ecl_slot_value(&st, 1, &v) == PR_REC_ECL_WINDOW_OK && v == 0x7E);
    /* tag 1: a window-0 address is read through the load-word core */
    st.tag[2] = 0x01; st.low[2] = 0xC4; st.high[2] = 0x4A;
    CHECK(pr_rec_ecl_slot_value(&st, 2, &v) == PR_REC_ECL_WINDOW_OK && v == 0xBEEF);
    /* tag 3: same path; a script address yields the zero-extended byte */
    st.tag[3] = 0x03; st.low[3] = 0x00; st.high[3] = 0x99;
    CHECK(pr_rec_ecl_slot_value(&st, 3, &v) == PR_REC_ECL_WINDOW_OK && v == 0x7B);
    /* tag 0x80: the engine reads a word at (high << 8) + low with the length byte as low */
    st.tag[4] = 0x80; st.low[4] = 0x10; st.high[4] = 0x97;
    CHECK(pr_rec_ecl_slot_value(&st, 4, &v) == PR_REC_ECL_WINDOW_OK && v == 0x4321);
    /* tag 1 into the 0x6B00 window: the read happens but the hook is reported */
    st.tag[5] = 0x01; st.low[5] = 0x20; st.high[5] = 0x6B;
    CHECK(pr_rec_ecl_slot_value(&st, 5, &v) == PR_REC_ECL_WINDOW_HOOK && v == 0x1234);
    /* tag 2 and 0x81: the assembled word itself */
    st.tag[6] = 0x02; st.low[6] = 0x34; st.high[6] = 0x12;
    CHECK(pr_rec_ecl_slot_value(&st, 6, &v) == PR_REC_ECL_WINDOW_OK && v == 0x1234);
    st.tag[7] = 0x81; st.low[7] = 0x00; st.high[7] = 0x6B;
    CHECK(pr_rec_ecl_slot_value(&st, 7, &v) == PR_REC_ECL_WINDOW_OK && v == 0x6B00);
    /* tag 1 at a class-4 pseudo-address: unresolved from the core */
    st.tag[8] = 0x01; st.low[8] = 0xB2; st.high[8] = 0x00;
    CHECK(pr_rec_ecl_slot_value(&st, 8, &v) == PR_REC_ECL_WINDOW_UNRESOLVED);
    /* any other tag: the engine returns an uninitialized local */
    st.tag[9] = 0x05; st.low[9] = 0x11; st.high[9] = 0x22;
    CHECK(pr_rec_ecl_slot_value(&st, 9, &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0);
    /* arguments */
    CHECK(pr_rec_ecl_slot_value(&st, 0, &v) == PR_REC_ECL_WINDOW_ARGUMENT);
    CHECK(pr_rec_ecl_slot_value(&st, (uint8_t)PR_REC_ECL_SLOT_COUNT, &v) == PR_REC_ECL_WINDOW_ARGUMENT);
    CHECK(pr_rec_ecl_slot_value(&st, 1, NULL) == PR_REC_ECL_WINDOW_ARGUMENT);
    CHECK(pr_rec_ecl_slot_value(NULL, 1, &v) == PR_REC_ECL_WINDOW_ARGUMENT);
    return 0;
}

static void save_state(pr_rec_ecl_loader_state *st, const uint8_t *script, size_t size, uint8_t *rw) {
    unsigned i;
    memset(st, 0, sizeof *st); st->script = script; st->script_size = size; st->script_rw = rw; st->pc = 0x9900;
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) st->window0[i] = 0xAAAA;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) st->window1[i] = 0xAAAA;
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) st->window2[i] = 0xAAAA;
}

static int test_op_save(void) {
    pr_rec_ecl_loader_state st; pr_rec_ecl_window_status ws; unsigned i;
    /* SAVE immediate byte 5 to window-0 address 0x4AC4 (the new-game block's first instruction shape) */
    static const uint8_t s1[] = { 0x09, 0x00, 0x05, 0x01, 0xC4, 0x4A, 0x00 };
    save_state(&st, s1, sizeof s1, NULL);
    CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_OK && ws == PR_REC_ECL_WINDOW_OK);
    CHECK(st.window0[0x1C4] == 5 && st.pc == 0x9906);
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) if (i != 0x1C4) CHECK(st.window0[i] == 0xAAAA);
    /* SAVE immediate word (tag 2) */
    { static const uint8_t s2[] = { 0x09, 0x02, 0x34, 0x12, 0x01, 0x00, 0x49 };
      save_state(&st, s2, sizeof s2, NULL);
      CHECK(pr_rec_ecl_op_save(&st, NULL) == PR_REC_ECL_HANDLER_OK && st.window0[0] == 0x1234 && st.pc == 0x9907); }
    /* SAVE variable to variable (tag 1 value): window 0 word copied into window 2 */
    { static const uint8_t s3[] = { 0x09, 0x01, 0xC4, 0x4A, 0x01, 0x02, 0x97 };
      save_state(&st, s3, sizeof s3, NULL); st.window0[0x1C4] = 0x0BAD;
      CHECK(pr_rec_ecl_op_save(&st, NULL) == PR_REC_ECL_HANDLER_OK && st.window2[2] == 0x0BAD); }
    /* value read from the 0x6B00 window: stored, hook reported */
    { static const uint8_t s4[] = { 0x09, 0x01, 0x40, 0x6B, 0x01, 0x00, 0x49 };
      save_state(&st, s4, sizeof s4, NULL); st.window1[0x40] = 0x0C0D;
      CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_HOOK && ws == PR_REC_ECL_WINDOW_OK && st.window0[0] == 0x0C0D); }
    /* destination in the 0x6B00 window: stored, the store hook reported */
    { static const uint8_t s5[] = { 0x09, 0x00, 0x09, 0x01, 0x10, 0x6B };
      save_state(&st, s5, sizeof s5, NULL);
      CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_HOOK && ws == PR_REC_ECL_WINDOW_HOOK && st.window1[0x10] == 9); }
    /* destination class 3 (script byte) with a writable script: low byte stored */
    { static const uint8_t s6[] = { 0x09, 0x02, 0x78, 0x56, 0x01, 0x06, 0x99 }; static uint8_t rw[7];
      memcpy(rw, s6, sizeof s6); save_state(&st, rw, sizeof rw, rw);
      CHECK(pr_rec_ecl_op_save(&st, NULL) == PR_REC_ECL_HANDLER_OK && rw[6] == 0x78); }
    /* destination class 4: nothing stored, unresolved */
    { static const uint8_t s7[] = { 0x09, 0x00, 0x01, 0x01, 0xB2, 0x00 };
      save_state(&st, s7, sizeof s7, NULL);
      CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_UNRESOLVED && ws == PR_REC_ECL_WINDOW_UNRESOLVED);
      for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) CHECK(st.window0[i] == 0xAAAA); }
    /* unresolved value (class-4 source): the store is not made */
    { static const uint8_t s8[] = { 0x09, 0x01, 0xB2, 0x00, 0x01, 0x00, 0x49 };
      save_state(&st, s8, sizeof s8, NULL);
      CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_UNRESOLVED && ws == PR_REC_ECL_WINDOW_OK && st.window0[0] == 0xAAAA); }
    /* string path: an empty inline string writes one zero word at the destination */
    { static const uint8_t s9[] = { 0x09, 0x80, 0x00, 0x01, 0x02, 0x49 };
      save_state(&st, s9, sizeof s9, NULL);
      CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_OK && ws == PR_REC_ECL_WINDOW_OK);
      CHECK(st.window0[2] == 0 && st.window0[1] == 0xAAAA && st.window0[3] == 0xAAAA && st.pc == 0x9906); }
    /* string path: a three-byte packed string is stored one word per decoded character, then a zero word */
    { static const uint8_t s10[] = { 0x09, 0x80, 0x03, 0x1A, 0x2B, 0x3C, 0x01, 0x10, 0x49 };
      char want[PR_REC_ECL_TEXT_MAX]; size_t n;
      n = pr_rec_ecl_unpack_text(s10 + 3, 3, want, sizeof want);
      CHECK(n >= 1 && n <= 4);
      save_state(&st, s10, sizeof s10, NULL);
      CHECK(pr_rec_ecl_op_save(&st, NULL) == PR_REC_ECL_HANDLER_OK);
      for (i = 0; i < n; ++i) CHECK(st.window0[0x10 + i] == (uint16_t)(uint8_t)want[i]);
      CHECK(st.window0[0x10 + n] == 0 && st.window0[0x10 + n + 1] == 0xAAAA && st.pc == 0x9909); }
    /* truncated operands */
    { static const uint8_t s11[] = { 0x09, 0x00, 0x05, 0x01, 0xC4 };
      save_state(&st, s11, sizeof s11, NULL);
      CHECK(pr_rec_ecl_op_save(&st, NULL) == PR_REC_ECL_HANDLER_BOUNDS); }
    CHECK(pr_rec_ecl_op_save(NULL, &ws) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static pr_rec_ecl_handler_status save_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_save(st, NULL); }

/* A class-1 SAVE inside the run loop: the store is made, the handler reports
 * HOOK, the dispatcher maps it to RUN_HANDLER and the run loop returns at
 * once with the PC already past the instruction (PR #88 review). Continuing
 * from that PC is the caller's decision, as the private route tracer does. */
static int test_run_stops_on_hook(void) {
    static const uint8_t script[] = { 0x09, 0x00, 0x09, 0x01, 0x10, 0x6B,   /* 0x9900: SAVE 9 -> 0x6B10 (class 1) */
                                      0x09, 0x00, 0x03, 0x01, 0x00, 0x49,   /* 0x9906: SAVE 3 -> 0x4900 (class 0) */
                                      0x00 };                               /* 0x990C: EXIT */
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps; pr_rec_ecl_handler_status hs;
    save_state(&st, script, sizeof script, NULL);
    memset(&r, 0, sizeof r); memset(&tb, 0, sizeof tb); tb.ctx = &r;
    tb.handler[0x09] = save_adapter; tb.handler[0x00] = exit_adapter;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 10, &steps, &hs) == PR_REC_ECL_RUN_HANDLER);
    CHECK(hs == PR_REC_ECL_HANDLER_HOOK && steps == 1 && st.pc == 0x9906 && st.latch == 0x09);
    CHECK(st.window1[0x10] == 9 && st.window0[0] == 0xAAAA);   /* stored; the second SAVE did not run */
    CHECK(st.b442e == 0);                                       /* the stop byte was never set */
    /* explicit continuation from the returned PC */
    CHECK(pr_rec_ecl_run(&st, &tb, st.pc, 10, &steps, &hs) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 2 && hs == PR_REC_ECL_HANDLER_OK && st.window0[0] == 3 && r.calls == 1 && st.pc == 0x990C);
    return 0;
}
static pr_rec_ecl_handler_status print_return_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_print_return(st); }

/* Composition smoke: block load, initializer, then a run from the first head
 * entry through two SAVEs, a GOTO, and the next handled opcode (PRINT
 * RETURN) to an EXIT stub, as the new-game block's shape suggests. */
static int test_save_composition(void) {
    static uint8_t block[2 + 20 + 32];
    static uint8_t buf[PR_REC_ECL_SCRIPT_BUFFER_SIZE];
    static const uint8_t body[] = {
        0x09, 0x00, 0x07, 0x01, 0xC4, 0x4A,     /* 0x9914: SAVE 7 -> 0x4AC4 */
        0x09, 0x00, 0x00, 0x01, 0xEB, 0x49,     /* 0x991A: SAVE 0 -> 0x49EB */
        0x01, 0x01, 0x30, 0x99,                 /* 0x9920: GOTO 0x9930 */
        0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, /* filler */
        0x33,                                   /* 0x9930: PRINT RETURN */
        0x00                                    /* 0x9931: EXIT */
    };
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps, i;
    fake_io_t f; pr_rec_ecl_block_io io;
    memset(block, 0, sizeof block);
    for (i = 0; i < 5; ++i) { block[2 + 4 * i] = 0x01; block[3 + 4 * i] = 0x01; block[4 + 4 * i] = 0x14; block[5 + 4 * i] = 0x99; }
    memcpy(block + 22, body, sizeof body);
    memset(&st, 0, sizeof st); st.file_number = 3;
    fio_reset(&f, &io); f.sizes[0] = (uint16_t)sizeof block; f.blocks[0] = block;
    CHECK(pr_rec_ecl_load_block(&st, buf, 0, &io, 2, NULL) == PR_REC_ECL_BLOCK_OK);
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
    st.window0[0x1C4] = 0xAAAA; st.window0[0xEB] = 0xAAAA;
    memset(&r, 0, sizeof r); memset(&tb, 0, sizeof tb); tb.ctx = &r;
    tb.handler[0x09] = save_adapter; tb.handler[0x01] = goto_adapter; tb.handler[0x33] = print_return_adapter; tb.handler[0x00] = exit_adapter;
    CHECK(pr_rec_ecl_run(&st, &tb, st.entry_addr[0], 100, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 5 && r.calls == 1 && r.last_pc == 0x9931);
    CHECK(st.window0[0x1C4] == 7 && st.window0[0xEB] == 0);
    CHECK(st.b5e24 == 1 && st.b5e25 == 1);                      /* PRINT RETURN ran once */
    CHECK(st.latch == 0x00 && st.prev_opcode == 0x33 && st.pc == 0x9931);
    return 0;
}

/* ---- issue #92: LOAD FILES / LOAD PIECES ----------------------------------- */

typedef struct { char log[128]; unsigned n; } lf_log_t;
static void lf_put(lf_log_t *l, const char *s) { size_t k = strlen(l->log); if (k + strlen(s) < sizeof l->log) strcat(l->log, s); l->n++; }
static void lf_wallset(void *c, uint8_t id) { char b[16]; snprintf(b, sizeof b, "W%u ", id); lf_put((lf_log_t *)c, b); }
static void lf_pieces(void *c, uint8_t id, uint8_t slot) { char b[16]; snprintf(b, sizeof b, "P%u.%u ", id, slot); lf_put((lf_log_t *)c, b); }
static void lf_text(void *c) { lf_put((lf_log_t *)c, "T "); }
static void lf_27d9(void *c) { lf_put((lf_log_t *)c, "R1 "); }
static void lf_0767(void *c) { lf_put((lf_log_t *)c, "R2 "); }
static void lf_reset(lf_log_t *l, pr_rec_ecl_load_files_io *io) {
    memset(l, 0, sizeof *l); memset(io, 0, sizeof *io); io->ctx = l;
    io->load_wallset = lf_wallset; io->load_pieces = lf_pieces; io->show_text = lf_text; io->resident_27d9 = lf_27d9; io->resident_0767 = lf_0767;
}
static void lf_state(pr_rec_ecl_loader_state *st, const uint8_t *script, size_t size, uint8_t latch) {
    save_state(st, script, size, NULL); st->latch = latch; st->window0[0xE6] = 1; st->window0[0xE7] = 0; st->window0[0xE8] = 0;
}

static int test_op_load_files(void) {
    pr_rec_ecl_loader_state st; lf_log_t l; pr_rec_ecl_load_files_io io; unsigned i;
    static const uint8_t s_ff[] = { 0x21, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03 };   /* v1 = 5, v2 = 0, v3 = 3 */
    /* 0x21, wallset branch (the new-game shape): word 0xC5 set, wallset loaded, word 0x2C9 cleared, no text */
    lf_state(&st, s_ff, sizeof s_ff, 0x21); lf_reset(&l, &io);
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK);
    CHECK(strcmp(l.log, "W5 ") == 0 && st.window0[0xC5] == 5 && st.window1[0x2C9] == 0 && st.pc == 0x9907);
    CHECK(st.b4431 == 1 && st.b4434 == 1 && st.b4433 == 0 && st.b4432 == 0);
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) if (i != 0xC5 && i != 0xE6 && i != 0xE7 && i != 0xE8) CHECK(st.window0[i] == 0xAAAA);
    /* 0x21 with word 0xE6 == 0: no wallset, the text routine runs (v3 != 0xFF) */
    lf_state(&st, s_ff, sizeof s_ff, 0x21); st.window0[0xE6] = 0; lf_reset(&l, &io);
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "T ") == 0 && st.window0[0xC5] == 0xAAAA);
    /* 0x21 with v1 == 0xFF and with v1 == 0x7F: no wallset; v3 == 0xFF and word 0xE6 == 0: no text */
    { static const uint8_t s1[] = { 0x21, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF };
      lf_state(&st, s1, sizeof s1, 0x21); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && l.n == 0 && st.b4434 == 1);
      lf_state(&st, s1, sizeof s1, 0x21); st.window0[0xE6] = 0; lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && l.n == 0); }
    { static const uint8_t s2[] = { 0x21, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s2, sizeof s2, 0x21); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && l.n == 0); }
    /* 0x37 with v1 == 0x7F: pieces (0, 1) only */
    { static const uint8_t s3[] = { 0x37, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s3, sizeof s3, 0x37); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "P0.1 ") == 0);
      CHECK(st.b4433 == 1 && st.b4434 == 0 && st.b4431 == 1); }
    /* 0x37 with words 0xE7 and 0xE8 non-zero: pieces (v1, 1) and (v3, 3) */
    lf_state(&st, s_ff, sizeof s_ff, 0x37); st.window0[0xE7] = 2; st.window0[0xE8] = 9; lf_reset(&l, &io);
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "P5.1 P3.3 ") == 0);
    /* 0x37 default loop: slot 2 is 0xFF, its two words become 0xFFFF */
    { static const uint8_t s4[] = { 0x37, 0x00, 0x05, 0x00, 0xFF, 0x00, 0x03 };
      lf_state(&st, s4, sizeof s4, 0x37); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "P5.1 P3.3 ") == 0);
      CHECK(st.piece_words[1][0] == 0xFFFF && st.piece_words[1][1] == 0xFFFF);
      CHECK(st.piece_words[0][0] == 0 && st.piece_words[2][1] == 0); }
    /* the gate: with [0x4432] set, 0x21 then 0x37 run the two resident routines once and clear [0x4432] */
    { static const uint8_t s5[] = { 0x21, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x37, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s5, sizeof s5, 0x21); st.b4432 = 1; lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "W5 ") == 0 && st.b4432 == 1);
      st.latch = 0x37;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "W5 P0.1 R1 R2 ") == 0 && st.b4432 == 0);
      st.latch = 0x21; st.pc = 0x9900;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "W5 P0.1 R1 R2 W5 ") == 0); }
    /* operand values through variables: tag 1 window-0 word, and a class-1 source reports HOOK after the effects */
    { static const uint8_t s6[] = { 0x21, 0x01, 0x00, 0x49, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s6, sizeof s6, 0x21); st.window0[0] = 0x0107; lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && strcmp(l.log, "W7 ") == 0 && st.window0[0xC5] == 7); }
    { static const uint8_t s7[] = { 0x21, 0x01, 0x40, 0x6B, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s7, sizeof s7, 0x21); st.window1[0x40] = 6; lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_HOOK && strcmp(l.log, "W6 ") == 0); }
    /* an unresolved source (class 4) stops before any effect */
    { static const uint8_t s8[] = { 0x21, 0x01, 0xB2, 0x00, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s8, sizeof s8, 0x21); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && l.n == 0 && st.b4434 == 0 && st.b4431 == 1); }
    /* a reached boundary without its callback: UNRESOLVED after the earlier writes, before the later effects */
    lf_state(&st, s_ff, sizeof s_ff, 0x21); lf_reset(&l, &io); io.load_wallset = NULL;
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED);
    CHECK(st.window0[0xC5] == 5 && st.window1[0x2C9] == 0xAAAA && st.b4434 == 1 && l.n == 0);   /* word 0x2C9 not cleared */
    lf_state(&st, s_ff, sizeof s_ff, 0x21); st.window0[0xE6] = 0; lf_reset(&l, &io); io.show_text = NULL;
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && l.n == 0 && st.b4434 == 1);
    { static const uint8_t s3b[] = { 0x37, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s3b, sizeof s3b, 0x37); lf_reset(&l, &io); io.load_pieces = NULL;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && l.n == 0 && st.b4433 == 1); }
    lf_state(&st, s_ff, sizeof s_ff, 0x37); st.window0[0xE7] = 2; st.window0[0xE8] = 9; lf_reset(&l, &io); io.load_pieces = NULL;
    CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && l.n == 0);
    { static const uint8_t s4b[] = { 0x37, 0x00, 0xFF, 0x00, 0x05, 0x00, 0x03 };   /* slot 1 is 0xFF: its words are set before slot 2 reaches the boundary */
      lf_state(&st, s4b, sizeof s4b, 0x37); lf_reset(&l, &io); io.load_pieces = NULL;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && l.n == 0);
      CHECK(st.piece_words[0][0] == 0xFFFF && st.piece_words[0][1] == 0xFFFF && st.piece_words[2][0] == 0); }
    /* the gate's two resident boundaries: [0x4432] stays set when either is missing */
    { static const uint8_t s5b[] = { 0x37, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03 };
      lf_state(&st, s5b, sizeof s5b, 0x37); st.b4432 = 1; st.b4434 = 1; lf_reset(&l, &io); io.resident_27d9 = NULL;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && strcmp(l.log, "P0.1 ") == 0 && st.b4432 == 1);
      lf_state(&st, s5b, sizeof s5b, 0x37); st.b4432 = 1; st.b4434 = 1; lf_reset(&l, &io); io.resident_0767 = NULL;
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_UNRESOLVED && strcmp(l.log, "P0.1 R1 ") == 0 && st.b4432 == 1); }
    /* branches that reach no boundary succeed with every callback absent */
    { static const uint8_t s6b[] = { 0x21, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF };
      lf_state(&st, s6b, sizeof s6b, 0x21); memset(&io, 0, sizeof io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && st.b4434 == 1);
      lf_state(&st, s6b, sizeof s6b, 0x21); st.window0[0xE6] = 0; memset(&io, 0, sizeof io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK); }
    { static const uint8_t s7b[] = { 0x37, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF };
      lf_state(&st, s7b, sizeof s7b, 0x37); memset(&io, 0, sizeof io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_OK && st.piece_words[0][0] == 0xFFFF && st.piece_words[2][1] == 0xFFFF && st.b4433 == 1); }
    /* truncated operands; arguments */
    { static const uint8_t s9[] = { 0x21, 0x00, 0x05, 0x00 };
      lf_state(&st, s9, sizeof s9, 0x21); lf_reset(&l, &io);
      CHECK(pr_rec_ecl_op_load_files(&st, &io) == PR_REC_ECL_HANDLER_BOUNDS && l.n == 0); }
    CHECK(pr_rec_ecl_op_load_files(NULL, &io) == PR_REC_ECL_HANDLER_ARGUMENT);
    CHECK(pr_rec_ecl_op_load_files(&st, NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static lf_log_t g_lf; static pr_rec_ecl_load_files_io g_lf_io;
static pr_rec_ecl_handler_status load_files_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_load_files(st, &g_lf_io); }

/* Composition smoke in the initial-entry shape: four SAVEs, 0x21, 0x37, then
 * a GOTO to PRINT RETURN and an EXIT stub, through block load, initializer
 * and run loop with the gameplay loop's [0x4432] = 1 in place. */
static int test_load_files_composition(void) {
    static uint8_t block[2 + 20 + 64];
    static uint8_t buf[PR_REC_ECL_SCRIPT_BUFFER_SIZE];
    static const uint8_t body[] = {
        0x09, 0x00, 0x01, 0x01, 0x00, 0x4A,     /* 0x9914 */
        0x09, 0x00, 0x02, 0x01, 0x02, 0x4A,     /* 0x991A */
        0x09, 0x00, 0x03, 0x01, 0x04, 0x4A,     /* 0x9920 */
        0x09, 0x00, 0x04, 0x01, 0x06, 0x4A,     /* 0x9926 */
        0x21, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, /* 0x992C: 0x21 v1 = 5, v3 = 3 */
        0x37, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x03, /* 0x9933: 0x37 v1 = 0x7F */
        0x01, 0x01, 0x40, 0x99,                 /* 0x993A: GOTO 0x9940 */
        0x0D, 0x0D,                             /* filler */
        0x33,                                   /* 0x9940: PRINT RETURN */
        0x00                                    /* 0x9941: EXIT */
    };
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps, i;
    fake_io_t f; pr_rec_ecl_block_io io;
    memset(block, 0, sizeof block);
    for (i = 0; i < 5; ++i) { block[2 + 4 * i] = 0x01; block[3 + 4 * i] = 0x01; block[4 + 4 * i] = 0x14; block[5 + 4 * i] = 0x99; }
    memcpy(block + 22, body, sizeof body);
    memset(&st, 0, sizeof st); st.file_number = 3;
    fio_reset(&f, &io); f.sizes[0] = (uint16_t)sizeof block; f.blocks[0] = block;
    CHECK(pr_rec_ecl_load_block(&st, buf, 0, &io, 2, NULL) == PR_REC_ECL_BLOCK_OK);
    CHECK(pr_rec_ecl_script_init(&st) == PR_REC_ECL_HANDLER_OK);
    CHECK(st.window0[0xE6] == 1);                               /* the initializer sets it: wallset branch */
    st.b4432 = 1;                                               /* as the gameplay loop does at +0x3312 */
    lf_reset(&g_lf, &g_lf_io);
    memset(&r, 0, sizeof r); memset(&tb, 0, sizeof tb); tb.ctx = &r;
    tb.handler[0x09] = save_adapter; tb.handler[0x21] = load_files_adapter; tb.handler[0x37] = load_files_adapter;
    tb.handler[0x01] = goto_adapter; tb.handler[0x33] = print_return_adapter; tb.handler[0x00] = exit_adapter;
    CHECK(pr_rec_ecl_run(&st, &tb, st.entry_addr[0], 100, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 9 && r.calls == 1 && r.last_pc == 0x9941);
    CHECK(strcmp(g_lf.log, "W5 P0.1 R1 R2 ") == 0);
    CHECK(st.window0[0x100] == 1 && st.window0[0x102] == 2 && st.window0[0x104] == 3 && st.window0[0x106] == 4);   /* addresses 0x4A00, 0x4A02, 0x4A04, 0x4A06 */
    CHECK(st.window0[0xC5] == 5 && st.window1[0x2C9] == 0);
    CHECK(st.b4431 == 1 && st.b4432 == 0 && st.b4433 == 1 && st.b4434 == 1);
    CHECK(st.b5e24 == 1 && st.latch == 0x00 && st.prev_opcode == 0x33);
    return 0;
}

/* ---- issue #102: compare helpers, COMPARE and the IF family ---------------- */

static int flags_are(const pr_rec_ecl_loader_state *st, int eq, int ne, int lt, int gt, int le, int ge) {
    return st->cmp_flags[0] == eq && st->cmp_flags[1] == ne && st->cmp_flags[2] == lt && st->cmp_flags[3] == gt && st->cmp_flags[4] == le && st->cmp_flags[5] == ge;
}

/* Load word, class-4 pseudo-addresses (unit 0x25 + 0x0E0C, +0x0EA0..+0x0F49): every resolved pair, the
 * extension rules, the boundaries, and every unmatched address staying UNRESOLVED. Synthetic DS values. */
static int test_load_word_class4(void) {
    static pr_rec_ecl_loader_state st;
    uint16_t v;
    unsigned a, i;
    memset(&st, 0, sizeof st);
    st.w6f6e = 0x1234; st.w6f6a = 0x5678; st.w6f6c = 0x9ABC;
    st.b6aad = 0x11; st.b6aae = 0x22; st.b6aaf = 0x33; st.b6ab0 = 0x44; st.b6ab1 = 0x55;
    /* each exact pair */
    CHECK(pr_rec_ecl_load_word(&st, 0x00B1, &v) == PR_REC_ECL_WINDOW_OK && v == 0x1234);       /* [0x6F6E] */
    CHECK(pr_rec_ecl_load_word(&st, 0x00FB, &v) == PR_REC_ECL_WINDOW_OK && v == 0x5678);       /* [0x6F6A] */
    CHECK(pr_rec_ecl_load_word(&st, 0x00FC, &v) == PR_REC_ECL_WINDOW_OK && v == 0x9ABC);       /* [0x6F6C] */
    CHECK(pr_rec_ecl_load_word(&st, 0x033D, &v) == PR_REC_ECL_WINDOW_OK && v == 0x33);         /* byte [0x6AAF] */
    CHECK(pr_rec_ecl_load_word(&st, 0xC04B, &v) == PR_REC_ECL_WINDOW_OK && v == 0x11);         /* +0: [0x6AAD] */
    CHECK(pr_rec_ecl_load_word(&st, 0xC04C, &v) == PR_REC_ECL_WINDOW_OK && v == 0x22);         /* +1: [0x6AAE] */
    CHECK(pr_rec_ecl_load_word(&st, 0xC04D, &v) == PR_REC_ECL_WINDOW_OK && v == 0x19);         /* +2: [0x6AAF] >> 1 */
    CHECK(pr_rec_ecl_load_word(&st, 0xC04E, &v) == PR_REC_ECL_WINDOW_OK && v == 0x44);         /* +3: [0x6AB0] */
    CHECK(pr_rec_ecl_load_word(&st, 0xC04F, &v) == PR_REC_ECL_WINDOW_OK && v == 0x55);         /* +4: [0x6AB1] */
    /* class 4 and word values are as stored: full 16-bit words, byte sources zero-extended, party bytes sign-extended */
    st.w6f6e = 0xFFFF;
    CHECK(pr_rec_ecl_load_word(&st, 0x00B1, &v) == PR_REC_ECL_WINDOW_OK && v == 0xFFFF);
    st.b6aaf = 0xFF;
    CHECK(pr_rec_ecl_load_word(&st, 0x033D, &v) == PR_REC_ECL_WINDOW_OK && v == 0x00FF);       /* xor ah, ah */
    for (i = 0; i < 256; ++i) {
        uint16_t sx = (uint16_t)(i < 0x80u ? i : 0xFF00u | i);
        st.b6aad = (uint8_t)i; st.b6aae = (uint8_t)(255u - i); st.b6aaf = (uint8_t)i; st.b6ab0 = (uint8_t)i; st.b6ab1 = (uint8_t)(i ^ 0x5Au);
        CHECK(pr_rec_ecl_load_word(&st, 0xC04B, &v) == PR_REC_ECL_WINDOW_OK && v == sx);        /* cbw */
        CHECK(pr_rec_ecl_load_word(&st, 0xC04C, &v) == PR_REC_ECL_WINDOW_OK && v == (uint16_t)((255u - i) < 0x80u ? (255u - i) : 0xFF00u | (255u - i)));
        CHECK(pr_rec_ecl_load_word(&st, 0xC04D, &v) == PR_REC_ECL_WINDOW_OK && v == (uint16_t)(i >> 1));   /* zero-extended, then shr 1 */
        CHECK(pr_rec_ecl_load_word(&st, 0xC04E, &v) == PR_REC_ECL_WINDOW_OK && v == i);
        CHECK(pr_rec_ecl_load_word(&st, 0xC04F, &v) == PR_REC_ECL_WINDOW_OK && v == (uint16_t)(i ^ 0x5Au));
    }
    /* boundaries and every other class-4 address: the engine returns an uninitialized local there */
    v = 0x7777;
    CHECK(pr_rec_ecl_load_word(&st, 0xC04A, &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);   /* just below the 0xC04B group */
    CHECK(pr_rec_ecl_load_word(&st, 0xC050, &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);   /* +5 */
    CHECK(pr_rec_ecl_load_word(&st, 0xC059, &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);   /* +0xE: compared, nothing loaded */
    CHECK(pr_rec_ecl_load_word(&st, 0xFFFF, &v) == PR_REC_ECL_WINDOW_UNRESOLVED);
    CHECK(pr_rec_ecl_load_word(&st, 0x035F, &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);   /* compared, nothing loaded */
    {
        static const uint16_t near_miss[] = { 0x00B0, 0x00B2, 0x00FA, 0x00FD, 0x033C, 0x033E, 0x0000, 0x00FF, 0x0360, 0x0100 };
        for (i = 0; i < sizeof near_miss / sizeof near_miss[0]; ++i) {
            CHECK(pr_rec_ecl_address_class(near_miss[i]) == 4u);
            CHECK(pr_rec_ecl_load_word(&st, near_miss[i], &v) == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);
        }
    }
    /* every class-4 address below 0xC04B other than the four pairs, and every one above 0xC04F, is unresolved;
       classes 0-3 never reach the table */
    for (a = 0; a < 0x10000u; ++a) {
        uint8_t cls = pr_rec_ecl_address_class((uint16_t)a);
        pr_rec_ecl_window_status ws;
        if (cls != 4u) continue;
        v = 0x7777;
        ws = pr_rec_ecl_load_word(&st, (uint16_t)a, &v);
        if (a == 0x00B1u || a == 0x00FBu || a == 0x00FCu || a == 0x033Du || (a >= 0xC04Bu && a <= 0xC04Fu))
            CHECK(ws == PR_REC_ECL_WINDOW_OK);
        else
            CHECK(ws == PR_REC_ECL_WINDOW_UNRESOLVED && v == 0x7777);
    }
    /* a class-4 source through slot value and SAVE: 0xC04F gives the live [0x6AB1] the host holds, nothing else */
    {
        static const uint8_t s[] = { 0x09, 0x01, 0x4F, 0xC0, 0x01, 0x10, 0x4A };            /* SAVE [0x4A10] := [0xC04F] */
        uint16_t sv = 0;
        pr_rec_ecl_window_status ws;
        memset(&st, 0, sizeof st); st.script = s; st.script_size = sizeof s; st.pc = 0x9900u; st.b6ab1 = 0x3C;
        CHECK(pr_rec_ecl_op_save(&st, &ws) == PR_REC_ECL_HANDLER_OK && ws == PR_REC_ECL_WINDOW_OK && st.window0[0x110] == 0x3C);
        st.tag[1] = 0x01; st.low[1] = 0x4F; st.high[1] = 0xC0; st.b6ab1 = 0x07;
        CHECK(pr_rec_ecl_slot_value(&st, 1, &sv) == PR_REC_ECL_WINDOW_OK && sv == 0x07);
        st.low[1] = 0x59;                                                                     /* 0xC059: unresolved stays unresolved */
        CHECK(pr_rec_ecl_slot_value(&st, 1, &sv) == PR_REC_ECL_WINDOW_UNRESOLVED);
    }
    CHECK(pr_rec_ecl_load_word(NULL, 0xC04F, &v) == PR_REC_ECL_WINDOW_ARGUMENT);
    CHECK(pr_rec_ecl_load_word(&st, 0xC04F, NULL) == PR_REC_ECL_WINDOW_ARGUMENT);
    return 0;
}

/* Store word class 1 (+0x0CB7..+0x0CD8): window write, then the hook +0x0A74. Only 0x6E82 is proven to return from
 * the hook with no DS/ES game-state write and no call, so only it reports OK; every other class-1 address keeps HOOK. */
static int test_store_word_6e82(void) {
    static pr_rec_ecl_loader_state st;
    unsigned a, n_effect = 0;
    static uint8_t record[PR_U25_HOOK_RECORD_SIZE];
    pr_u25_hook_flags flags;
    memset(&st, 0, sizeof st);
    st.window1[0x382] = 0xAAAA; st.window1[0x381] = 0xAAAA; st.window1[0x383] = 0xAAAA;
    CHECK(pr_rec_ecl_store_word(&st, 0x6E82, 0x1234) == PR_REC_ECL_WINDOW_OK && st.window1[0x382] == 0x1234);     /* the write still happens */
    CHECK(st.window1[0x381] == 0xAAAA && st.window1[0x383] == 0xAAAA);
    CHECK(pr_rec_ecl_store_word(&st, 0x6E82, 0) == PR_REC_ECL_WINDOW_OK && st.window1[0x382] == 0);
    CHECK(pr_rec_ecl_store_word(&st, 0x6E82, 0xFFFF) == PR_REC_ECL_WINDOW_OK && st.window1[0x382] == 0xFFFF);
    /* adjacent no-op addresses and every side-effectful address stay HOOK (after their window write) */
    {
        static const uint16_t hooked[] = { 0x6E81, 0x6E83, 0x6E80, 0x6E84, 0x6B00, 0x6B20, 0x6B70, 0x6B71, 0x6BB8, 0x6BBB, 0x6BF9,
                                           0x6C00, 0x6C0C, 0x6E12, 0x6E22, 0x6E24, 0x6E26, 0x6EFF, 0x6B01 };
        unsigned i;
        for (i = 0; i < sizeof hooked / sizeof hooked[0]; ++i) {
            st.window1[hooked[i] - 0x6B00u] = 0;
            CHECK(pr_rec_ecl_store_word(&st, hooked[i], 0x5A5A) == PR_REC_ECL_WINDOW_HOOK && st.window1[hooked[i] - 0x6B00u] == 0x5A5A);
        }
    }
    /* sweep: exactly one class-1 address (0x6E82) returns OK; every other returns HOOK */
    for (a = 0x6B00; a <= 0x6EFF; ++a) {
        pr_rec_ecl_window_status w = pr_rec_ecl_store_word(&st, (uint16_t)a, 1);
        CHECK(a == 0x6E82 ? w == PR_REC_ECL_WINDOW_OK : w == PR_REC_ECL_WINDOW_HOOK);
    }
    /* cross-check with the standalone model of the hook (+0x0A74): 0x6E82 matches no case, makes no game-state write or call, and the 96
       addresses that do something are exactly the ones the byte-level control trace found (rel 0x000, 0x020..0x070,
       0x0B8, 0x0BB, 0x0BD, 0x0BF, 0x0C1, 0x0C3, 0x0F7, 0x0F9, 0x100, 0x10C, 0x312, 0x322, 0x324, 0x326) */
    memset(record, 0x77, sizeof record); memset(&flags, 0, sizeof flags);
    {
        pr_u25_hook_report hr;
        CHECK(pr_u25_store_hook_0a74(0x6E82, 0x1234, record, &flags, NULL, &hr) == PR_U25_HOOK_OK && hr.which == PR_U25_HOOK_CASE_NONE && !hr.callout_made);
        CHECK(flags.b84d2 == 0 && flags.b84d3 == 0);
        for (a = 0; a < PR_U25_HOOK_RECORD_SIZE; ++a) CHECK(record[a] == 0x77);
    }
    for (a = 0x6B00; a <= 0x6EFF; ++a) {
        pr_u25_hook_report hr;
        unsigned rel = a - 0x6B00u;
        int expect = rel == 0x000u || (rel >= 0x020u && rel <= 0x070u) || rel == 0x0B8u || rel == 0x0BBu || rel == 0x0BDu ||
                     rel == 0x0BFu || rel == 0x0C1u || rel == 0x0C3u || rel == 0x0F7u || rel == 0x0F9u || rel == 0x100u ||
                     rel == 0x10Cu || rel == 0x312u || rel == 0x322u || rel == 0x324u || rel == 0x326u;
        memset(record, 0x77, sizeof record); memset(&flags, 0, sizeof flags);
        (void)pr_u25_store_hook_0a74((uint16_t)a, 0x00A5, record, &flags, NULL, &hr);
        CHECK((hr.which != PR_U25_HOOK_CASE_NONE) == expect);
        if (expect) ++n_effect;
    }
    CHECK(n_effect == 96);
    return 0;
}

static int test_compare_words(void) {
    pr_rec_ecl_loader_state st; unsigned i;
    memset(&st, 0, sizeof st); memset(st.cmp_flags, 0xAA, sizeof st.cmp_flags);
    CHECK(pr_rec_ecl_compare_words(&st, 5, 5) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));
    CHECK(pr_rec_ecl_compare_words(&st, 3, 9) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0));
    CHECK(pr_rec_ecl_compare_words(&st, 9, 3) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1));
    CHECK(pr_rec_ecl_compare_words(&st, 0x0000, 0xFFFF) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0));   /* unsigned */
    CHECK(pr_rec_ecl_compare_words(&st, 0x8000, 0x7FFF) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1));
    CHECK(pr_rec_ecl_compare_words(&st, 0, 0) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));
    for (i = 0; i < 6; ++i) CHECK(st.cmp_flags[i] == 0 || st.cmp_flags[i] == 1);
    CHECK(pr_rec_ecl_compare_words(NULL, 1, 2) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static int test_compare_strings(void) {
    pr_rec_ecl_loader_state st;
    static char big[300];
    memset(&st, 0, sizeof st); memset(st.cmp_flags, 0xAA, sizeof st.cmp_flags); memset(big, 'A', sizeof big);
    CHECK(pr_rec_ecl_compare_strings(&st, "ABC", 3, "ABC", 3) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));
    CHECK(pr_rec_ecl_compare_strings(&st, "AB", 2, "ABC", 3) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0));   /* prefix is less */
    CHECK(pr_rec_ecl_compare_strings(&st, "ABD", 3, "ABC", 3) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1));
    CHECK(pr_rec_ecl_compare_strings(&st, "", 0, "A", 1) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0));
    CHECK(pr_rec_ecl_compare_strings(&st, "", 0, "", 0) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));
    CHECK(pr_rec_ecl_compare_strings(&st, "a", 1, "A", 1) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1));   /* case-sensitive, unsigned bytes */
    CHECK(pr_rec_ecl_compare_strings(&st, "\xFF", 1, "\x01", 1) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1));
    CHECK(pr_rec_ecl_compare_strings(&st, big, 300, big, 255) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));   /* both limited to 255 */
    /* an input that aliases the flag bytes is compared as it was on entry (copy before clear) */
    { static const char six[6] = { 1, 2, 3, 4, 5, 6 };
      st.cmp_flags[0] = 1; st.cmp_flags[1] = 2; st.cmp_flags[2] = 3; st.cmp_flags[3] = 4; st.cmp_flags[4] = 5; st.cmp_flags[5] = 6;
      CHECK(pr_rec_ecl_compare_strings(&st, (const char *)st.cmp_flags, 6, six, 6) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1));
      st.cmp_flags[0] = 1; st.cmp_flags[1] = 2; st.cmp_flags[2] = 3; st.cmp_flags[3] = 4; st.cmp_flags[4] = 5; st.cmp_flags[5] = 6;
      CHECK(pr_rec_ecl_compare_strings(&st, six, 6, (const char *)st.cmp_flags, 6) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1)); }
    CHECK(pr_rec_ecl_compare_strings(&st, NULL, 0, "", 0) == PR_REC_ECL_HANDLER_OK);
    CHECK(pr_rec_ecl_compare_strings(&st, NULL, 1, "", 0) == PR_REC_ECL_HANDLER_ARGUMENT);
    CHECK(pr_rec_ecl_compare_strings(NULL, "", 0, "", 0) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static int test_op_compare(void) {
    pr_rec_ecl_loader_state st;
    /* word path: immediate byte against immediate byte */
    { static const uint8_t s1[] = { 0x03, 0x00, 0x07, 0x00, 0x09 };
      save_state(&st, s1, sizeof s1, NULL); memset(st.cmp_flags, 0xAA, sizeof st.cmp_flags);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0) && st.pc == 0x9905); }
    /* word path: a window-0 variable (tag 1) against an immediate word (tag 2) */
    { static const uint8_t s2[] = { 0x03, 0x01, 0xC4, 0x4A, 0x02, 0x34, 0x12 };
      save_state(&st, s2, sizeof s2, NULL); st.window0[0x1C4] = 0x1234;
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1) && st.pc == 0x9907); }
    /* word path with a class-1 source in slot 1: HOOK at once, flags untouched, slot 2 never read
     * (slot 2 names a class-4 address, which would turn the result into UNRESOLVED if it were looked up) */
    { static const uint8_t s3[] = { 0x03, 0x01, 0x40, 0x6B, 0x01, 0xB2, 0x00 };
      save_state(&st, s3, sizeof s3, NULL); st.window1[0x40] = 5; memset(st.cmp_flags, 0x55, sizeof st.cmp_flags);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_HOOK);
      CHECK(st.cmp_flags[0] == 0x55 && st.cmp_flags[1] == 0x55 && st.cmp_flags[2] == 0x55 && st.cmp_flags[3] == 0x55 && st.cmp_flags[4] == 0x55 && st.cmp_flags[5] == 0x55);
      CHECK(st.pc == 0x9907); }                                 /* the loader's advance stands */
    /* class-1 source in slot 2: HOOK before the compare, flags untouched */
    { static const uint8_t s3b[] = { 0x03, 0x00, 0x02, 0x01, 0x40, 0x6B };
      save_state(&st, s3b, sizeof s3b, NULL); st.window1[0x40] = 5; memset(st.cmp_flags, 0x55, sizeof st.cmp_flags);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_HOOK && st.cmp_flags[0] == 0x55 && st.cmp_flags[5] == 0x55); }
    /* word path with a class-4 source: unresolved, flags untouched */
    { static const uint8_t s4[] = { 0x03, 0x00, 0x02, 0x01, 0xB2, 0x00 };
      save_state(&st, s4, sizeof s4, NULL); memset(st.cmp_flags, 0x55, sizeof st.cmp_flags);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_UNRESOLVED && st.cmp_flags[0] == 0x55 && st.cmp_flags[5] == 0x55); }
    /* string path: two inline strings; equal, then differing */
    { static const uint8_t s5[] = { 0x03, 0x80, 0x03, 0x1A, 0x2B, 0x3C, 0x80, 0x03, 0x1A, 0x2B, 0x3C };
      save_state(&st, s5, sizeof s5, NULL);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 1, 0, 0, 0, 1, 1) && st.pc == 0x990B); }
    { static const uint8_t s6[] = { 0x03, 0x80, 0x03, 0x1A, 0x2B, 0x3C, 0x80, 0x00 };
      save_state(&st, s6, sizeof s6, NULL);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 0, 1, 0, 1)); }   /* empty second string is less */
    /* string path with one string operand in second position: it lands in buffer 1 and is compared with buffer 2 as left by an earlier load */
    { static const uint8_t s7[] = { 0x03, 0x00, 0x05, 0x80, 0x00 };
      save_state(&st, s7, sizeof s7, NULL); st.string_len[2] = 1; st.string_text[2][0] = 'Z';
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_OK && flags_are(&st, 0, 1, 1, 0, 1, 0)); }   /* "" against "Z" */
    /* truncated operands; null */
    { static const uint8_t s8[] = { 0x03, 0x00, 0x05, 0x01, 0x00 };
      save_state(&st, s8, sizeof s8, NULL);
      CHECK(pr_rec_ecl_op_compare(&st) == PR_REC_ECL_HANDLER_BOUNDS); }
    CHECK(pr_rec_ecl_op_compare(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static int test_op_if(void) {
    pr_rec_ecl_loader_state st; unsigned op;
    /* IF at 0x9900 followed by a SAVE (6 bytes) at 0x9901 and a zero-operand opcode at 0x9907 */
    static const uint8_t script[] = { 0x16, 0x09, 0x00, 0x01, 0x01, 0x00, 0x49, 0x0D, 0x00 };
    for (op = 0x16; op <= 0x1B; ++op) {
        unsigned k = op - 0x16;
        /* flag set: no skip, PC past the IF byte only, latch untouched */
        save_state(&st, script, sizeof script, NULL); st.latch = (uint8_t)op; memset(st.cmp_flags, 0, 6); st.cmp_flags[k] = 1;
        CHECK(pr_rec_ecl_op_if(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9901 && st.latch == op);
        /* flag clear: the SAVE is skipped, the latch holds its opcode */
        save_state(&st, script, sizeof script, NULL); st.latch = (uint8_t)op; memset(st.cmp_flags, 1, 6); st.cmp_flags[k] = 0;
        CHECK(pr_rec_ecl_op_if(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9907 && st.latch == 0x09);
    }
    /* skipping a zero-operand statement advances by one */
    save_state(&st, script, sizeof script, NULL); st.pc = 0x9906; st.latch = 0x16; memset(st.cmp_flags, 0, 6);
    CHECK(pr_rec_ecl_op_if(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9908 && st.latch == 0x0D);
    /* skipping past the end of the script */
    save_state(&st, script, 2, NULL); st.latch = 0x17; memset(st.cmp_flags, 0, 6);
    CHECK(pr_rec_ecl_op_if(&st) == PR_REC_ECL_HANDLER_BOUNDS);
    /* a latch outside the family only advances */
    save_state(&st, script, sizeof script, NULL); st.latch = 0x15; memset(st.cmp_flags, 0, 6);
    CHECK(pr_rec_ecl_op_if(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9901);
    CHECK(pr_rec_ecl_op_if(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static pr_rec_ecl_handler_status compare_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_compare(st); }
static pr_rec_ecl_handler_status if_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_if(st); }

/* Composition: COMPARE then IF-not-equal with a SAVE in its shadow, both ways,
 * through the run loop with the real handlers. */
static int test_compare_if_composition(void) {
    static const uint8_t script[] = {
        0x03, 0x01, 0x00, 0x49, 0x00, 0x07,         /* 0x9900: COMPARE window word 0x4900 with 7 */
        0x17,                                       /* 0x9906: IF <> */
        0x09, 0x00, 0x01, 0x01, 0x02, 0x49,         /* 0x9907: SAVE 1 -> 0x4902 (runs only when not equal) */
        0x09, 0x00, 0x02, 0x01, 0x04, 0x49,         /* 0x990D: SAVE 2 -> 0x4904 */
        0x00                                        /* 0x9913: EXIT */
    };
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps;
    memset(&r, 0, sizeof r); memset(&tb, 0, sizeof tb); tb.ctx = &r;
    tb.handler[0x03] = compare_adapter; tb.handler[0x17] = if_adapter; tb.handler[0x09] = save_adapter; tb.handler[0x00] = exit_adapter;
    /* not equal: the shadowed SAVE runs */
    save_state(&st, script, sizeof script, NULL); st.window0[0] = 3;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 20, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 5 && st.window0[2] == 1 && st.window0[4] == 2 && flags_are(&st, 0, 1, 1, 0, 1, 0));
    /* equal: the shadowed SAVE is skipped, the following one runs */
    save_state(&st, script, sizeof script, NULL); st.window0[0] = 7;
    memset(&r, 0, sizeof r);
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 20, &steps, NULL) == PR_REC_ECL_RUN_OK);
    CHECK(steps == 4 && st.window0[2] == 0xAAAA && st.window0[4] == 2 && st.prev_opcode == 0x09 && r.last_pc == 0x9913);
    return 0;
}

/* ---- issue #113: RETURN -------------------------------------------------- */

static int test_op_return(void) {
    pr_rec_ecl_loader_state st;
    static const uint8_t script[] = { 0x13, 0x0D, 0x00 };
    /* empty list: only the PC advance */
    save_state(&st, script, sizeof script, NULL);
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9901 && st.call_depth == 0);
    /* one frame: the saved PC comes back, the frame is gone */
    save_state(&st, script, sizeof script, NULL); st.call_stack[0] = 0x9A10; st.call_depth = 1;
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9A10 && st.call_depth == 0);
    /* nested frames come back last in, first out */
    save_state(&st, script, sizeof script, NULL); st.call_stack[0] = 0x9A10; st.call_stack[1] = 0x9B20; st.call_depth = 2;
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9B20 && st.call_depth == 1);
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9A10 && st.call_depth == 0);
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9A11 && st.call_depth == 0);
    /* A corrupt host depth cannot index past the bounded list array. */
    save_state(&st, script, sizeof script, NULL); st.call_depth = PR_REC_ECL_CALL_STACK_MAX + 1u;
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_BOUNDS && st.pc == 0x9901 &&
          st.call_depth == PR_REC_ECL_CALL_STACK_MAX + 1u);
    /* after the real GOSUB push, RETURN lands on the byte after the GOSUB operand */
    { static const uint8_t s2[] = { 0x02, 0x01, 0x10, 0x9A, 0x0D };
      save_state(&st, s2, sizeof s2, NULL);
      CHECK(pr_rec_ecl_op_gosub(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9A10 && st.call_depth == 1 && st.call_stack[0] == 0x9904);
      CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.pc == 0x9904 && st.call_depth == 0); }
    /* nothing else in the state moves */
    save_state(&st, script, sizeof script, NULL); st.call_stack[0] = 0x9A10; st.call_depth = 1; st.latch = 0x13; st.b442e = 0; st.b49ff = 0;
    memset(st.cmp_flags, 0x55, 6);
    CHECK(pr_rec_ecl_op_return(&st) == PR_REC_ECL_HANDLER_OK && st.latch == 0x13 && st.b442e == 0 && st.cmp_flags[0] == 0x55 && st.window0[0] == 0xAAAA);
    CHECK(pr_rec_ecl_op_return(NULL) == PR_REC_ECL_HANDLER_ARGUMENT);
    return 0;
}

static pr_rec_ecl_handler_status gosub_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_gosub(st); }
static pr_rec_ecl_handler_status return_adapter(pr_rec_ecl_loader_state *st, void *ctx) { (void)ctx; return pr_rec_ecl_op_return(st); }

/* Composition through the run loop: GOSUB to a subroutine that RETURNs into
 * the caller, a nested GOSUB two deep, and a RETURN on an empty list that
 * falls through to EXIT. Addresses are laid out explicitly. */
static int test_gosub_return_composition(void) {
    static uint8_t script[0x40];
    pr_rec_ecl_loader_state st; pr_rec_ecl_dispatch_table tb; rec_ctx_t r; unsigned steps;
    memset(script, 0x0D, sizeof script);
    /* 0x9900 GOSUB 0x9920; 0x9904 SAVE 1->0x4900; 0x990A GOSUB 0x9930; 0x990E RETURN (empty); 0x990F EXIT */
    script[0x00] = 0x02; script[0x01] = 0x01; script[0x02] = 0x20; script[0x03] = 0x99;
    script[0x04] = 0x09; script[0x05] = 0x00; script[0x06] = 0x01; script[0x07] = 0x01; script[0x08] = 0x00; script[0x09] = 0x49;
    script[0x0A] = 0x02; script[0x0B] = 0x01; script[0x0C] = 0x30; script[0x0D] = 0x99;
    script[0x0E] = 0x13; script[0x0F] = 0x00;
    /* 0x9920 SAVE 2->0x4902; 0x9926 RETURN */
    script[0x20] = 0x09; script[0x21] = 0x00; script[0x22] = 0x02; script[0x23] = 0x01; script[0x24] = 0x02; script[0x25] = 0x49; script[0x26] = 0x13;
    /* 0x9930 GOSUB 0x9938; 0x9934 SAVE 3->0x4904 (6 bytes, ends 0x993A) -> put SAVE at 0x993A instead */
    script[0x30] = 0x02; script[0x31] = 0x01; script[0x32] = 0x3A; script[0x33] = 0x99;   /* GOSUB 0x993A */
    script[0x34] = 0x13;                                                                  /* 0x9934 RETURN -> 0x990E */
    script[0x3A] = 0x0D; script[0x3B] = 0x13;                                             /* 0x993A filler, 0x993B RETURN -> 0x9934 */
    save_state(&st, script, sizeof script, NULL);
    memset(&r, 0, sizeof r); memset(&tb, 0, sizeof tb); tb.ctx = &r;
    tb.handler[0x02] = gosub_adapter; tb.handler[0x13] = return_adapter; tb.handler[0x09] = save_adapter;
    tb.handler[0x0D] = rec_handler; tb.handler[0x00] = exit_adapter;
    CHECK(pr_rec_ecl_run(&st, &tb, 0x9900, 50, &steps, NULL) == PR_REC_ECL_RUN_OK);
    /* GOSUB, SAVE2, RETURN, SAVE1, GOSUB, GOSUB, filler, RETURN, RETURN, RETURN(empty), EXIT = 11 dispatches */
    CHECK(steps == 11 && r.last_pc == 0x990F);
    CHECK(st.window0[0] == 1 && st.window0[2] == 2 && st.call_depth == 0 && st.pc == 0x990F);   /* addresses 0x4900 and 0x4902 */
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
    rc |= test_windows_round_trip();
    rc |= test_windows_bounds_and_name();
    rc |= test_windows_clear_drops_active_name();
    rc |= test_op_goto();
    rc |= test_op_gosub_and_push_frame();
    rc |= test_op_print_return();
    rc |= test_u38_entry8();
    rc |= test_u38_entry9();
    rc |= test_u38_entry10();
    rc |= test_u38_entry31();
    rc |= test_u38_entry30();
    rc |= test_u3bd_entry1();
    rc |= test_script_init();
    rc |= test_block_name();
    rc |= test_block_load();
    rc |= test_handler_offsets();
    rc |= test_dispatch_all_opcodes();
    rc |= test_run_loop();
    rc |= test_run_composition();
    rc |= test_slot_value();
    rc |= test_op_save();
    rc |= test_save_composition();
    rc |= test_run_stops_on_hook();
    rc |= test_op_load_files();
    rc |= test_load_files_composition();
    rc |= test_load_word_class4();
    rc |= test_store_word_6e82();
    rc |= test_compare_words();
    rc |= test_compare_strings();
    rc |= test_op_compare();
    rc |= test_op_if();
    rc |= test_compare_if_composition();
    rc |= test_op_return();
    rc |= test_gosub_return_composition();
    if (rc == 0) {
        puts("recovered ECL helper tests passed");
    }
    return rc;
}
