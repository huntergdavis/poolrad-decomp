#include "poolrad/recovered/overlay_u3bd_post_up_026e_arm.h"

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

static uint16_t word_at(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int synthetic(void)
{
    uint8_t ss[0x10000u] = {0};
    uint8_t before[sizeof ss];
    pr_u3bd_post_up_026e_arm_state state = {0};
    pr_u3bd_post_up_026e_arm_report report;
    unsigned changes;

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 0x40u;
    state.ax = 5u;
    state.flags = 0x7297u;
    put_word(ss + state.bp - 8u, 5u);
    put_word(ss + state.bp - 2u, 4u);
    put_word(ss + state.bp - 13u, 4u);
    put_word(ss + state.bp - 6u, 4u);
    ss[state.bp + 12u] = 4u;
    memcpy(before, ss, sizeof ss);
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_OK);
    CHECK(report.outer_before == 5u && report.outer_after == 6u &&
          report.copied_word == 4u && report.source_byte == 4u &&
          report.compared_ax == 4u && report.next_ip == 0x028Bu);
    CHECK(state.ax == 4u && state.flags == 0x7246u);
    changes = 0u;
    for (size_t i = 0u; i < sizeof ss; ++i) {
        if (ss[i] != before[i]) {
            ++changes;
            CHECK(i == state.bp - 8u && before[i] == 5u && ss[i] == 6u);
        }
    }
    CHECK(changes == 1u);

    /* Source 5 exceeds copied word 4: skip the inner-word store. */
    ss[state.bp + 12u] = 5u;
    put_word(ss + state.bp - 6u, 0xBEEFu);
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_OK);
    CHECK(report.next_ip == 0x030Eu && state.ax == 5u &&
          state.flags == 0x7202u);
    CHECK(word_at(ss + state.bp - 6u) == 0xBEEFu);

    /* CBW is signed: byte 0xFF becomes AX=0xFFFF and takes JLE. */
    put_word(ss + state.bp - 2u, 0u);
    ss[state.bp + 12u] = 0xFFu;
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_OK);
    CHECK(report.next_ip == 0x028Bu && state.ax == 0xFFFFu);
    CHECK(word_at(ss + state.bp - 6u) == 0xFFFFu);

    /* A reached missing source byte keeps the prior INC and copy effects. */
    put_word(ss + state.bp - 8u, 5u);
    put_word(ss + state.bp - 2u, 4u);
    put_word(ss + state.bp - 13u, 9u);
    state.flags = 0x7202u;
    state.ss_size = state.bp + 12u;
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_BOUNDARY);
    CHECK(word_at(ss + state.bp - 8u) == 6u);
    CHECK(word_at(ss + state.bp - 13u) == 4u);
    CHECK(state.ax == 4u && state.flags == 0x7206u);

    state.ss_size = state.bp - 8u + 1u;
    state.ax = 0x1234u;
    state.flags = 0x4321u;
    memcpy(before, ss, sizeof ss);
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_BOUNDARY);
    CHECK(state.ax == 0x1234u && state.flags == 0x4321u);
    CHECK(memcmp(ss, before, sizeof ss) == 0);
    state.ss = NULL;
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_ARGUMENT);
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
    pr_u3bd_post_up_026e_arm_state state = {0};
    pr_u3bd_post_up_026e_arm_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    state.ss = pre + SS_BASE;
    state.ss_size = 0x10000u;
    state.bp = 0x3EBAu;
    state.ax = 5u;
    state.flags = 0x7297u;
    CHECK(word_at(state.ss + state.bp - 8u) == 5u);
    CHECK(word_at(state.ss + state.bp - 2u) == 4u);
    CHECK(state.ss[state.bp + 12u] == 4u);
    CHECK(pr_u3bd_post_up_026e_arm(&state, &report) ==
          PR_U3BD_POST_UP_026E_ARM_OK);
    CHECK(report.outer_before == 5u && report.outer_after == 6u &&
          report.next_ip == 0x028Bu);
    CHECK(state.ax == 4u && state.flags == 0x7246u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; outer 5->6, AX=4, IP=0x028B");
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
