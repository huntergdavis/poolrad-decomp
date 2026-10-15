#include "poolrad/recovered/overlay_u3bd_post_up_outer_loop_gate.h"

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
    pr_u3bd_post_up_outer_loop_gate_state state = {0};
    pr_u3bd_post_up_outer_loop_gate_report report;

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 0x40u;
    state.ax = 4u;
    state.flags = 0x7246u;
    put_word(ss + state.bp - 8u, 5u);
    put_word(ss + state.bp - 11u, 6u);
    memcpy(before, ss, sizeof ss);
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, &report) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK);
    CHECK(report.lhs == 5u && report.rhs == 6u &&
          report.next_ip == 0x026Eu);
    CHECK(state.ax == 5u && state.flags == 0x7297u);
    CHECK(memcmp(ss, before, sizeof ss) == 0);

    put_word(ss + state.bp - 8u, 6u);
    state.flags = 0x7202u;
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, &report) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK);
    CHECK(report.next_ip == 0x0319u && state.ax == 6u &&
          state.flags == 0x7246u);

    state.ss_size = state.bp - 8u + 1u;
    state.ax = 0x1234u;
    state.flags = 0x4321u;
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, &report) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_BOUNDARY);
    CHECK(state.ax == 0x1234u && state.flags == 0x4321u);
    state.ss = NULL;
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, &report) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_ARGUMENT);

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 3u;
    put_word(ss + 0xFFFBu, 1u);
    put_word(ss + 0xFFF8u, 2u);
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, NULL) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK);
    CHECK(state.ax == 1u);
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
    pr_u3bd_post_up_outer_loop_gate_state state = {0};
    pr_u3bd_post_up_outer_loop_gate_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    state.ss = pre + SS_BASE;
    state.ss_size = 0x10000u;
    state.bp = 0x3EBAu;
    state.ax = 4u;
    state.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_outer_loop_gate(&state, &report) ==
          PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK);
    CHECK(report.lhs == 5u && report.rhs == 6u &&
          report.next_ip == 0x026Eu);
    CHECK(state.ax == 5u && state.flags == 0x7297u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; AX=5 next IP=0x026E");
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
