/* Synthetic tests for unit 0x25 + 0x0A74, the class-1 ECL store hook. No
 * DOS bytes. Expected record offsets and values are hand-derived from the
 * decoded cases; every case checks that no other record byte or flag moved. */
#include "poolrad/recovered/overlay_ecl_store_hook.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    unsigned res_calls, u3bd_calls;
    unsigned last_value, last_selector;
} capture;

static void on_res(void *ctx, uint16_t pushed_word)
{
    capture *c = (capture *)ctx; c->res_calls++; c->last_value = pushed_word;
}

static void on_u3bd(void *ctx, uint8_t value, uint8_t selector)
{
    capture *c = (capture *)ctx; c->u3bd_calls++; c->last_value = value; c->last_selector = selector;
}

static void fill(uint8_t *record, unsigned seed)
{
    unsigned i;
    for (i = 0; i < PR_U25_HOOK_RECORD_SIZE; ++i) record[i] = (uint8_t)(i * 5u + seed);
}

/* Returns the number of bytes that differ between two records. */
static unsigned diff_count(const uint8_t *a, const uint8_t *b)
{
    unsigned i, n = 0;
    for (i = 0; i < PR_U25_HOOK_RECORD_SIZE; ++i) n += (a[i] != b[i]);
    return n;
}

/* Sweep every address 0x6B00..0x6EFF (and a few outside) with a value that
 * is not special, and verify exactly which bytes may move. */
static int test_address_map(void)
{
    uint8_t record[PR_U25_HOOK_RECORD_SIZE], copy[PR_U25_HOOK_RECORD_SIZE];
    unsigned rel;
    for (rel = 0; rel < 0x400u; ++rel) {
        pr_u25_hook_flags flags = {0, 0};
        pr_u25_hook_callouts co = {on_res, on_u3bd, NULL};
        capture cap = {0, 0, 0, 0};
        pr_u25_hook_report rep;
        pr_u25_hook_status st;
        uint16_t address = (uint16_t)(0x6B00u + rel);
        uint16_t value = 0x1234;          /* low byte 0x34, > 0xB2, not 0, 0x80, 0x81, 0x87 */
        co.context = &cap;
        fill(record, rel); memcpy(copy, record, sizeof record);
        st = pr_u25_store_hook_0a74(address, value, record, &flags, &co, &rep);
        CHECK(st == PR_U25_HOOK_OK);
        CHECK(flags.b84d2 == 0 && flags.b84d3 == 0);   /* value nonzero: flags untouched */
        if (rel == 0) {
            CHECK(rep.which == PR_U25_HOOK_CASE_FLAG_84D3 && diff_count(record, copy) == 0);
        } else if (rel >= 0x20u && rel <= 0x70u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_BYTE_RANGE && rep.record_offset == rel - 8u);
            CHECK(record[rel - 8u] == 0x34 && diff_count(record, copy) == (copy[rel - 8u] != 0x34u));
        } else if (rel == 0xB8u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_B8 && record[0x84] == (uint8_t)(0x1234u - 0x32u));
            CHECK(diff_count(record, copy) <= 1u);
        } else if (rel == 0xBBu || rel == 0xBDu || rel == 0xBFu || rel == 0xC1u || rel == 0xC3u) {
            unsigned off = 0x88u + (rel - 0xBBu);
            CHECK(rep.which == PR_U25_HOOK_CASE_WORD && rep.record_offset == off);
            CHECK(record[off] == 0x34 && record[off + 1] == 0x12);
            copy[off] = 0x34; copy[off + 1] = 0x12; CHECK(diff_count(record, copy) == 0);
        } else if (rel == 0xF7u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_WORD && rep.record_offset == 0xB8u);
            CHECK(record[0xB8] == 0x34 && record[0xB9] == 0x12);
            copy[0xB8] = 0x34; copy[0xB9] = 0x12; CHECK(diff_count(record, copy) == 0);
        } else if (rel == 0xF9u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_BYTE_BA && record[0xBA] == 0x34);
            copy[0xBA] = 0x34; CHECK(diff_count(record, copy) == 0);
        } else if (rel == 0x100u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_100);
            CHECK(record[0x10D] == 0);                 /* value >= 0x80 */
            copy[0x10D] = 0; CHECK(diff_count(record, copy) == 0);   /* not 0x87: 0x10C untouched */
        } else if (rel == 0x10Cu) {
            CHECK(rep.which == PR_U25_HOOK_CASE_10C && diff_count(record, copy) == 0);   /* 0x1234 is none of 0/0x80/0x81 */
        } else if (rel == 0x312u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_312 && cap.res_calls == 1 && cap.last_value == 0x0334);
            CHECK(rep.callout_made == 1 && diff_count(record, copy) == 0);
        } else if (rel == 0x322u || rel == 0x324u || rel == 0x326u) {
            CHECK(rep.which == PR_U25_HOOK_CASE_322 && cap.u3bd_calls == 1);
            CHECK(cap.last_value == (0x1234u & 0x7Fu) && cap.last_selector == 1u + (rel - 0x322u) / 2u);
            CHECK(rep.callout_selector == cap.last_selector && diff_count(record, copy) == 0);
        } else {
            CHECK(rep.which == PR_U25_HOOK_CASE_NONE && diff_count(record, copy) == 0);
            CHECK(cap.res_calls == 0 && cap.u3bd_calls == 0);
        }
    }
    /* addresses below the window wrap through the 16-bit subtract: 0x6AFF -> rel 0xFFFF, no case */
    {
        uint8_t record[PR_U25_HOOK_RECORD_SIZE], copy[PR_U25_HOOK_RECORD_SIZE];
        pr_u25_hook_flags flags = {0, 0}; pr_u25_hook_report rep;
        fill(record, 1); memcpy(copy, record, sizeof record);
        CHECK(pr_u25_store_hook_0a74(0x6AFF, 0, record, &flags, NULL, &rep) == PR_U25_HOOK_OK);
        CHECK(rep.which == PR_U25_HOOK_CASE_NONE && diff_count(record, copy) == 0 && flags.b84d3 == 0);
    }
    return 0;
}

