#include "poolrad/recovered/overlay_u3bd_post_up_flush_entry.h"

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

static void init(uint8_t *ram, pr_u3bd_post_up_flush_entry_state *s)
{
    memset(ram, 0, 0x4000u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x4000u;
    s->cs = 0x0100u;
    s->bp = 0x0300u;
    s->sp = 0x0200u;
    s->ip = 0x0DAAu;
    s->ax = 4u;
    s->flags = 0x7246u;
    put_word(ram + 0x1000u + 0x05F7u, 0x1B52u);
    put_word(ram + 0x1000u + 0x05F9u, 0x0200u);
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    pr_u3bd_post_up_flush_entry_state s;
    pr_u3bd_post_up_flush_entry_report r;

    init(ram, &s);
    CHECK(pr_u3bd_post_up_flush_entry(&s, &r) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_OK);
    CHECK(r.near_return_ip == 0x0DAEu && r.far_return_ip == 0x05FBu &&
          r.target_cs == 0x0200u && r.target_ip == 0x1B52u);
    CHECK(s.cs == 0x0200u && s.ip == 0x1B52u && s.sp == 0x01F6u &&
          s.bp == 0x01FAu && s.ax == 4u && s.flags == 0x7246u);
    CHECK(word_at(ram + 0x01F6u) == 0x05FBu &&
          word_at(ram + 0x01F8u) == 0x0100u &&
          word_at(ram + 0x01FAu) == 0x0300u &&
          word_at(ram + 0x01FCu) == 0x0DAEu &&
          word_at(ram + 0x01FEu) == 0x0100u);

    init(ram, &s);
    s.ram = NULL;
    CHECK(pr_u3bd_post_up_flush_entry(&s, NULL) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY);
    CHECK(s.ip == 0x0DAAu && s.sp == 0x0200u && s.bp == 0x0300u);

    init(ram, &s);
    s.ram_size = 0x1000u; /* Stack available, relocated target absent. */
    CHECK(pr_u3bd_post_up_flush_entry(&s, NULL) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY);
    CHECK(s.cs == 0x0100u && s.ip == 0x05F6u && s.sp == 0x01FAu &&
          s.bp == 0x01FAu && word_at(ram + 0x01FCu) == 0x0DAEu);

    init(ram, &s);
    s.ip = 0x05F3u;
    CHECK(pr_u3bd_post_up_flush_entry(&s, NULL) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_ARGUMENT);
    CHECK(s.ip == 0x05F3u && s.sp == 0x0200u);
    CHECK(pr_u3bd_post_up_flush_entry(NULL, NULL) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_ARGUMENT);
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
    pr_u3bd_post_up_flush_entry_state s = {0};
    pr_u3bd_post_up_flush_entry_report r;
    CHECK(ram != NULL && after != NULL);
    CHECK(load_ram(before_path, ram) && load_ram(after_path, after));
    s.ram = ram;
    s.ram_size = RAM_BYTES;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x0DAAu;
    s.ax = 4u;
    s.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_flush_entry(&s, &r) ==
          PR_U3BD_POST_UP_FLUSH_ENTRY_OK);
    CHECK(r.near_return_ip == 0x0DAEu && r.far_return_ip == 0x05FBu &&
          r.target_cs == 0x0C88u && r.target_ip == 0x1B52u);
    CHECK(s.cs == 0x0C88u && s.ip == 0x1B52u && s.sp == 0x3EBEu &&
          s.bp == 0x3EC2u && s.ax == 4u && s.flags == 0x7246u);
    CHECK(memcmp(ram, after, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact at resident flush entry");
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
