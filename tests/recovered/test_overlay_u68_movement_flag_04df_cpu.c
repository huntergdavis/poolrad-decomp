/* Focused synthetic test for raw-CPU U68 +04DF (docs/re/U68_MOVEMENT_FLAG_04DF_CPU.md), composing the dice chain through the
 * shared synthetic fixture and the raw +0109. No game bytes; rolls predicted with the fixture's reference generator. */
#include "poolrad/recovered/overlay_u68_movement_flag_04df_cpu.h"
#include "dice_chain_fixture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u68 04df line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS0 = 0x3000, DS0 = 0x1000, SS0 = 0x2000, SP0 = 0x8000, MAP = 0x6000 };

static uint8_t *ram;
static unsigned checks;

/* Members at 5000h + 200h k with skill +78h and busy +10Ch; (x, y, d) = (3, 4, 2); table1[2] = 1, table2[2] = 0, so the
 * neighbour is (4, 4, 6); map cells 6000:0443 = 00h and 6000:0444 = FFh. */
static pr_u68_04df_cpu frame(const uint8_t *skill, const uint8_t *busy, unsigned count, int cyclic, uint32_t seed)
{
    pr_u68_04df_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, CS0, 0x04DFu, DS0, 0x1111u, SS0, 0x7202u};
    unsigned k;
    memset(ram, 0xA5, DICE_RAM);
    dice_site(ram, CS0, 0x0513u, 0x48u, 0x0DE3u);
    dice_system(ram, DS0, seed);
    dice_put16(ram, DS0, 0x5D96u, 0u);
    dice_put16(ram, DS0, 0x5D98u, count ? 0x5000u : 0u);
    for (k = 0; k < count; ++k) {
        uint16_t seg = (uint16_t)(0x5000u + 0x200u * k);
        uint16_t next = k + 1u < count ? (uint16_t)(seg + 0x200u) : (cyclic ? 0x5000u : 0u);
        ram[dice_physical(seg, 0x78u)] = skill[k];
        ram[dice_physical(seg, 0x10Cu)] = busy[k];
        dice_put16(ram, seg, 0x104u, 0u);
        dice_put16(ram, seg, 0x106u, next);
    }
    ram[dice_physical(DS0, 0x6AADu)] = 3; ram[dice_physical(DS0, 0x6AAEu)] = 4; ram[dice_physical(DS0, 0x6AAFu)] = 2;
    ram[dice_physical(DS0, 0x2738u)] = 1; ram[dice_physical(DS0, 0x2741u)] = 0;
    dice_put16(ram, DS0, 0x6A5Cu, 0x0100u); dice_put16(ram, DS0, 0x6A5Eu, MAP);
    ram[dice_physical(MAP, 0x0443u)] = 0x00; ram[dice_physical(MAP, 0x0444u)] = 0xFF;
    dice_put16(ram, SS0, SP0, 0x7000u); dice_put16(ram, SS0, SP0 + 2u, 0x7001u);
    return cpu;
}

static void returned(const pr_u68_04df_cpu *cpu)
{
    CHECK(cpu->cs == 0x7001u && cpu->ip == 0x7000u && cpu->sp == SP0 + 4u && cpu->bp == 0x7777u && cpu->ds == DS0);
}

