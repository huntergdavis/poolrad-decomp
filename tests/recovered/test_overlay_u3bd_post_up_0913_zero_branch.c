#include "poolrad/recovered/overlay_u3bd_post_up_0913_zero_branch.h"

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

static void init(uint8_t *ram, pr_u3bd_post_up_0913_zero_branch_state *s)
{
    memset(ram, 0xA5, 0x4000u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x4000u;
    s->cs = 0x2D14u;
    s->bp = 0x0120u;
    s->sp = 0x0100u;
    s->ip = 0x0913u;
    s->flags = 0x7256u;
}

static int synthetic(void)
{
    uint8_t ram[0x4000u];
    pr_u3bd_post_up_0913_zero_branch_state s;
    pr_u3bd_post_up_0913_zero_branch_report r;

    init(ram, &s);
    s.ax = 0u;
    CHECK(pr_u3bd_post_up_0913_zero_branch(&s, &r) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_OK);
    CHECK(r.result == 0u && r.next_ip == 0x0951u);
    CHECK(s.ip == 0x0951u && s.flags == 0x7246u && s.sp == 0x0100u);
    CHECK(ram[s.bp - 0x15u] == 0u);

    init(ram, &s);
    s.ax = 0x1280u;
    CHECK(pr_u3bd_post_up_0913_zero_branch(&s, &r) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_OK);
    CHECK(r.result == 0x80u && r.next_ip == 0x091Cu &&
          s.ax == 0x1280u && s.sp == 0x0100u);
    CHECK(s.flags == 0x7282u && ram[s.bp - 0x15u] == 0x80u);

    init(ram, &s);
    s.bp = 0x4015u; /* First reached write lies past host RAM. */
    s.ax = 0xAB03u;
    CHECK(pr_u3bd_post_up_0913_zero_branch(&s, NULL) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_BOUNDARY);
    CHECK(s.ip == 0x0913u && s.ax == 0xAB03u && s.flags == 0x7256u);

    init(ram, &s);
    s.ip = 0x0914u;
    CHECK(pr_u3bd_post_up_0913_zero_branch(&s, NULL) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_ARGUMENT);
    CHECK(s.ip == 0x0914u && s.flags == 0x7256u);
    CHECK(pr_u3bd_post_up_0913_zero_branch(NULL, NULL) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_ARGUMENT);
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
    pr_u3bd_post_up_0913_zero_branch_state s = {0};
    pr_u3bd_post_up_0913_zero_branch_report r;
    CHECK(pre != NULL && post != NULL);
    CHECK(load_ram(pre_path, pre) && load_ram(post_path, post));
    s.ram = pre;
    s.ram_size = RAM_BYTES;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x0913u;
    s.ax = 0u;
    s.flags = 0x7256u;
    CHECK(pr_u3bd_post_up_0913_zero_branch(&s, &r) ==
          PR_U3BD_POST_UP_0913_ZERO_BRANCH_OK);
    CHECK(r.result == 0u && r.next_ip == 0x0951u);
    CHECK(s.ip == 0x0951u && s.sp == 0x3EC8u && s.ax == 0u &&
          s.bp == 0x3EE0u && s.ss == 0x1D16u && s.flags == 0x7246u);
    CHECK(memcmp(pre, post, RAM_BYTES) == 0);
    puts("same-run RAM 655360/655360 exact at +0951");
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
