#include "poolrad/recovered/overlay_u3bd_post_up_05fb_wrapper_return.h"
#include "poolrad/recovered/overlay_u3bd_post_up_0dae_outer_return.h"

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

static int compose(pr_u3bd_post_up_05fb_state *wrapper,
                   pr_u3bd_post_up_0dae_state *outer)
{
    if (pr_u3bd_post_up_05fb_wrapper_return(wrapper) !=
        PR_U3BD_POST_UP_05FB_OK) return 0;

    outer->ram = wrapper->ram;
    outer->ram_size = wrapper->ram_size;
    outer->cs = wrapper->cs;
    outer->ss = wrapper->ss;
    outer->bp = wrapper->bp;
    outer->sp = wrapper->sp;
    outer->ip = wrapper->ip;
    outer->ax = wrapper->ax;
    outer->di = wrapper->di;
    outer->flags = wrapper->flags;
    return pr_u3bd_post_up_0dae_outer_return(outer) ==
           PR_U3BD_POST_UP_0DAE_OK;
}

static int synthetic(void)
{
    uint8_t ram[0x400u] = {0};
    uint8_t before[0x400u];
    pr_u3bd_post_up_05fb_state wrapper = {0};
    pr_u3bd_post_up_0dae_state outer = {0};

    wrapper.ram = ram;
    wrapper.ram_size = sizeof ram;
    wrapper.cs = 0x1111u;
    wrapper.ss = 0;
    wrapper.bp = 0x0100u;
    wrapper.sp = 0x0100u;
    wrapper.ip = 0x05FBu;
    wrapper.ax = 0x0100u;
    wrapper.di = 0x4EC0u;
    wrapper.flags = 0x7246u;
    put_word(ram + 0x0100u, 0x0200u); /* Wrapper's saved BP. */
    put_word(ram + 0x0102u, 0x0DAEu); /* Wrapper's far return. */
    put_word(ram + 0x0104u, 0x1111u);
    put_word(ram + 0x0200u, 0x0300u); /* Outer frame's saved BP. */
    put_word(ram + 0x0202u, 0x2222u); /* Outer far return. */
    put_word(ram + 0x0204u, 0x3333u);
    put_word(ram + 0x0206u, 0x0006u);
    put_word(ram + 0x0208u, 0x0004u);
    put_word(ram + 0x020Au, 0x000Eu);
    memcpy(before, ram, sizeof before);

    CHECK(compose(&wrapper, &outer));
    CHECK(wrapper.cs == 0x1111u && wrapper.ip == 0x0DAEu &&
          wrapper.bp == 0x0200u && wrapper.sp == 0x0106u);
    CHECK(outer.ram == wrapper.ram && outer.ram_size == wrapper.ram_size &&
          outer.ss == wrapper.ss && outer.cs == 0x3333u &&
          outer.ip == 0x2222u && outer.bp == 0x0300u &&
          outer.sp == 0x020Cu && outer.ax == wrapper.ax &&
          outer.di == wrapper.di && outer.flags == wrapper.flags);
    CHECK(memcmp(ram, before, sizeof ram) == 0);
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

static int private_pre_state(const char *path)
{
    uint8_t *ram = malloc(RAM_BYTES);
    uint8_t *before = malloc(RAM_BYTES);
    pr_u3bd_post_up_05fb_state wrapper = {0};
    pr_u3bd_post_up_0dae_state outer = {0};
    CHECK(ram != NULL && before != NULL);
    CHECK(load_ram(path, ram));
    memcpy(before, ram, RAM_BYTES);
    wrapper.ram = ram;
    wrapper.ram_size = RAM_BYTES;
    wrapper.cs = 0x2D14u;
    wrapper.ss = 0x1D16u;
    wrapper.bp = 0x3EC2u;
    wrapper.sp = 0x3EC2u;
    wrapper.ip = 0x05FBu;
    wrapper.ax = 0x0100u;
    wrapper.di = 0x4EC0u;
    wrapper.flags = 0x7246u;

    CHECK(compose(&wrapper, &outer));
    /* The first checkpoint is measured native; the second is C-predicted. */
    CHECK(wrapper.cs == 0x2D14u && wrapper.ip == 0x0DAEu &&
          wrapper.bp == 0x3EE0u && wrapper.sp == 0x3EC8u);
    CHECK(outer.ram == ram && outer.ram_size == RAM_BYTES &&
          outer.ss == 0x1D16u && outer.cs == 0x2E35u &&
          outer.ip == 0x022Eu && outer.bp == 0x3EF0u &&
          outer.sp == 0x3EECu && outer.ax == 0x0100u &&
          outer.di == 0x4EC0u && outer.flags == 0x7246u);
    CHECK(memcmp(ram, before, RAM_BYTES) == 0);
    puts("C continuity: measured +0DAE intermediate; predicted 2E35:022E; RAM 655360/655360 unchanged");
    free(before);
    free(ram);
    return 0;
}

int main(int argc, char **argv)
{
    if (synthetic() != 0) return 1;
    if (argc == 1) return 0;
    if (argc != 2) return 2;
    return private_pre_state(argv[1]);
}
