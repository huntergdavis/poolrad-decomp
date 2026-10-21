/* Focused synthetic test for the raw-CPU GAME.OVR unit 7Fh +0294 numeric entry editor (docs/re/U7F_NUMBER_0294_CPU.md).
 * No game bytes: the far-call segment words and the arguments are placed; the 0709 routines' code is absent, so the first
 * screen call stops with the space model's own CODE refusal, which is what this test pins (with the frame and StrStore). */
#include "poolrad/recovered/overlay_u7f_number_0294_cpu.h"
#include "poolrad/recovered/ega_hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "u7f 0294 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & 1048575u; }
static void put(uint8_t *m, unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & 1048575u] = (uint8_t)(v >> 8); }
static unsigned get(const uint8_t *m, unsigned s, unsigned o) { size_t a = p(s, o); return m[a] | (unsigned)m[(a + 1u) & 1048575u] << 8; }

enum { OVL = 0x8000, SYS = 0x1AF8, RES = 0x1709, KEY = 0x1899, DS = 0x1C7C, SS = 0x7000, PROMPT = 0x7800 };
static const uint16_t sites[17][2] = {{0x02AC, SYS}, {0x02BD, RES}, {0x02D1, RES}, {0x02F5, SYS}, {0x02FE, KEY}, {0x0322, SYS}, {0x0331, SYS},
                                      {0x0336, SYS}, {0x0344, SYS}, {0x0353, SYS}, {0x0382, SYS}, {0x03A7, RES}, {0x03CE, SYS}, {0x03DC, SYS},
                                      {0x03EE, RES}, {0x0411, RES}, {0x042D, SYS}};
static pr_u7f_cpu setup(uint8_t *ram, const char *prompt, unsigned maxv)
{
    pr_u7f_cpu c = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0xF000, OVL, 0x0294, DS, 0x1111, SS, 0x0002};
    size_t n = strlen(prompt); unsigned i;
    memset(ram, 0x5A, PR_U7F_NUMBER_RAM_SIZE);
    for (i = 0; i < 17u; ++i) put(ram, OVL, sites[i][0], sites[i][1]);       /* the relocated unit fixups */
    ram[p(PROMPT, 0x100)] = (uint8_t)n; memcpy(ram + p(PROMPT, 0x101), prompt, n);
    put(ram, SS, 0xF000, 0x7000); put(ram, SS, 0xF002, 0x9001);              /* far return */
    put(ram, SS, 0xF004, 0x000E); put(ram, SS, 0xF006, 0x0100); put(ram, SS, 0xF008, PROMPT); put(ram, SS, 0xF00A, maxv);
    return c;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U7F_NUMBER_RAM_SIZE);
    static pr_ega ega;
    pr_bus bus;
    pr_u7f_number_env env;
    pr_u7f_number_diagnostic d;
    pr_u7f_cpu c, before;
    char long_prompt[46];
    CHECK(ram != NULL);
    memset(&env, 0, sizeof env); env.space_loopbacks = env.printer_loopbacks = env.glyph_loopbacks = 1000u; env.key_limit = 8u;
    pr_ega_reset(&ega);
    bus.ram = ram; bus.ega = &ega; bus.ports = NULL; bus.on_vram_write = NULL; bus.ctx = NULL;

    /* Prologue: BP = F000h - 2 = EFFEh; SUB SP,244h. StrStore copies the prompt to [BP-2Bh]. The space routine (+02BA) finds
     * no code at 0709:061F and refuses: CALLEE at site 02BAh with its CODE status, before any key is read. */
    c = setup(ram, "How much? ", 100);
    CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_CALLEE);
    CHECK(d.site == 0x02BA && d.status == PR_R0709_SPACE_CPU_CODE && d.keys == 0);
    CHECK(ram[p(SS, 0xEFFE - 0x2B)] == 10 && memcmp(ram + p(SS, 0xEFFE - 0x2B + 1), "How much? ", 10) == 0);
    CHECK(get(ram, SS, 0xEFFE) == 0x7777);                                   /* the saved BP */
    /* A 45-character prompt is cut to 40 by StrStore (max 28h). */
    memset(long_prompt, 'X', 45); long_prompt[45] = 0;
    c = setup(ram, long_prompt, 100);
    CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_CALLEE);
    CHECK(ram[p(SS, 0xEFFE - 0x2B)] == 40 && ram[p(SS, 0xEFFE - 0x2B + 41)] == 0x5A);

    /* ARGUMENT: rejected before any change (no EGA, wrong size, SP outside [025Ah,FFF4h], wrong IP). */
    c = setup(ram, "Q", 7); before = c;
    bus.ega = NULL; CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_ARGUMENT); bus.ega = &ega;
    CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE - 1u, &c, &env, &d) == PR_U7F_NUMBER_ARGUMENT);
    c.sp = 0x0259; CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_ARGUMENT);
    c.sp = 0xFFF5; CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_ARGUMENT);
    c.sp = 0xF000; c.ip = 0x0295; CHECK(pr_u7f_number_0294_cpu(&bus, PR_U7F_NUMBER_RAM_SIZE, &c, &env, &d) == PR_U7F_NUMBER_ARGUMENT);
    c.ip = 0x0294; CHECK(memcmp(&c, &before, sizeof c) == 0 && get(ram, SS, 0xEFFE) == 0x5A5A);

    free(ram);
    printf("u7f number 0294: %u semantic checks passed\n", checks);
    return 0;
}