int main(void)
{
    static const uint8_t sure[1] = {0xFF}, idle[1] = {0}, two_skill[2] = {0, 0xFF}, two_busy[2] = {0, 1};
    pr_u68_04df_cpu cpu;
    uint32_t seed;
    uint8_t roll, skill[1];
    int chain = -1;
    ram = malloc(DICE_RAM);
    if (ram == NULL)
        return 1;

    /* Skill FFh succeeds on any d100 roll: both marks, AL = 1, [6DDFh] = 0, one roll consumed. */
    seed = 0x12345678u;
    cpu = frame(sure, idle, 1, 0, seed);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_OK && chain == 0);
    (void)dice_roll(&seed, 1u, 100u);
    CHECK(dice_seed(ram, DS0) == seed && (cpu.ax & 0xFFu) == 1 && ram[dice_physical(DS0, 0x6DDFu)] == 0);
    CHECK(ram[dice_physical(MAP, 0x0443u)] == 0x04 && ram[dice_physical(MAP, 0x0444u)] == 0x7F);
    returned(&cpu);

    /* The JA boundary: a skill equal to the predicted roll succeeds, one below it fails. */
    seed = 0x0BADF00Du;
    roll = dice_roll(&seed, 1u, 100u);
    skill[0] = roll;
    cpu = frame(skill, idle, 1, 0, 0x0BADF00Du);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_OK && (cpu.ax & 0xFFu) == 1);
    skill[0] = (uint8_t)(roll - 1u);
    cpu = frame(skill, idle, 1, 0, 0x0BADF00Du);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_OK && (cpu.ax & 0xFFu) == 0);
    CHECK(ram[dice_physical(MAP, 0x0443u)] == 0x00 && ram[dice_physical(MAP, 0x0444u)] == 0xFF);

    /* Skill 0 fails, and a busy member fails even with skill FFh: two rolls, no marks, AL = 0. */
    seed = 0x00C0FFEEu;
    cpu = frame(two_skill, two_busy, 2, 0, seed);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_OK && (cpu.ax & 0xFFu) == 0);
    (void)dice_roll(&seed, 1u, 100u);
    (void)dice_roll(&seed, 1u, 100u);
    CHECK(dice_seed(ram, DS0) == seed && ram[dice_physical(MAP, 0x0443u)] == 0x00);
    returned(&cpu);

    /* No party: no roll; CMP BYTE [BP-6],0 leaves ZF and PF over 7202h. */
    cpu = frame(sure, idle, 0, 0, 0x12345678u);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_OK);
    CHECK(dice_seed(ram, DS0) == 0x12345678u && (cpu.ax & 0xFFu) == 0 && cpu.flags == 0x7246);

    /* A member linked to itself with cap 0: one roll, then LIMIT at +04F6. */
    seed = 0x12345678u;
    cpu = frame(idle, idle, 1, 1, seed);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 0, &chain) == PR_U68_04DF_LIMIT && cpu.ip == 0x04F6u);
    (void)dice_roll(&seed, 1u, 100u);
    CHECK(dice_seed(ram, DS0) == seed);

    /* CODE at +0513: the two words (AH from the member test's 5000h) pushed, no return address. */
    cpu = frame(sure, idle, 1, 0, 0x12345678u);
    ram[dice_physical(CS0, 0x0514u)] = 0x49;
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_CODE && cpu.ip == 0x0513u);
    CHECK(cpu.sp == SP0 - 0x10u && ram[dice_physical(SS0, cpu.sp)] == 0x64 && ram[dice_physical(SS0, (uint16_t)(cpu.sp + 1u))] == 0x50);
    CHECK(ram[dice_physical(SS0, (uint16_t)(cpu.sp + 2u))] == 0x01 && ram[dice_physical(SS0, (uint16_t)(cpu.sp + 3u))] == 0x50);

    /* Guards. */
    cpu = frame(sure, idle, 1, 0, 1u);
    cpu.ip = 0x04E0u;
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM, &cpu, 64, &chain) == PR_U68_04DF_ARGUMENT && chain == -1 && cpu.sp == SP0);
    CHECK(pr_u68_movement_flag_04df_cpu(NULL, DICE_RAM, &cpu, 64, NULL) == PR_U68_04DF_ARGUMENT);
    CHECK(pr_u68_movement_flag_04df_cpu(ram, DICE_RAM - 1u, &cpu, 64, NULL) == PR_U68_04DF_ARGUMENT);

    free(ram);
    printf("u68 +04df: %u checks passed\n", checks);
    return 0;
}