static int test_flag_cases(void)
{
    uint8_t record[PR_U25_HOOK_RECORD_SIZE], copy[PR_U25_HOOK_RECORD_SIZE];
    pr_u25_hook_flags flags;
    unsigned v;
    /* rel 0: only value 0 sets [0x84D3]; the record is never touched (NULL accepted) */
    for (v = 0; v < 0x10000u; v += 0x101u) {
        flags.b84d2 = 0; flags.b84d3 = 0;
        CHECK(pr_u25_store_hook_0a74(0x6B00, (uint16_t)v, NULL, &flags, NULL, NULL) == PR_U25_HOOK_OK);
        CHECK(flags.b84d3 == (v == 0) && flags.b84d2 == 0);
    }
    flags.b84d2 = 0; flags.b84d3 = 0;
    CHECK(pr_u25_store_hook_0a74(0x6B00, 0, NULL, &flags, NULL, NULL) == PR_U25_HOOK_OK && flags.b84d3 == 1);
    /* rel 0x100: value < 0x80 (nonzero) touches nothing; 0 sets [0x84D2] only; 0x80..0xFFFF
     * clears 0x10D; 0x87 also sets 0x10C = 7 */
    for (v = 0; v < 0x10000u; v += (v < 0x200u ? 1u : 0x1FFu)) {
        pr_u25_hook_report rep;
        flags.b84d2 = 0; flags.b84d3 = 0;
        fill(record, 7); memcpy(copy, record, sizeof record);
        CHECK(pr_u25_store_hook_0a74(0x6C00, (uint16_t)v, record, &flags, NULL, &rep) == PR_U25_HOOK_OK);
        CHECK(rep.which == PR_U25_HOOK_CASE_100);
        CHECK(flags.b84d2 == (v == 0) && flags.b84d3 == 0);
        if (v >= 0x80u) { copy[0x10D] = 0; if (v == 0x87u) copy[0x10C] = 7; }
        CHECK(diff_count(record, copy) == 0);
    }
    /* rel 0x100 with value < 0x80 accepts a NULL record; value >= 0x80 does not */
    flags.b84d2 = 0;
    CHECK(pr_u25_store_hook_0a74(0x6C00, 0x7F, NULL, &flags, NULL, NULL) == PR_U25_HOOK_OK);
    CHECK(pr_u25_store_hook_0a74(0x6C00, 0x80, NULL, &flags, NULL, NULL) == PR_U25_HOOK_ARGUMENT);
    return 0;
}

