#include "poolrad/recovered/overlay_u3bd_post_up_05fb_wrapper_return.h"

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

static void init(uint8_t *ram, pr_u3bd_post_up_05fb_state *s)
{
    memset(ram, 0, 0x400u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x400u;
    s->cs = 0x1111u;
    s->ss = 0;
    s->bp = 0x0200u;
    s->sp = 0x0100u;
    s->ip = 0x05FBu;
    s->ax = 0x0100u;
    s->di = 0x4EC0u;
    s->flags = 0x7246u;
    put_word(ram + 0x0100u, 0x0300u);
    put_word(ram + 0x0102u, 0x0DAEu);
    put_word(ram + 0x0104u, 0x2D14u);
}

static int synthetic(void)
{
    uint8_t ram[0x400u];
    pr_u3bd_post_up_05fb_state s;

    init(ram, &s);
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(&s) ==
          PR_U3BD_POST_UP_05FB_OK);
    CHECK(s.cs == 0x2D14u && s.ip == 0x0DAEu && s.bp == 0x0300u &&
          s.sp == 0x0106u && s.ax == 0x0100u && s.di == 0x4EC0u &&
          s.flags == 0x7246u);

    init(ram, &s);
    s.ram_size = 0x0100u;
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(&s) ==
          PR_U3BD_POST_UP_05FB_BOUNDARY);
    CHECK(s.ip == 0x05FBu && s.sp == 0x0100u && s.bp == 0x0200u);

    init(ram, &s);
    s.ram_size = 0x0104u;
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(&s) ==
          PR_U3BD_POST_UP_05FB_BOUNDARY);
    CHECK(s.cs == 0x1111u && s.ip == 0x05FCu && s.sp == 0x0102u &&
          s.bp == 0x0300u);

    init(ram, &s);
    s.ip = 0x05FCu;
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(&s) ==
          PR_U3BD_POST_UP_05FB_ARGUMENT);
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(NULL) ==
          PR_U3BD_POST_UP_05FB_ARGUMENT);
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
    pr_u3bd_post_up_05fb_state s = {0};
    CHECK(ram != NULL && after != NULL);
    CHECK(load_ram(before_path, ram) && load_ram(after_path, after));
    s.ram = ram;
    s.ram_size = RAM_BYTES;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EC2u;
    s.sp = 0x3EC2u;
    s.ip = 0x05FBu;
    s.ax = 0x0100u;
    s.di = 0x4EC0u;
    s.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_05fb_wrapper_return(&s) ==
          PR_U3BD_POST_UP_05FB_OK);
    CHECK(s.cs == 0x2D14u && s.ip == 0x0DAEu && s.bp == 0x3EE0u &&
          s.sp == 0x3EC8u && s.ax == 0x0100u && s.di == 0x4EC0u &&
          s.flags == 0x7246u);
    CHECK(memcmp(ram, after, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact at +0DAE return");
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
