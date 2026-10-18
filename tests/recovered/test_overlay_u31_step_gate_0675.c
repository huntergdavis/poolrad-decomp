/* Focused synthetic test for GAME.OVR unit 0x31:+0675 (docs/re/U31_STEP_GATE_0675.md), composing the
 * U3C6 +0CB9 and raw-CPU RES_00BA_2199 models. No game bytes: only the far-call sites, the stub and
 * small DS tables (tests/recovered/u33_place_fixture.h). */
#include "poolrad/recovered/overlay_u31_step_gate_0675.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "u33_place_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x)                                                                    \
    do {                                                                            \
        if (!(x)) {                                                                 \
            fprintf(stderr, "u31 +0675 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                                \
        }                                                                           \
        ++checks;                                                                   \
    } while (0)

enum { X_OFF = 0x1101, X_SEG = 0x2222, Y_OFF = 0x0200, Y_SEG = 0x6000, R_OFF = 0x0100, R_SEG = 0x6800, BP0 = 0x7FFE };

static unsigned checks;

/* +0675's call sites: LCALL 03E0:0070 at +06DA and LCALL 00BA:2199 at +0758/+076F/+0786/+079D. */
static void link_sites(uint8_t *ram)
{
    static const uint16_t sites[4] = {0x0758u, 0x076Fu, 0x0786u, 0x079Du};
    unsigned n;
    place_w8(ram, PLACE_CS, 0x06DAu, 0x9Au);
    place_w16(ram, PLACE_CS, 0x06DBu, 0x0070u);
    place_w16(ram, PLACE_CS, 0x06DDu, 0x03E0u);
    for (n = 0; n < 4u; ++n) {
        place_w8(ram, PLACE_CS, sites[n], 0x9Au);
        place_w16(ram, PLACE_CS, (uint16_t)(sites[n] + 1u), 0x2199u);
        place_w16(ram, PLACE_CS, (uint16_t)(sites[n] + 3u), 0x00BAu);
    }
}

/* place_world with every probe of slot 1 (X) at class `cls`, weight `weight` for that class, occupant `occupant`;
 * slot 1's origin at (10, 10); Y = X->+108 with Y[6] = y6 and Y[15h] = 1; t = DS:[02AE + 5 + c] = 4 for c = 2. */
static pr_u31_0675_cpu world(uint8_t *ram, uint8_t cls, uint8_t weight, uint8_t occupant, uint8_t y6, uint8_t b)
{
    pr_u31_0675_cpu cpu = {0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
                           0x7777u, 0x8000u, PLACE_CS, 0x0675u, PLACE_DS, 0x9999u, PLACE_SS, 0x0202u};
    unsigned i;
    place_world(ram, 1u, weight, occupant);
    link_sites(ram);
    for (i = 0; i < 50u * 25u; ++i)
        place_w8(ram, PLACE_GRID_SEG, (uint16_t)(7u + i), cls);
    place_w8(ram, PLACE_DS, (uint16_t)(0x2744u + 4u * cls), weight);
    place_w8(ram, PLACE_DS, 0x5F2Bu, 10u);
    place_w8(ram, PLACE_DS, 0x5F2Cu, 10u);
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x108u), Y_OFF);
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x10Au), Y_SEG);
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x7Fu), 0u);
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x81u), 0u);
    place_w8(ram, Y_SEG, Y_OFF + 6u, y6);
    place_w8(ram, Y_SEG, Y_OFF + 0x15u, 1u);
    place_w8(ram, PLACE_DS, 0x02B5u, 4u);
    place_w8(ram, R_SEG, R_OFF, 0xEEu);
    place_w16(ram, PLACE_SS, 0x8000u, 0x7000u);
    place_w16(ram, PLACE_SS, 0x8002u, 0x7001u);
    place_w16(ram, PLACE_SS, 0x8004u, R_OFF);
    place_w16(ram, PLACE_SS, 0x8006u, R_SEG);
    place_w16(ram, PLACE_SS, 0x8008u, (uint16_t)(0x5A00u | b));
    place_w16(ram, PLACE_SS, 0x800Au, 0x5A02u);
    place_w16(ram, PLACE_SS, 0x800Cu, X_OFF);
    place_w16(ram, PLACE_SS, 0x800Eu, X_SEG);
    return cpu;
}

