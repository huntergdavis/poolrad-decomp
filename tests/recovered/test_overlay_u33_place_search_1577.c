#include "poolrad/recovered/overlay_u33_place_search_1577.h"
#include "u33_place_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "u33 +1577 line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* Search tables: cardinal quadrants, ring bases (bx, by), first-ring limit 2, mode 2 (no +01AD probes). */
static void search_tables(uint8_t *ram, uint8_t bx, uint8_t by, const uint8_t quadrants[4])
{
    unsigned i;
    for (i = 0; i < 4u; ++i) place_w8(ram, PLACE_DS, (uint16_t)(0x02F2u + i), quadrants[i]);
    for (i = 0; i < 8u; ++i) {
        place_w8(ram, PLACE_DS, (uint16_t)(0x02F6u + i), bx);
        place_w8(ram, PLACE_DS, (uint16_t)(0x02FEu + i), by);
    }
    for (i = 0; i < 0x10u; ++i) place_w8(ram, PLACE_DS, (uint16_t)(0x02E2u + i), 0u);
    place_w8(ram, PLACE_DS, 0x4650u, 1u); place_w8(ram, PLACE_DS, 0x4652u, 1u);   /* origin (1, 1) */
    place_w8(ram, PLACE_DS, 0x4654u, 2u); place_w8(ram, PLACE_DS, 0x4656u, 0u);
    place_w8(ram, PLACE_DS, 0x4658u, 1u);                                          /* map 1: no probe block */
    place_w8(ram, PLACE_DS, 0x4651u, 1u); place_w8(ram, PLACE_DS, 0x4653u, 1u);
    place_w8(ram, PLACE_DS, 0x4655u, 2u); place_w8(ram, PLACE_DS, 0x4657u, 0u);
    place_w8(ram, PLACE_DS, 0x49FAu, 2u);
    place_w8(ram, PLACE_DS, 0x6AADu, 0u); place_w8(ram, PLACE_DS, 0x6AAEu, 0u);
}

static pr_u33_1577_cpu enter(uint8_t *ram, uint8_t stale_bp2)
{
    pr_u33_1577_cpu cpu = {0x1111u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u,
                           0x7777u, 0x8000u, PLACE_CS, 0x1577u, PLACE_DS, 0x9999u, PLACE_SS, 0x0202u};
    place_w16(ram, PLACE_SS, 0x8000u, 0x7000u);
    place_w16(ram, PLACE_SS, 0x8002u, 0x7001u);
    place_w16(ram, PLACE_SS, 0x8004u, 0x5A01u);              /* slot q = 1 */
    place_w8(ram, PLACE_SS, 0x7FFCu, stale_bp2);              /* [BP-2] before any write */
    return cpu;
}

int main(void)
{
    static const uint8_t cardinal[4] = {0, 2, 4, 6}, frozen[4] = {8, 8, 8, 8};
    uint8_t *ram = malloc(PLACE_RAM);
    pr_u33_1577_cpu cpu;
    int callee = 0;
    unsigned i, closed;
    CHECK(ram != NULL);

    /* The first position (5, 2) is open and claimable: one +1434 call claims it, AL = 1. */
    place_world(ram, 1u, 3u, 0u);
    search_tables(ram, 5u, 2u, cardinal);
    cpu = enter(ram, 0u);
    CHECK(pr_u33_place_search_1577(ram, PLACE_RAM, &cpu, 1000u, &callee) == PR_U33_1577_OK);
    CHECK((cpu.ax & 0xFFu) == 1u && cpu.ip == 0x7000u && cpu.cs == 0x7001u && cpu.sp == 0x8006u && cpu.bp == 0x7777u);
    CHECK(place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + 0x108u + 0x0Bu * 2u + 5u)) == 0u);   /* map 1, pass 0, y 2, x 5 */
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);                 /* CMP [BP-4],0 with [BP-4] = 0 */

    /* Nothing is open: the walk leaves the grid on every pass and ends with AL = 0. */
    place_world(ram, 0u, 3u, 0u);
    search_tables(ram, 5u, 2u, cardinal);
    place_w8(ram, PLACE_DS, 0x02F4u, 3u);                    /* the ring moves diagonally, so both axes leave */
    cpu = enter(ram, 0u);
    CHECK(pr_u33_place_search_1577(ram, PLACE_RAM, &cpu, 100000u, &callee) == PR_U33_1577_OK);
    CHECK((cpu.ax & 0xFFu) == 0u && (cpu.flags & 0x08D5u) == 0x0000u && cpu.sp == 0x8006u);
    for (closed = 0, i = 0; i < 0x210u; ++i) closed += place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + i)) == 0u;
    CHECK(closed == 0x210u);

    /* A first base outside x only never writes [BP-2]: a stale nonzero byte there ends the search
     * with AL = 1 although nothing was claimed, as in the original. */
    place_world(ram, 1u, 3u, 0u);
    search_tables(ram, 12u, 2u, cardinal);
    cpu = enter(ram, 0x33u);
    CHECK(pr_u33_place_search_1577(ram, PLACE_RAM, &cpu, 1000u, &callee) == PR_U33_1577_OK);
    CHECK((cpu.ax & 0xFFu) == 1u);
    for (closed = 0, i = 0; i < 0x210u; ++i) closed += place_r8(ram, PLACE_DS, (uint16_t)(0x4440u + i)) == 0u;
    CHECK(closed == 0u);

    /* Every direction maps to the zero vector and nothing is open: the original never returns,
     * and the host cap stops the model at the loop head. */
    place_world(ram, 0u, 3u, 0u);
    search_tables(ram, 5u, 2u, frozen);
    cpu = enter(ram, 0u);
    CHECK(pr_u33_place_search_1577(ram, PLACE_RAM, &cpu, 50u, &callee) == PR_U33_1577_LIMIT);
    CHECK(cpu.cs == PLACE_CS && cpu.ip == 0x15ABu);

    {
        pr_u33_1577_cpu before = enter(ram, 0u), after = before;
        CHECK(pr_u33_place_search_1577(NULL, PLACE_RAM, &after, 10u, NULL) == PR_U33_1577_ARGUMENT);
        CHECK(pr_u33_place_search_1577(ram, PLACE_RAM - 1u, &after, 10u, NULL) == PR_U33_1577_ARGUMENT);
        CHECK(pr_u33_place_search_1577(ram, PLACE_RAM, NULL, 10u, NULL) == PR_U33_1577_ARGUMENT);
        CHECK(memcmp(&before, &after, sizeof before) == 0);
    }
    free(ram);
    printf("u33 +1577: claim, exhausted, stale-byte, cap and guard cases passed\n");
    return 0;
}
