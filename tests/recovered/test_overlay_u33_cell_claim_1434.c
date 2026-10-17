#include "poolrad/recovered/overlay_u33_cell_claim_1434.h"
#include "u33_place_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +1434 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* Enter with r, b8, b10, y, x, q pushed as words (high byte 0x5A) and a far return frame. */
static pr_u33_1434_cpu enter(uint8_t *ram, uint8_t r, uint8_t b8, uint8_t b10, uint8_t y, uint8_t x, uint8_t q)
{
    const uint8_t args[6] = {r, b8, b10, y, x, q};
    pr_u33_1434_cpu cpu = {0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
                           0x7777u, 0x8000u, PLACE_CS, 0x1434u, PLACE_DS, 0x9999u, PLACE_SS, 0x0202u};
    unsigned i;
    place_w16(ram, PLACE_SS, 0x8000u, 0x7000u);
    place_w16(ram, PLACE_SS, 0x8002u, 0x7001u);
    for (i = 0; i < 6u; ++i)
        place_w16(ram, PLACE_SS, (uint16_t)(0x8004u + 2u * i), (uint16_t)(0x5A00u | args[i]));
    return cpu;
}

static void returned(const pr_u33_1434_cpu *cpu)
{
    CHECK(cpu->ip == 0x7000u && cpu->cs == 0x7001u && cpu->sp == 0x8010u && cpu->bp == 0x7777u);
}

int main(void)
{
    uint8_t *ram = malloc(PLACE_RAM);
    pr_u33_1434_cpu cpu;
    int callee = 0;
    uint16_t index;
    CHECK(ram != NULL);

    /* x = 0Bh is outside 0..0Ah: AL = 0 with CMP x,0Ah's flags, no table write. */
    place_world(ram, 1u, 3u, 0u);
    cpu = enter(ram, 0u, 0u, 0u, 2u, 0x0Bu, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && (cpu.flags & 0x08D5u) == 0x0000u && callee == -1);
    CHECK(place_r8(ram, PLACE_DS, 0x5F2Bu) == 0xA5u);
    returned(&cpu);

    /* A closed availability byte: AL = 0 and the slot origin is untouched. */
    place_world(ram, 0u, 3u, 0u);
    cpu = enter(ram, 1u, 0u, 0u, 2u, 3u, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && place_r8(ram, PLACE_DS, 0x5F2Bu) == 0xA5u);
    returned(&cpu);

    /* Open, unoccupied, class 2 of weight 3: the origin is written, +0CB9 runs, the cell is claimed. */
    place_world(ram, 1u, 3u, 0u);
    cpu = enter(ram, 1u, 1u, 2u, 2u, 3u, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_OK && callee == 0);
    CHECK((cpu.ax & 0xFFu) == 1u);
    CHECK(place_r8(ram, PLACE_DS, 0x5F2Bu) == (uint8_t)(3u + 6u * 2u + 5u * 1u + 0x16u));
    CHECK(place_r8(ram, PLACE_DS, 0x5F2Cu) == (uint8_t)(2u + 5u * 1u + 0x0Au));
    index = (uint16_t)(0x42u * 1u + 0x0Bu * 2u + 3u);
    CHECK(place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + index)) == 0u);
    CHECK(place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + index + 1u)) == 1u);
    CHECK(place_r8(ram, PLACE_SS, 0x7FFBu) == 2u && place_r8(ram, PLACE_SS, 0x7FFCu) == 0u);  /* [BP-3] class, [BP-2] occupant */
    returned(&cpu);

    /* The same cell with an occupant: AL = 0 and the availability byte stays open. */
    place_world(ram, 1u, 3u, 5u);
    cpu = enter(ram, 1u, 1u, 2u, 2u, 3u, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + index)) == 1u);
    returned(&cpu);

    /* A class weight of FFh refuses the claim. */
    place_world(ram, 1u, 0xFFu, 0u);
    cpu = enter(ram, 1u, 1u, 2u, 2u, 3u, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + index)) == 1u);

    /* A far-call site that is not the loaded original stops with CODE before the transfer. */
    place_world(ram, 1u, 3u, 0u);
    place_w8(ram, PLACE_CS, 0x150Eu, 0xCDu);
    cpu = enter(ram, 1u, 1u, 2u, 2u, 3u, 1u);
    CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, &cpu, &callee) == PR_U33_1434_CODE);

    {
        pr_u33_1434_cpu before = enter(ram, 0u, 0u, 0u, 0u, 0u, 1u), after = before;
        CHECK(pr_u33_cell_claim_1434(NULL, PLACE_RAM, &after, NULL) == PR_U33_1434_ARGUMENT);
        CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM - 1u, &after, NULL) == PR_U33_1434_ARGUMENT);
        CHECK(pr_u33_cell_claim_1434(ram, PLACE_RAM, NULL, NULL) == PR_U33_1434_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +1434: range, closed, claim, occupied, heavy, site and guard cases passed\n");
    return 0;
}
