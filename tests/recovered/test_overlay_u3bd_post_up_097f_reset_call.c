#include "poolrad/recovered/overlay_u3bd_post_up_097f_reset_call.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

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
                            pr_u3bd_post_up_097f_reset_call_state *s)
{
    memset(ram, 0, 0x4000u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x4000u;
    s->cs = 0x2D14u;
    s->bp = 0x0100u;
    s->sp = 0x0080u;
    s->ip = 0x097Fu;
    s->ax = 7u;
    s->flags = 0x7246u;
    put_word(ram + 0x00FBu, 12u);
    put_word(ram + 0x00F9u, 4u);
    put_word(ram + 0x00F3u, 12u);
    put_word(ram + 0x00F1u, 7u);
    ram[0x00F0u] = 3u;
    put_word(ram + 0x00EEu, 0xFFFAu);
    ram[0x00FEu] = 0u;
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    pr_u3bd_post_up_097f_reset_call_state s;
    pr_u3bd_post_up_097f_reset_call_report r;

    synthetic_frame(ram, &s);
    CHECK(pr_u3bd_post_up_097f_reset_call(&s, &r) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_OK);
    CHECK(r.x == 12u && r.y == 4u && r.query == 0u &&
          r.pushed_x == 12u && r.pushed_y == 4u &&
          r.pushed_query == 0u && r.pushed_cs == 0x2D14u);
    CHECK(s.ip == 0x09A7u && s.sp == 0x0078u && s.bp == 0x0100u &&
          s.ax == 0u && s.flags == 0x7293u &&
          s.known_arith_flags == 0x08D5u);
    CHECK(word_at(ram + 0x00F3u) == 12u &&
          word_at(ram + 0x00F1u) == 4u &&
          ram[0x00F0u] == 0u && word_at(ram + 0x00EEu) == 0u);
    CHECK(word_at(ram + 0x0078u) == 0x2D14u &&
          word_at(ram + 0x007Au) == 0u &&
          word_at(ram + 0x007Cu) == 4u &&
          word_at(ram + 0x007Eu) == 12u);

    synthetic_frame(ram, &s);
    s.ram = NULL;
    CHECK(pr_u3bd_post_up_097f_reset_call(&s, &r) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY);
    CHECK(s.ip == 0x097Fu && s.ax == 7u && s.sp == 0x0080u);

    synthetic_frame(ram, &s);
    s.sp = 0x5000u;
    CHECK(pr_u3bd_post_up_097f_reset_call(&s, &r) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY);
    CHECK(s.ip == 0x099Du && s.sp == 0x5000u && s.ax == 12u &&
          ram[0x00F0u] == 0u && word_at(ram + 0x00EEu) == 0u);

    synthetic_frame(ram, &s);
    s.ip = 0x09A7u;
    CHECK(pr_u3bd_post_up_097f_reset_call(&s, &r) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_ARGUMENT);
    CHECK(s.ip == 0x09A7u && ram[0x00F0u] == 3u);
    CHECK(pr_u3bd_post_up_097f_reset_call(NULL, NULL) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_ARGUMENT);
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
    pr_u3bd_post_up_097f_reset_call_state s = {0};
    pr_u3bd_post_up_097f_reset_call_report r;
    CHECK(ram != NULL && after != NULL);
    CHECK(load_ram(before_path, ram) && load_ram(after_path, after));
    s.ram = ram;
    s.ram_size = RAM_BYTES;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x097Fu;
    s.ax = 7u;
    s.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_097f_reset_call(&s, &r) ==
          PR_U3BD_POST_UP_097F_RESET_CALL_OK);
    CHECK(r.x == 12u && r.y == 4u && r.query == 0u &&
          r.pushed_x == 12u && r.pushed_y == 4u &&
          r.pushed_query == 0u && r.pushed_cs == 0x2D14u);
    CHECK(s.ip == 0x09A7u && s.sp == 0x3EC0u &&
          s.ax == 0u && s.flags == 0x7293u);
    CHECK(memcmp(ram, after, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; next IP=0x09A7");
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
