#include "poolrad/recovered/overlay_u3bd_post_up_0319_exit.h"

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
    pr_u3bd_post_up_0319_exit_state state = {0};
    pr_u3bd_post_up_0319_exit_report report = {0};

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 0x40u;
    state.sp = 0x20u;
    state.cs = 0x9999u;
    state.ip = 0x0319u;
    state.ax = 6u;
    state.flags = 0x7246u;
    put_word(ss + 0x40u, 0x1234u);
    put_word(ss + 0x42u, 0x5678u);
    put_word(ss + 0x44u, 0x9ABCu);
    memcpy(before, ss, sizeof ss);
    CHECK(pr_u3bd_post_up_0319_exit(&state, &report) ==
          PR_U3BD_POST_UP_0319_EXIT_OK);
    CHECK(state.bp == 0x1234u && state.sp == 0x4Eu);
    CHECK(state.cs == 0x9ABCu && state.ip == 0x5678u);
    CHECK(state.ax == 6u && state.flags == 0x7246u);
    CHECK(report.saved_bp == 0x1234u && report.return_ip == 0x5678u &&
          report.return_cs == 0x9ABCu && report.exit_sp == 0x4Eu);
    CHECK(memcmp(ss, before, sizeof ss) == 0);

    state.bp = 0x40u;
    state.sp = 0x20u;
    state.cs = 0x9999u;
    state.ip = 0x0319u;
    state.ss_size = 0x41u;
    CHECK(pr_u3bd_post_up_0319_exit(&state, &report) ==
          PR_U3BD_POST_UP_0319_EXIT_BOUNDARY);
    CHECK(state.bp == 0x40u && state.sp == 0x40u &&
          state.cs == 0x9999u && state.ip == 0x0319u);

    state.bp = 0x40u;
    state.sp = 0x20u;
    state.ss_size = 0x43u;
    CHECK(pr_u3bd_post_up_0319_exit(&state, &report) ==
          PR_U3BD_POST_UP_0319_EXIT_BOUNDARY);
    CHECK(state.bp == 0x1234u && state.sp == 0x42u &&
          state.cs == 0x9999u && state.ip == 0x0319u);

    state.bp = 0x40u;
    state.sp = 0x20u;
    state.ss_size = 0x45u;
    CHECK(pr_u3bd_post_up_0319_exit(&state, &report) ==
          PR_U3BD_POST_UP_0319_EXIT_BOUNDARY);
    CHECK(state.bp == 0x1234u && state.sp == 0x42u &&
          state.cs == 0x9999u && state.ip == 0x0319u);

    state.ss = NULL;
    state.bp = 0x40u;
    state.sp = 0x20u;
    CHECK(pr_u3bd_post_up_0319_exit(&state, NULL) ==
          PR_U3BD_POST_UP_0319_EXIT_BOUNDARY);
    CHECK(state.sp == 0x40u);
    CHECK(pr_u3bd_post_up_0319_exit(NULL, NULL) ==
          PR_U3BD_POST_UP_0319_EXIT_ARGUMENT);

    state.ss = ss;
    state.ss_size = sizeof ss;
    state.bp = 0xFFFCu;
    state.sp = 1u;
    put_word(ss + 0xFFFCu, 0x1111u);
    put_word(ss + 0xFFFEu, 0x2222u);
    put_word(ss, 0x3333u);
    CHECK(pr_u3bd_post_up_0319_exit(&state, NULL) ==
          PR_U3BD_POST_UP_0319_EXIT_OK);
    CHECK(state.bp == 0x1111u && state.sp == 0x000Au &&
          state.ip == 0x2222u && state.cs == 0x3333u);
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
    pr_u3bd_post_up_0319_exit_state state = {0};
    pr_u3bd_post_up_0319_exit_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    state.ss = pre + SS_BASE;
    state.ss_size = 0x10000u;
    state.bp = 0x3EBAu;
    state.sp = 0x3EACu;
    state.cs = 0x2D14u;
    state.ip = 0x0319u;
    state.ax = 6u;
    state.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_0319_exit(&state, &report) ==
          PR_U3BD_POST_UP_0319_EXIT_OK);
    CHECK(report.saved_bp == 0x3EE0u && report.return_cs == 0x2D14u &&
          report.return_ip == 0x0938u && report.exit_sp == 0x3EC8u);
    CHECK(state.bp == 0x3EE0u && state.sp == 0x3EC8u &&
          state.cs == 0x2D14u && state.ip == 0x0938u &&
          state.ax == 6u && state.flags == 0x7246u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; far return 2D14:0938");
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