static int test_value_cases(void)
{
    uint8_t record[PR_U25_HOOK_RECORD_SIZE], copy[PR_U25_HOOK_RECORD_SIZE];
    pr_u25_hook_flags flags = {0, 0};
    unsigned v;
    /* rel 0xB8: unsigned word compare with 0xB2, then the low byte of the adjusted word */
    for (v = 0; v < 0x10000u; v += (v < 0x100u ? 1u : 0xFFu)) {
        unsigned expect = (v > 0xB2u) ? ((v - 0x32u) & 0xFFu) : (v & 0xFFu);
        fill(record, 3); memcpy(copy, record, sizeof record);
        CHECK(pr_u25_store_hook_0a74(0x6BB8, (uint16_t)v, record, &flags, NULL, NULL) == PR_U25_HOOK_OK);
        CHECK(record[0x84] == expect);
        copy[0x84] = (uint8_t)expect; CHECK(diff_count(record, copy) == 0);
    }
    { static const unsigned edge[][2] = {{0xB2, 0xB2}, {0xB3, 0x81}, {0xFF, 0xCD}, {0x100, 0xCE}, {0x132, 0x00}, {0xFFFF, 0xCD}};
      size_t i;
      for (i = 0; i < sizeof edge / sizeof edge[0]; ++i) {
          fill(record, 4);
          CHECK(pr_u25_store_hook_0a74(0x6BB8, (uint16_t)edge[i][0], record, &flags, NULL, NULL) == PR_U25_HOOK_OK);
          CHECK(record[0x84] == edge[i][1]);
      } }
    /* rel 0x10C: the three recognised values and everything else */
    for (v = 0; v < 0x10000u; v += (v < 0x100u ? 1u : 0x1FFu)) {
        fill(record, 5); memcpy(copy, record, sizeof record);
        CHECK(pr_u25_store_hook_0a74(0x6C0C, (uint16_t)v, record, &flags, NULL, NULL) == PR_U25_HOOK_OK);
        if (v == 0) { copy[0x10E] = 0; copy[0x10F] = 0; }
        else if (v == 0x80u) { copy[0x10E] = 0; copy[0x10F] = 1; }
        else if (v == 0x81u) { copy[0x10E] = 1; copy[0x10F] = 1; }
        CHECK(diff_count(record, copy) == 0);
    }
    CHECK(pr_u25_store_hook_0a74(0x6C0C, 0x82, NULL, &flags, NULL, NULL) == PR_U25_HOOK_OK);        /* no record access */
    CHECK(pr_u25_store_hook_0a74(0x6C0C, 0x81, NULL, &flags, NULL, NULL) == PR_U25_HOOK_ARGUMENT);
    /* byte range: value high byte ignored; index arithmetic at both ends */
    fill(record, 6);
    CHECK(pr_u25_store_hook_0a74(0x6B20, 0xAB01, record, &flags, NULL, NULL) == PR_U25_HOOK_OK && record[0x18] == 0x01);
    CHECK(pr_u25_store_hook_0a74(0x6B70, 0xCDFF, record, &flags, NULL, NULL) == PR_U25_HOOK_OK && record[0x68] == 0xFF);
    CHECK(record[0x17] == (uint8_t)(0x17u * 5u + 6u) && record[0x69] == (uint8_t)(0x69u * 5u + 6u));
    /* word stores are little-endian */
    CHECK(pr_u25_store_hook_0a74(0x6BC3, 0xBEEF, record, &flags, NULL, NULL) == PR_U25_HOOK_OK);
    CHECK(record[0x90] == 0xEF && record[0x91] == 0xBE);
    CHECK(flags.b84d2 == 0 && flags.b84d3 == 0);
    return 0;
}

