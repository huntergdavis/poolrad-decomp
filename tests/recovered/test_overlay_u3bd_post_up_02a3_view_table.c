#include "poolrad/recovered/overlay_u3bd_post_up_02a3_view_table.h"

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

enum { RAM_BYTES = 0xA0000u, SS = 0x1D16u, DS = 0x14A0u, BP = 0x3EBAu };

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void seed(uint8_t *ram, pr_u3bd_post_up_02a3_view_table_state *s)
{
    uint32_t far = physical(DS, 0x6A58u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = RAM_BYTES;
    s->ss = SS;
    s->ds = DS;
    s->bp = BP;
    s->ax = 4u;
    s->flags = 0x7297u;
    s->known_arith_flags = 0x08D5u;
    ram[physical(SS, BP - 9u)] = 5u;
    ram[physical(SS, BP + 8u)] = 14u;
    put_word(ram + far, 8u);
    put_word(ram + far + 2u, 0x3D0Eu);
    ram[physical(0x3D0Eu, 0x07F9u)] = 19u;
}

static int synthetic(void)
{
    uint8_t *ram = calloc(RAM_BYTES, 1u);
    uint8_t *before = malloc(RAM_BYTES);
    pr_u3bd_post_up_02a3_view_table_state s;
    pr_u3bd_post_up_02a3_view_table_report report;
    uint32_t far;
    CHECK(ram != NULL && before != NULL);
    seed(ram, &s);
    memcpy(before, ram, RAM_BYTES);
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_OK);
    CHECK(report.column == 5u && report.slice == 14u && report.item == 19u);
    CHECK(report.item_offset == 0x07F9u &&
          report.item_physical == 0x3D8D9u && report.next_ip == 0x02C4u);
    CHECK(s.ax == 0x0888u && s.cx == 5u && s.dx == 0u &&
          s.es == 0x3D0Eu && s.di == 0x0895u);
    CHECK(s.flags == 0x7202u && s.known_arith_flags == 0x08D5u);
    CHECK(memcmp(ram, before, RAM_BYTES) == 0);

    /* Zero byte takes the unsigned-below-or-equal branch. */
    seed(ram, &s);
    ram[physical(0x3D0Eu, 0x07F9u)] = 0u;
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_OK);
    CHECK(report.item == 0u && report.next_ip == 0x0300u);
    CHECK(s.flags == 0x7246u);

    /* A first missing stack operand leaves all register effects untouched. */
    seed(ram, &s);
    s.ram_size = physical(SS, BP - 9u);
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY);
    CHECK(s.ax == 4u && s.cx == 0u && s.flags == 0x7297u);

    /* A missing second operand retains the first MOV/XOR/MOV effects. */
    seed(ram, &s);
    s.ram_size = physical(SS, BP + 8u);
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY);
    CHECK(s.ax == 5u && s.cx == 5u && s.dx == 0u);
    CHECK(s.known_arith_flags == (0x08D5u & ~0x0010u));

    /* A truncated far pointer stops after the reached unsigned MUL. */
    seed(ram, &s);
    s.ds = 0x2500u;
    far = physical(s.ds, 0x6A58u);
    s.ram_size = far + 3u;
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY);
    CHECK(s.ax == 0x0888u && s.cx == 5u && s.dx == 0u);
    CHECK(s.es == 0u && s.di == 0u && s.known_arith_flags == 0x0801u);

    /* A missing item stops after LES and both ADDs; memory stays read-only. */
    seed(ram, &s);
    put_word(ram + physical(DS, 0x6A58u) + 2u, 0x4000u);
    s.ram_size = 0x30000u;
    memcpy(before, ram, RAM_BYTES);
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_BOUNDARY);
    CHECK(s.es == 0x4000u && s.di == 0x0895u &&
          s.known_arith_flags == 0x08D5u);
    CHECK(memcmp(ram, before, RAM_BYTES) == 0);

    s.ram = NULL;
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_ARGUMENT);
    free(before);
    free(ram);
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
    pr_u3bd_post_up_02a3_view_table_state s = {0};
    pr_u3bd_post_up_02a3_view_table_report report;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    s.ram = pre;
    s.ram_size = RAM_BYTES;
    s.ss = SS;
    s.ds = DS;
    s.bp = BP;
    s.ax = 4u;
    s.flags = 0x7297u;
    s.known_arith_flags = 0x08D5u;
    CHECK(pr_u3bd_post_up_02a3_view_table(&s, &report) ==
          PR_U3BD_POST_UP_02A3_VIEW_TABLE_OK);
    CHECK(report.column == 5u && report.slice == 14u && report.item == 19u);
    CHECK(report.item_physical == 0x3D8D9u && report.next_ip == 0x02C4u);
    CHECK(s.ax == 0x0888u && s.cx == 5u && s.dx == 0u &&
          s.es == 0x3D0Eu && s.di == 0x0895u && s.flags == 0x7202u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact; item 19, IP=0x02C4");
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
