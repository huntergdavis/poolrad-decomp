#include "poolrad/recovered/overlay_u3bd_post_up_0dae_outer_return.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

static void put_word(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void init(uint8_t *ram, pr_u3bd_post_up_0dae_state *s)
{
    memset(ram, 0, 0x400u);
    memset(s, 0, sizeof *s);
    s->ram = ram;
    s->ram_size = 0x400u;
    s->cs = 0x2D14u;
    s->ss = 0;
    s->bp = 0x0200u;
    s->sp = 0x0100u;
    s->ip = 0x0DAEu;
    s->ax = 0x0100u;
    s->di = 0x4EC0u;
    s->flags = 0x7246u;
    put_word(ram + 0x0200u, 0x0300u);
    put_word(ram + 0x0202u, 0x022Eu);
    put_word(ram + 0x0204u, 0x2E35u);
    put_word(ram + 0x0206u, 0x0006u);
    put_word(ram + 0x0208u, 0x0004u);
    put_word(ram + 0x020Au, 0x000Eu);
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *fp = fopen(path, "rb");
    size_t count;
    int extra;
    if (fp == NULL) return 0;
    count = fread(ram, 1u, 0xA0000u, fp);
    extra = fgetc(fp);
    if (fclose(fp) != 0) return 0;
    return count == 0xA0000u && extra == EOF;
}

static int private_pre_state(const char *path)
{
    uint8_t *ram = malloc(0xA0000u);
    uint8_t *before = malloc(0xA0000u);
    pr_u3bd_post_up_0dae_state s = {0};
    CHECK(ram != NULL && before != NULL);
    CHECK(load_ram(path, ram));
    memcpy(before, ram, 0xA0000u);
    s.ram = ram;
    s.ram_size = 0xA0000u;
    s.cs = 0x2D14u;
    s.ss = 0x1D16u;
    s.bp = 0x3EE0u;
    s.sp = 0x3EC8u;
    s.ip = 0x0DAEu;
    s.ax = 0x0100u;
    s.di = 0x4EC0u;
    s.flags = 0x7246u;
    CHECK(pr_u3bd_post_up_0dae_outer_return(&s) ==
          PR_U3BD_POST_UP_0DAE_OK);
    CHECK(s.cs == 0x2E35u && s.ip == 0x022Eu && s.bp == 0x3EF0u &&
          s.sp == 0x3EECu && s.ax == 0x0100u && s.di == 0x4EC0u &&
          s.flags == 0x7246u && memcmp(ram, before, 0xA0000u) == 0);
    puts("private pre-state C prediction: 2E35:022E, BP=3EF0, SP=3EEC; native successor pending");
    free(before);
    free(ram);
    return 0;
}

int main(int argc, char **argv)
{
    uint8_t ram[0x400u];
    uint8_t before[0x400u];
    pr_u3bd_post_up_0dae_state s;

    init(ram, &s);
    memcpy(before, ram, sizeof before);
    CHECK(pr_u3bd_post_up_0dae_outer_return(&s) ==
          PR_U3BD_POST_UP_0DAE_OK);
    CHECK(s.cs == 0x2E35u && s.ip == 0x022Eu && s.bp == 0x0300u &&
          s.sp == 0x020Cu && s.ax == 0x0100u && s.di == 0x4EC0u &&
          s.flags == 0x7246u && memcmp(ram, before, sizeof ram) == 0);

    init(ram, &s);
    s.ram_size = 0x0200u;
    CHECK(pr_u3bd_post_up_0dae_outer_return(&s) ==
          PR_U3BD_POST_UP_0DAE_BOUNDARY);
    CHECK(s.cs == 0x2D14u && s.ip == 0x0DB0u && s.bp == 0x0200u &&
          s.sp == 0x0200u);

    init(ram, &s);
    s.ram_size = 0x0204u;
    CHECK(pr_u3bd_post_up_0dae_outer_return(&s) ==
          PR_U3BD_POST_UP_0DAE_BOUNDARY);
    CHECK(s.cs == 0x2D14u && s.ip == 0x0DB1u && s.bp == 0x0300u &&
          s.sp == 0x0202u);

    init(ram, &s);
    s.ip = 0x0DB0u;
    CHECK(pr_u3bd_post_up_0dae_outer_return(&s) ==
          PR_U3BD_POST_UP_0DAE_ARGUMENT);
    CHECK(pr_u3bd_post_up_0dae_outer_return(NULL) ==
          PR_U3BD_POST_UP_0DAE_ARGUMENT);
    if (argc == 1) return 0;
    if (argc != 2) return 2;
    return private_pre_state(argv[1]);
}
