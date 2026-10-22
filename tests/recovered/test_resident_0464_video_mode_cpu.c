/* Focused synthetic test for RES_0464_00CF (video-mode set over Dos.Intr) and RES_0464_00FF (restore)
 * (docs/re/RES_0464_VIDEO_MODE_00CF_CPU.md). No game bytes: only the relocated segment word at CS:00F1. */
#include "poolrad/recovered/resident_0464_video_mode_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "0464 video line %d: %s\n", __LINE__, #x);            \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, BODY = 0x1464, DOS = 0x1A32, DS0 = 0x1C7C, SS0 = 0x7000, SP0 = 0xF000, STUB = 0x6000 };

static uint8_t *ram;
static unsigned checks, services, refuse;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void put16(uint16_t seg, uint16_t off, uint16_t v) { ram[at(seg, off)] = (uint8_t)v; ram[at(seg, (uint16_t)(off + 1u))] = (uint8_t)(v >> 8); }
static uint16_t get16(uint16_t seg, uint16_t off) { return (uint16_t)(ram[at(seg, off)] | (ram[at(seg, (uint16_t)(off + 1u))] << 8)); }

/* The declared INT 10h stub: [SP-2] := DS, [SP-4] := AX, 0040:0049 := AL, IRET. */
static int video(void *ctx, uint8_t *r, const pr_dos_intr_cpu *b, pr_dos_intr_cpu *a)
{
    (void)ctx; (void)r;
    ++services;
    if (refuse || b->cs != STUB || b->ip != 0)
        return 1;
    *a = *b;
    put16(b->ss, (uint16_t)(b->sp - 2u), b->ds);
    put16(b->ss, (uint16_t)(b->sp - 4u), b->ax);
    ram[at(0x0040, 0x0049)] = (uint8_t)b->ax;
    a->ip = get16(b->ss, b->sp); a->cs = get16(b->ss, (uint16_t)(b->sp + 2u)); a->flags = get16(b->ss, (uint16_t)(b->sp + 4u));
    a->sp = (uint16_t)(b->sp + 6u);
    return 0;
}

static pr_r0464_video_cpu world(uint16_t ip, uint8_t mode, uint8_t prev, uint8_t saved)
{
    pr_r0464_video_cpu c = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, BODY, ip, DS0, 0x1111u, SS0, 0x0002u};
    memset(ram, 0x5A, RAM);
    put16(BODY, 0x00F1u, DOS);
    put16(0, 0x40, 0); put16(0, 0x42, STUB);
    ram[at(DS0, 0x4A1Cu)] = saved; ram[at(DS0, 0x4A1Du)] = prev;
    put16(DS0, 0x4A12u, 0x2121u); put16(DS0, 0x4A14u, 0x3131u); put16(DS0, 0x4A16u, 0x0246u);   /* Regs.DS, ES, FLAGS */
    put16(SS0, SP0, 0x7000u); put16(SS0, SP0 + 2u, 0x9001u);
    if (ip == 0x00CFu)
        put16(SS0, SP0 + 4u, (uint16_t)(0xA500u | mode));
    return c;
}

int main(void)
{
    pr_r0464_video_env env = {video, NULL};
    pr_r0464_video_cpu c;
    int intr = 99;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* 00CF(0Dh) with previous mode 03h: saves 03h, sets Regs.AX = 000Dh, INT 10h sees AL 0Dh, records 0Dh; RETF 2. */
    c = world(0x00CFu, 0x0D, 0x03, 0x77);
    CHECK(pr_r0464_set_mode_00cf_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_OK && intr == 0 && services == 1);
    CHECK(ram[at(DS0, 0x4A1Cu)] == 0x03 && ram[at(DS0, 0x4A1Du)] == 0x0D && ram[at(DS0, 0x4A02u)] == 0x0D && ram[at(DS0, 0x4A03u)] == 0);
    CHECK(ram[at(0x0040, 0x0049)] == 0x0D && c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 6u && c.ds == DS0 && c.bp == 0x7777u);
    CHECK(get16(SS0, SP0 - 2u) == 0x7777u && get16(SS0, SP0 - 6u) == 0x1210u && get16(SS0, SP0 - 8u) == DS0 && get16(SS0, SP0 - 10u) == 0x4A02u);

    /* 00FF restores the saved mode 13h through 00CF, which saves the mode it replaces (0Dh); plain RETF. */
    c = world(0x00FFu, 0, 0x0D, 0x13);
    CHECK(pr_r0464_restore_mode_00ff_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_OK && intr == 0);
    CHECK(ram[at(DS0, 0x4A1Cu)] == 0x0D && ram[at(DS0, 0x4A1Du)] == 0x13 && ram[at(0x0040, 0x0049)] == 0x13);
    CHECK(c.cs == 0x9001u && c.ip == 0x7000u && c.sp == SP0 + 4u && c.bp == 0x7777u && get16(SS0, SP0 - 4u) == 0x1213u);

    /* A refused INT 10h stops with INTR (Intr's SERVICE); the saved-mode store has already happened. */
    refuse = 1;
    c = world(0x00CFu, 0x10, 0x03, 0x77);
    CHECK(pr_r0464_set_mode_00cf_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_INTR && intr == (int)PR_DOS_INTR_SERVICE);
    CHECK(ram[at(DS0, 0x4A1Cu)] == 0x03 && ram[at(DS0, 0x4A1Du)] == 0x03);
    refuse = 0;

    /* Guards change nothing: wrong IP, SP outside the guard, null service. */
    c = world(0x00CFu, 0x10, 0x03, 0x77);
    c.sp = 0x0020u;
    CHECK(pr_r0464_set_mode_00cf_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_ARGUMENT && ram[at(DS0, 0x4A1Cu)] == 0x77);
    c = world(0x00FFu, 0, 0x03, 0x77);
    c.ip = 0x00CFu;
    CHECK(pr_r0464_restore_mode_00ff_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_ARGUMENT);
    env.video = NULL;
    CHECK(pr_r0464_set_mode_00cf_cpu(ram, RAM, &c, &env, &intr) == PR_R0464_VIDEO_ARGUMENT);

    free(ram);
    printf("0464 video mode: %u checks passed\n", checks);
    return 0;
}
