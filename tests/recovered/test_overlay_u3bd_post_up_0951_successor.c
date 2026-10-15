#include "poolrad/recovered/overlay_u3bd_post_up_0951_successor.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

enum { RAM_BYTES = 0xA0000u };

static void word_put(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint16_t word_get(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void synthetic_frame(uint8_t *ram,
                            pr_u3bd_post_up_0938_control_state *s)
{
    memset(ram, 0, 0x4000u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x4000u;
    s->bp = 0x0100u;
    s->sp = 0x0080u;
    s->ip = 0x0951u;
    s->flags = 0x7246u;
    ram[0x00F0u] = 1u;
    word_put(ram + 0x00EEu, 0xFFFEu);
    word_put(ram + 0x00F3u, 12u);
    word_put(ram + 0x00F1u, 5u);
    ram[0x00FFu] = 4u;
    ram[0x2736u + 4u] = 0u;
    ram[0x273Fu + 4u] = 1u;
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    pr_u3bd_post_up_0938_control_state s;
    pr_u3bd_post_up_0938_control_report r;

    synthetic_frame(ram, &s);
    CHECK(pr_u3bd_post_up_0951_successor(&s, &r) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(r.loop_before == 1u && r.loop_after == 2u &&
          r.step_before == 0xFFFEu && r.step_after == 0xFFFCu);
    CHECK(r.index == 4u && r.dir_x == 0u && r.dir_y == 1u &&
          r.x_before == 12u && r.x_after == 12u &&
          r.y_before == 5u && r.y_after == 6u);
    CHECK(r.next_ip == 0x0903u && s.ip == 0x0903u &&
          s.ax == 6u && s.di == 4u && s.flags == 0x7297u);
    CHECK(ram[0x00F0u] == 2u && word_get(ram + 0x00EEu) == 0xFFFCu &&
          word_get(ram + 0x00F1u) == 6u);

    synthetic_frame(ram, &s);
    ram[0x00F0u] = 2u;
    CHECK(pr_u3bd_post_up_0951_successor(&s, &r) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(r.loop_after == 3u && r.next_ip == 0x097Fu &&
          s.ip == 0x097Fu && s.ax == 6u && s.flags == 0x7246u);

    synthetic_frame(ram, &s);
    s.ip = 0x0938u;
    CHECK(pr_u3bd_post_up_0951_successor(&s, &r) ==
          PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT);
    CHECK(s.ip == 0x0938u && ram[0x00F0u] == 1u);
    CHECK(pr_u3bd_post_up_0951_successor(NULL, NULL) ==
          PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT);

    synthetic_frame(ram, &s);
    s.ram_size = 0x200u;
    CHECK(pr_u3bd_post_up_0951_successor(&s, &r) ==
          PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY);
    CHECK(s.ip == 0x095Fu && ram[0x00F0u] == 2u &&
          word_get(ram + 0x00EEu) == 0xFFFCu);
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

static int private_pair(const char *before_path, const char *after_path)
{
    uint8_t *ram = malloc(RAM_BYTES);
    uint8_t *after = malloc(RAM_BYTES);
    pr_u3bd_post_up_0938_control_state s = {0};
    pr_u3bd_post_up_0938_control_report r;
    CHECK(ram != NULL && after != NULL);
    CHECK(load_ram(before_path, ram) && load_ram(after_path, after));
    s.ram = ram;
    s.ram_size = RAM_BYTES;
    s.ss = 0x1D16u;
    s.ds = 0x14A0u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x0951u;
    s.ax = 0u;
    s.di = 0x005Cu;
    s.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_0951_successor(&s, &r) ==
          PR_U3BD_POST_UP_0938_CONTROL_OK);
    CHECK(r.loop_before == 1u && r.loop_after == 2u &&
          r.step_before == 0xFFFEu && r.step_after == 0xFFFCu &&
          r.x_before == 12u && r.x_after == 12u &&
          r.y_before == 5u && r.y_after == 6u &&
          r.next_ip == 0x0903u);
    CHECK(s.ip == 0x0903u && s.ax == 6u && s.di == 4u &&
          s.sp == 0x3EC8u && s.flags == 0x7297u);
    CHECK(memcmp(ram, after, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; next IP=0x0903");
    free(after);
    free(ram);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 3) return 2;
    return private_pair(argv[1], argv[2]);
}
