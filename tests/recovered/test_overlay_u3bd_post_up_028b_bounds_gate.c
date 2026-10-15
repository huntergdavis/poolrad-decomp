#include "poolrad/recovered/overlay_u3bd_post_up_028b_bounds_gate.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d\n", __LINE__); \
        return 1; \
    } \
} while (0)

enum { RAM_BYTES = 0xA0000u, SS_BASE = 0x1D160u };

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int synthetic(void)
{
    uint8_t ss[0x10000u] = {0};
    uint8_t before[sizeof ss];
    pr_u3bd_post_up_028b_bounds_state state = {0};
    pr_u3bd_post_up_028b_bounds_report report;
    struct case_entry {
        uint16_t outer, inner, next_ip;
        unsigned comparisons;
    } cases[] = {
        {6u, 4u, 0x02A3u, 4u},
        {0u, 10u, 0x02A3u, 4u},
        {0xFFFFu, 4u, 0x0300u, 1u},
        {11u, 4u, 0x0300u, 2u},
        {4u, 0xFFFFu, 0x0300u, 3u},
        {4u, 11u, 0x0300u, 4u}
    };
    size_t i;

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 0x40u;
    for (i = 0u; i < sizeof cases / sizeof cases[0]; ++i) {
        put_word(ss + state.bp - 8u, cases[i].outer);
        put_word(ss + state.bp - 6u, cases[i].inner);
        state.flags = 0x7246u;
        memcpy(before, ss, sizeof ss);
        CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
              PR_U3BD_POST_UP_028B_BOUNDS_OK);
        CHECK(report.outer == cases[i].outer &&
              report.next_ip == cases[i].next_ip &&
              report.comparisons == cases[i].comparisons);
        if (cases[i].comparisons >= 3u)
            CHECK(report.inner == cases[i].inner);
        CHECK(memcmp(ss, before, sizeof ss) == 0);
        if (i == 0u) CHECK(state.flags == 0x7297u);
    }

    /* An outer exit never reaches an unavailable inner word. */
    state.ss_size = state.bp - 8u + 2u;
    put_word(ss + state.bp - 8u, 0xFFFFu);
    CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
          PR_U3BD_POST_UP_028B_BOUNDS_OK);
    CHECK(report.next_ip == 0x0300u && report.comparisons == 1u);
    state.ss_size = sizeof ss;

    /* A reached missing inner word keeps the second outer CMP flags. */
    put_word(ss + state.bp - 8u, 6u);
    state.ss_size = state.bp - 6u + 1u;
    state.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
          PR_U3BD_POST_UP_028B_BOUNDS_BOUNDARY);
    CHECK(state.flags == 0x7297u); /* 6 - 10, before inner CMP. */

    /* An unavailable first word causes no flag or memory change. */
    state.ss_size = state.bp - 8u + 1u;
    state.flags = 0x4321u;
    memcpy(before, ss, sizeof ss);
    CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
          PR_U3BD_POST_UP_028B_BOUNDS_BOUNDARY);
    CHECK(state.flags == 0x4321u && memcmp(ss, before, sizeof ss) == 0);
    state.ss = NULL;
    CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
          PR_U3BD_POST_UP_028B_BOUNDS_ARGUMENT);
    return 0;
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    size_t count;
    int extra;
    if (fp == NULL) return 0;
    count = fread(ram, 1u, RAM_BYTES, fp);
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return count == RAM_BYTES && extra == EOF;
}

static int private_pair(const char *pre_path, const char *post_path)
{
    uint8_t *pre = malloc(RAM_BYTES);
    uint8_t *post = malloc(RAM_BYTES);
    pr_u3bd_post_up_028b_bounds_state state = {0};
    pr_u3bd_post_up_028b_bounds_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    state.ss = pre + SS_BASE;
    state.ss_size = 0x10000u;
    state.bp = 0x3EBAu;
    state.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_028b_bounds_gate(&state, &report) ==
          PR_U3BD_POST_UP_028B_BOUNDS_OK);
    CHECK(report.outer == 6u && report.inner == 4u &&
          report.comparisons == 4u && report.next_ip == 0x02A3u);
    CHECK(state.flags == 0x7297u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; outer 6, inner 4, IP=0x02A3");
    free(post);
    free(pre);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 3) return 2;
    return private_pair(argv[1], argv[2]);
}