static int test_callouts(void)
{
    pr_u25_hook_flags flags = {0, 0};
    pr_u25_hook_callouts co = {on_res, on_u3bd, NULL};
    capture cap = {0, 0, 0, 0};
    pr_u25_hook_report rep;
    unsigned v;
    co.context = &cap;
    /* rel 0x312 pushes 0x0300 | low byte: AH still holds the high byte of rel */
    for (v = 0; v < 0x10000u; v += 0x7Fu) {
        cap.res_calls = 0;
        CHECK(pr_u25_store_hook_0a74(0x6E12, (uint16_t)v, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK);
        CHECK(cap.res_calls == 1 && cap.last_value == (0x0300u | (v & 0xFFu)));
        CHECK(rep.callout_value == (0x0300u | (v & 0xFFu)));
    }
    cap.res_calls = 0;
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0x42, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 0x0342);
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0xFF42, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 0x0342);
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0x0000, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 0x0300);
    /* rel 0x322/4/6: value <= 0x80 makes no call; above, (value & 0x7F) and selector */
    for (v = 0; v < 0x10000u; v += (v < 0x200u ? 1u : 0x1FFu)) {
        unsigned k;
        for (k = 0; k < 3u; ++k) {
            cap.u3bd_calls = 0; cap.last_value = 99; cap.last_selector = 99;
            CHECK(pr_u25_store_hook_0a74((uint16_t)(0x6E22u + 2u * k), (uint16_t)v, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK);
            CHECK(rep.which == PR_U25_HOOK_CASE_322 && rep.callout_selector == k + 1u);
            if (v > 0x80u) CHECK(cap.u3bd_calls == 1 && cap.last_value == (v & 0x7Fu) && cap.last_selector == k + 1u);
            else CHECK(cap.u3bd_calls == 0 && rep.callout_made == 0);
        }
    }
    /* 0x81 -> 1, 0x100 -> 0, 0xFF -> 0x7F */
    cap.u3bd_calls = 0; CHECK(pr_u25_store_hook_0a74(0x6E24, 0x81, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 1 && cap.last_selector == 2);
    CHECK(pr_u25_store_hook_0a74(0x6E26, 0x100, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 0 && cap.last_selector == 3);
    CHECK(pr_u25_store_hook_0a74(0x6E22, 0xFF, NULL, &flags, &co, &rep) == PR_U25_HOOK_OK && cap.last_value == 0x7F);
    /* missing callbacks: CALLOUT status, report still names the case and value */
    CHECK(pr_u25_store_hook_0a74(0x6E12, 0x42, NULL, &flags, NULL, &rep) == PR_U25_HOOK_CALLOUT);
    CHECK(rep.which == PR_U25_HOOK_CASE_312 && rep.callout_value == 0x0342 && rep.callout_made == 0);
    CHECK(pr_u25_store_hook_0a74(0x6E22, 0x90, NULL, &flags, NULL, &rep) == PR_U25_HOOK_CALLOUT);
    CHECK(rep.which == PR_U25_HOOK_CASE_322 && rep.callout_value == 0x10 && rep.callout_selector == 1);
    CHECK(pr_u25_store_hook_0a74(0x6E22, 0x80, NULL, &flags, NULL, &rep) == PR_U25_HOOK_OK);   /* no call needed */
    {
        pr_u25_hook_callouts half = {on_res, NULL, NULL};
        half.context = &cap;
        CHECK(pr_u25_store_hook_0a74(0x6E24, 0x90, NULL, &flags, &half, &rep) == PR_U25_HOOK_CALLOUT);
        CHECK(pr_u25_store_hook_0a74(0x6E12, 0x90, NULL, &flags, &half, &rep) == PR_U25_HOOK_OK);
    }
    CHECK(flags.b84d2 == 0 && flags.b84d3 == 0);
    return 0;
}

static int test_arguments(void)
{
    uint8_t record[PR_U25_HOOK_RECORD_SIZE];
    pr_u25_hook_flags flags = {0, 0};
    pr_u25_hook_report rep;
    fill(record, 8);
    CHECK(pr_u25_store_hook_0a74(0x6B20, 1, record, NULL, NULL, &rep) == PR_U25_HOOK_ARGUMENT);
    CHECK(rep.which == PR_U25_HOOK_CASE_NONE);
    /* record cases with a NULL record: ARGUMENT, report names the case and offset */
    CHECK(pr_u25_store_hook_0a74(0x6B20, 1, NULL, &flags, NULL, &rep) == PR_U25_HOOK_ARGUMENT);
    CHECK(rep.which == PR_U25_HOOK_CASE_BYTE_RANGE && rep.record_offset == 0x18);
    CHECK(pr_u25_store_hook_0a74(0x6BB8, 1, NULL, &flags, NULL, &rep) == PR_U25_HOOK_ARGUMENT && rep.record_offset == 0x84);
    CHECK(pr_u25_store_hook_0a74(0x6BF7, 1, NULL, &flags, NULL, &rep) == PR_U25_HOOK_ARGUMENT && rep.record_offset == 0xB8);
    CHECK(pr_u25_store_hook_0a74(0x6BF9, 1, NULL, &flags, NULL, &rep) == PR_U25_HOOK_ARGUMENT && rep.record_offset == 0xBA);
    /* non-record cases accept a NULL record */
    CHECK(pr_u25_store_hook_0a74(0x6B01, 1, NULL, &flags, NULL, &rep) == PR_U25_HOOK_OK && rep.which == PR_U25_HOOK_CASE_NONE);
    /* report pointer optional */
    CHECK(pr_u25_store_hook_0a74(0x6B30, 0x55, record, &flags, NULL, NULL) == PR_U25_HOOK_OK && record[0x28] == 0x55);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_address_map();
    rc |= test_flag_cases();
    rc |= test_value_cases();
    rc |= test_callouts();
    rc |= test_arguments();
    if (rc == 0) puts("ECL store hook +0x0A74 tests passed");
    return rc;
}
