#include "poolrad/recovered/overlay_u3bd_post_up_0903_helper_call.h"

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

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & 0xFFFFFu;
}

static uint16_t word_at(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return (uint16_t)(ram[at(seg, off)] |
                      ((uint16_t)ram[at(seg, (uint16_t)(off + 1u))] << 8));
}

static void init(uint8_t *ram, pr_u3bd_post_up_0903_helper_call_state *s)
{
    memset(ram, 0xA5, 0x4000u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x4000u;
    s->cs = 0x2D14u;
    s->bp = 0x0120u;
    s->sp = 0x0100u;
    s->ip = 0x0903u;
    s->ax = 0xAB05u;
    s->flags = 0x7293u;
    ram[s->bp - 0x0Du] = 12u;
    ram[s->bp - 0x0Fu] = 5u;
    ram[s->bp - 1u] = 4u;
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    pr_u3bd_post_up_0903_helper_call_state s;
    pr_u3bd_post_up_0903_helper_call_report r;

    init(ram, &s);
    CHECK(pr_u3bd_post_up_0903_helper_call(&s, &r) ==
          PR_U3BD_POST_UP_0903_HELPER_CALLEE_047B);
    CHECK(r.x == 12u && r.y == 5u && r.selector == 4u &&
          r.return_ip == 0x0913u);
    CHECK(s.sp == 0x00F6u && s.ip == 0x047Bu && s.ax == 0xAB04u &&
          s.flags == 0x7293u);
    CHECK(word_at(ram, s.ss, 0x00F6u) == 0x0913u);
    CHECK(word_at(ram, s.ss, 0x00F8u) == 0x2D14u);
    CHECK(word_at(ram, s.ss, 0x00FAu) == 0xAB04u);
    CHECK(word_at(ram, s.ss, 0x00FCu) == 0xAB05u);
    CHECK(word_at(ram, s.ss, 0x00FEu) == 0xAB0Cu);

    init(ram, &s);
    s.sp = 0x4001u; /* First local read exists; first stack word does not. */
    CHECK(pr_u3bd_post_up_0903_helper_call(&s, NULL) ==
          PR_U3BD_POST_UP_0903_HELPER_BOUNDARY);
    CHECK(s.ip == 0x0906u && s.sp == 0x4001u && s.ax == 0xAB0Cu &&
          s.flags == 0x7293u);

    init(ram, &s);
    s.bp = 0x400Du;
    CHECK(pr_u3bd_post_up_0903_helper_call(&s, NULL) ==
          PR_U3BD_POST_UP_0903_HELPER_BOUNDARY);
    CHECK(s.ip == 0x0903u && s.sp == 0x0100u && s.ax == 0xAB05u);

    init(ram, &s);
    s.ip = 0x0904u;
    CHECK(pr_u3bd_post_up_0903_helper_call(&s, NULL) ==
          PR_U3BD_POST_UP_0903_HELPER_ARGUMENT);
    CHECK(s.ip == 0x0904u && s.sp == 0x0100u);
    CHECK(pr_u3bd_post_up_0903_helper_call(NULL, NULL) ==
          PR_U3BD_POST_UP_0903_HELPER_ARGUMENT);
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

static int private_pair(const char *pre_path, const char *entry_path)
{
    uint8_t *pre = malloc(RAM_BYTES);
    uint8_t *entry = malloc(RAM_BYTES);
    pr_u3bd_post_up_0903_helper_call_state s = {0};
    pr_u3bd_post_up_0903_helper_call_report r;
    CHECK(pre != NULL && entry != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(entry_path, entry));
    s.ram = pre;
    s.ram_size = RAM_BYTES;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x0903u;
    s.ax = 5u;
    s.flags = 0x7293u;
    CHECK(pr_u3bd_post_up_0903_helper_call(&s, &r) ==
          PR_U3BD_POST_UP_0903_HELPER_CALLEE_047B);
    CHECK(r.x == 12u && r.y == 5u && r.selector == 4u);
    CHECK(s.ip == 0x047Bu && s.ax == 4u && s.sp == 0x3EBEu &&
          s.flags == 0x7293u);
    CHECK(memcmp(pre, entry, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact at +047B entry");
    free(entry);
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
