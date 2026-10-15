#include "poolrad/recovered/overlay_u3bd_post_up_0938_control.h"

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

enum { RAM_BYTES = 0xA0000u };

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint16_t word_at(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void synthetic_frame(uint8_t *ram,
                            pr_u3bd_post_up_0938_control_state *state)
{
    memset(ram, 0, 0x4000u);
    memset(state, 0, sizeof *state);
    state->ram = ram;
    state->ram_size = 0x4000u;
    state->bp = 0x0100u;
    state->sp = 0x0080u;
    state->ip = 0x0938u;
    state->ax = 6u;
    state->di = 0x008Eu;
    state->flags = 0x7246u;
    state->known_arith_flags = 0x08D5u;
    ram[state->bp - 0x10u] = 0u;
    put_word(ram + state->bp - 0x12u, 0u);
    put_word(ram + state->bp - 0x0Du, 12u);
    put_word(ram + state->bp - 0x0Fu, 4u);
    ram[state->bp - 1u] = 4u;
    ram[0x2736u + 4u] = 0u;
    ram[0x273Fu + 4u] = 1u;
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    uint8_t before[sizeof ram];
    pr_u3bd_post_up_0938_control_state state;
    pr_u3bd_post_up_0938_control_report report;

    synthetic_frame(ram, &state);
    memcpy(before, ram, sizeof ram);
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(report.loop_before == 0u && report.loop_after == 1u &&
          report.step_before == 0u && report.step_after == 0xFFFEu);
    CHECK(report.index == 4u && report.dir_x == 0u && report.dir_y == 1u);
    CHECK(report.x_before == 12u && report.x_after == 12u &&
          report.y_before == 4u && report.y_after == 5u);
    CHECK(report.next_ip == 0x0903u && state.ip == 0x0903u);
    CHECK(state.ax == 5u && state.di == 4u && state.flags == 0x7293u &&
          state.known_arith_flags == 0x08D5u);
    CHECK(state.bp == 0x0100u && state.sp == 0x0080u);
    CHECK(ram[state.bp - 0x10u] == 1u &&
          word_at(ram + state.bp - 0x12u) == 0xFFFEu &&
          word_at(ram + state.bp - 0x0Du) == 12u &&
          word_at(ram + state.bp - 0x0Fu) == 5u);
    CHECK(ram[0x2736u + 4u] == before[0x2736u + 4u]);

    synthetic_frame(ram, &state);
    ram[state.bp - 0x10u] = 2u;
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(report.loop_after == 3u && report.next_ip == 0x097Fu &&
          state.ip == 0x097Fu && state.flags == 0x7246u);

    synthetic_frame(ram, &state);
    ram[0x2736u + 4u] = 0xFFu;
    ram[0x273Fu + 4u] = 0xFEu;
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(report.dir_x == 0xFFu && report.dir_y == 0xFEu &&
          report.x_after == 11u && report.y_after == 2u);
    CHECK(state.ax == 2u);

    synthetic_frame(ram, &state);
    state.ram = NULL;
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY);
    CHECK(state.ip == 0x0951u && state.ax == 6u &&
          state.di == 0x008Eu && state.flags == 0x7246u);
    CHECK(pr_u3bd_post_up_0938_control(NULL, NULL) ==
          PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT);

    synthetic_frame(ram, &state);
    state.ram_size = 0x200u; /* All SS locals, no DS direction table. */
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY);
    CHECK(state.ip == 0x095Fu && state.ax == 4u && state.di == 4u);
    CHECK(ram[state.bp - 0x10u] == 1u &&
          word_at(ram + state.bp - 0x12u) == 0xFFFEu &&
          word_at(ram + state.bp - 0x0Du) == 12u);

    synthetic_frame(ram, &state);
    ram[0x2736u + 4u] = 0xFFu;
    state.ram_size = 0x273Bu; /* X table byte reached; Y byte missing. */
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY);
    CHECK(state.ip == 0x0971u && state.di == 4u &&
          word_at(ram + state.bp - 0x0Du) == 11u &&
          word_at(ram + state.bp - 0x0Fu) == 4u);
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
    pr_u3bd_post_up_0938_control_state state = {0};
    pr_u3bd_post_up_0938_control_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    state.ram = pre;
    state.ram_size = RAM_BYTES;
    state.ss = 0x1D16u;
    state.ds = 0x14A0u;
    state.bp = 0x3EE0u;
    state.sp = 0x3EC8u;
    state.ip = 0x0938u;
    state.ax = 6u;
    state.di = 0x008Eu;
    state.flags = 0x7246u;
    state.known_arith_flags = 0x08D5u;
    CHECK(pr_u3bd_post_up_0938_control(&state, &report) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(report.loop_before == 0u && report.loop_after == 1u &&
          report.step_before == 0u && report.step_after == 0xFFFEu);
    CHECK(report.index == 4u && report.dir_x == 0u && report.dir_y == 1u);
    CHECK(report.x_before == 12u && report.x_after == 12u &&
          report.y_before == 4u && report.y_after == 5u);
    CHECK(state.ip == 0x0903u && state.ax == 5u && state.di == 4u &&
          state.flags == 0x7293u && state.sp == 0x3EC8u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; next IP=0x0903");
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