/* X's sidecar list: nodes at 7000:0040 + 10h * i with the given keys. */
static void sidecars(uint8_t *ram, const uint8_t *keys, unsigned n, int cycle)
{
    unsigned i;
    for (i = 0; i < n; ++i) {
        uint16_t off = (uint16_t)(0x0040u + 0x10u * i);
        place_w8(ram, 0x7000u, off, keys[i]);
        place_w16(ram, 0x7000u, (uint16_t)(off + 5u), i + 1u < n ? (uint16_t)(off + 0x10u) : (cycle ? 0x0040u : 0u));
        place_w16(ram, 0x7000u, (uint16_t)(off + 7u), i + 1u < n || cycle ? 0x7000u : 0u);
    }
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x7Fu), n ? 0x0040u : 0u);
    place_w16(ram, X_SEG, (uint16_t)(X_OFF + 0x81u), n ? 0x7000u : 0u);
}

static uint8_t local(const uint8_t *ram, int delta)
{
    return place_r8(ram, PLACE_SS, (uint16_t)(BP0 + delta));
}

static void returned(const pr_u31_0675_cpu *cpu)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == 0x8010u && cpu->bp == 0x7777u);
    CHECK(cpu->ds == PLACE_DS && cpu->ss == PLACE_SS);
}

int main(void)
{
    uint8_t *ram = malloc(PLACE_RAM);
    pr_u31_0675_cpu cpu;
    pr_u31_0675_callee callee;
    static const uint8_t k6f[3] = {0x11u, 0x6Fu, 0x22u}, k7d[1] = {0x7Du}, kmiss[2] = {0x55u, 0x66u};
    CHECK(ram != NULL);

    /* Class 2, weight 3, open: r = (2 + 4) mod 8 = 6 (even), threshold 3 * 2 = 6 <= Y[6] = 10: AL = 1, *R = 0. */
    cpu = world(ram, 2u, 3u, 0u, 10u, 2u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 1u && place_r8(ram, R_SEG, R_OFF) == 0u && callee.site == 0x06DAu && callee.status == 0);
    CHECK(local(ram, -6) == 4u && local(ram, -7) == 6u && local(ram, -9) == 2u && local(ram, -8) == 0u);
    CHECK(local(ram, -0x0A) == 6u && local(ram, -1) == 1u);
    CHECK((cpu.flags & 0x08D5u) == 0x0000u);                  /* CMP 10,6 = 4: odd parity, no arithmetic flag */
    returned(&cpu);

    /* r = 7 (odd): threshold 3 * 3 = 9. Y[6] = 9 passes; Y[6] = 8 fails with CMP 8,9's flags. */
    cpu = world(ram, 2u, 3u, 0u, 9u, 3u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 1u && local(ram, -7) == 7u && local(ram, -0x0A) == 9u);
    returned(&cpu);
    cpu = world(ram, 2u, 3u, 0u, 8u, 3u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && local(ram, -1) == 0u && (cpu.flags & 0x0001u) && (cpu.flags & 0x0080u));
    returned(&cpu);

    /* Weight 10 is not below Y[6] = 10: leave before the threshold; [BP-0Ah] keeps its stack byte. */
    cpu = world(ram, 2u, 10u, 0u, 10u, 2u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && local(ram, -0x0A) == 0xA5u && (cpu.flags & 0x0040u));
    returned(&cpu);

    /* An occupant (P10 = 5): leave with AL = 0 and CMP 5,0's flags. */
    cpu = world(ram, 2u, 3u, 5u, 10u, 2u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && local(ram, -8) == 5u && place_r8(ram, R_SEG, R_OFF) == 0u);
    CHECK((cpu.flags & 0x08D5u) == 0x0004u);
    returned(&cpu);

    /* Class 0 (P6 = 0): *R := 1 and AL = 0 with CMP 0,0's flags. */
    cpu = world(ram, 0u, 3u, 0u, 10u, 2u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && local(ram, -9) == 0u && place_r8(ram, R_SEG, R_OFF) == 1u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);
    returned(&cpu);

    /* Class 1Eh with key 6Fh third in X's list: lookups 20h and 1Eh miss, 6Fh hits at +0786; the threshold stays
     * 3 * 2 = 6 and the cursor cell SS:BP-5 holds the hit node 7000:0050. */
    cpu = world(ram, 0x1Eu, 3u, 0u, 10u, 2u);
    sidecars(ram, k6f, 3u, 0);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK(callee.site == 0x0786u && callee.status == PR_RES_2199_OK);
    CHECK((cpu.ax & 0xFFu) == 1u && local(ram, -0x0A) == 6u);
    CHECK(place_r8(ram, PLACE_SS, BP0 - 5u) == 0x50u && place_r8(ram, PLACE_SS, BP0 - 4u) == 0x00u);
    CHECK(place_r8(ram, PLACE_SS, BP0 - 3u) == 0x00u && place_r8(ram, PLACE_SS, BP0 - 2u) == 0x70u);
    returned(&cpu);

    /* Key 7Dh alone: the fourth lookup (+079D) hits. */
    cpu = world(ram, 0x1Eu, 3u, 0u, 10u, 2u);
    sidecars(ram, k7d, 1u, 0);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK(callee.site == 0x079Du && (cpu.ax & 0xFFu) == 1u && local(ram, -0x0A) == 6u);
    returned(&cpu);

    /* No key: threshold := Y[6] + 1 = 11 > 10, so AL = 0. With Y[6] = FFh the threshold wraps to 0 and AL = 1. */
    cpu = world(ram, 0x1Eu, 3u, 0u, 10u, 2u);
    sidecars(ram, kmiss, 2u, 0);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK(callee.site == 0x079Du && (cpu.ax & 0xFFu) == 0u && local(ram, -0x0A) == 11u);
    returned(&cpu);
    cpu = world(ram, 0x1Eu, 3u, 0u, 0xFFu, 2u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_OK);
    CHECK((cpu.ax & 0xFFu) == 1u && local(ram, -0x0A) == 0u);
    returned(&cpu);

    /* A cyclic sidecar list without any key: the first lookup's model stops at its cap (CALLEE, LIMIT at 21B9). */
    cpu = world(ram, 0x1Eu, 3u, 0u, 10u, 2u);
    sidecars(ram, kmiss, 2u, 1);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 40u, &callee) == PR_U31_0675_CALLEE);
    CHECK(callee.site == 0x0758u && callee.status == PR_RES_2199_LIMIT && cpu.ip == 0x21B9u && cpu.cs == 0x00BAu);

    /* A far-call site that is not the loaded original stops with CODE before that call: at +06DA nothing was
     * pushed for +0CB9 yet; at +0758 (after +0CB9 ran) the stop keeps every earlier effect. */
    cpu = world(ram, 2u, 3u, 0u, 10u, 2u);
    place_w8(ram, PLACE_CS, 0x06DAu, 0xCDu);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_CODE);
    CHECK(callee.site == 0u && callee.status == -1 && cpu.sp == BP0 - 0x0Au - 0x0Eu && local(ram, -7) == 6u);
    cpu = world(ram, 0x1Eu, 3u, 0u, 10u, 2u);
    place_w16(ram, PLACE_CS, 0x0759u, 0x2198u);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_CODE);
    CHECK(callee.site == 0x06DAu && local(ram, -9) == 0x1Eu && cpu.sp == BP0 - 0x0Au - 0x0Au);

    /* Argument rejections. */
    cpu = world(ram, 2u, 3u, 0u, 10u, 2u);
    CHECK(pr_u31_step_gate_0675(NULL, PLACE_RAM, &cpu, 254u, &callee) == PR_U31_0675_ARGUMENT && callee.status == -1);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM - 1u, &cpu, 254u, NULL) == PR_U31_0675_ARGUMENT);
    CHECK(pr_u31_step_gate_0675(ram, PLACE_RAM, NULL, 254u, NULL) == PR_U31_0675_ARGUMENT);
    CHECK(cpu.sp == 0x8000u && cpu.ip == 0x0675u);

    free(ram);
    printf("PASS: u31 +0675, %u checks (factors, weight/occupant/class-0 exits, 1Eh lookups, wrap, LIMIT, CODE, "
           "arguments)\n", checks);
    return 0;
}
