/* Focused synthetic test for the raw-CPU unit 0x25 +19DB party step (docs/re/OVL_0025_19DB_CPU.md), composed with the raw unit 0x3BD +057A and +047B.
 * No game bytes: the stub slots, the map and the state bytes are this test's (the models read no code bytes). */
#include "poolrad/recovered/overlay_u25_move_19db_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "19DB line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define RAM PR_BUS_RAM_SIZE
static uint8_t *m;
static size_t p(unsigned s, unsigned o) { return ((size_t)s * 16u + (o & 65535u)) & (RAM - 1u); }
static void put(unsigned s, unsigned o, unsigned v) { size_t a = p(s, o); m[a] = (uint8_t)v; m[(a + 1u) & (RAM - 1u)] = (uint8_t)(v >> 8); }

enum { CS = 0x6000, U3BD = 0x7000, STUB = 0x13D5, DS = 0x1C7C, SS = 0x8000, SP0 = 0xF000, MAPSEG = 0x9400 };
static void slot(unsigned off, unsigned entry) { m[p(STUB, off)] = 0xEA; put(STUB, off + 1u, entry); put(STUB, off + 3u, U3BD); }
static void lcall(unsigned site, unsigned off) { m[p(CS, site)] = 0x9A; put(CS, site + 1u, off); put(CS, site + 3u, STUB); }
static pr_u25_19db_cpu setup(unsigned x, unsigned y, unsigned facing, unsigned map_id)
{
    pr_u25_19db_cpu c = {0x1234, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, SP0, CS, 0x19DB, DS, 0x8888, SS, 0x0202};
    unsigned k;
    memset(m, 0x5A, RAM);
    lcall(0x1A47, 0x0034); lcall(0x1A5B, 0x002A); slot(0x0034, 0x057A); slot(0x002A, 0x047B);
    m[p(DS, 0x6AAD)] = (uint8_t)x; m[p(DS, 0x6AAE)] = (uint8_t)y; m[p(DS, 0x6AAF)] = (uint8_t)facing; m[p(DS, 0x84DC)] = (uint8_t)map_id;
    put(DS, 0x6A5C, 0x0000); put(DS, 0x6A5E, MAPSEG);
    for (k = 0; k < 1024u; ++k) m[p(MAPSEG, k)] = (uint8_t)(k * 7u + 3u);
    put(SS, SP0, 0x7000); put(SS, SP0 + 2u, 0x9001);
    return c;
}

int main(void)
{
    static pr_ega ega; static uint8_t before[RAM];
    static const unsigned facing[4] = {0, 2, 4, 6}, edge[6] = {0x00, 0x01, 0x0E, 0x0F, 0x80, 0xFF};
    pr_bus bus; pr_u25_19db_cpu c, saved; size_t left; unsigned f, i, x, y;
    m = malloc(RAM); CHECK(m != NULL);
    memset(&bus, 0, sizeof bus); bus.ram = m; bus.ega = &ega; pr_ega_reset(&ega);
    /* the step rule, for each facing and each edge value of the moved coordinate (signed compares: 80h..FFh are below 0) */
    for (f = 0; f < 4u; ++f)
        for (i = 0; i < 6u; ++i) {
            int moved, sv = (int)(int8_t)(uint8_t)edge[i];
            x = (facing[f] == 2 || facing[f] == 6) ? edge[i] : 5u; y = (facing[f] == 0 || facing[f] == 4) ? edge[i] : 5u;
            c = setup(x, y, facing[f], 1); left = 1000;
            CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_RETURNED);
            if (facing[f] == 0) moved = sv > 0 ? sv - 1 : 0x0F;
            else if (facing[f] == 4) moved = sv < 0x0F ? sv + 1 : 0;
            else if (facing[f] == 2) moved = sv < 0x0F ? sv + 1 : 0;
            else moved = sv > 0 ? sv - 1 : 0x0F;
            if (facing[f] == 0 || facing[f] == 4) CHECK(m[p(DS, 0x6AAE)] == (uint8_t)moved && m[p(DS, 0x6AAD)] == 5u);
            else CHECK(m[p(DS, 0x6AAD)] == (uint8_t)moved && m[p(DS, 0x6AAE)] == 5u);
            CHECK(c.cs == 0x9001 && c.ip == 0x7000 && c.sp == SP0 + 4u && c.bp == 0x7777 && m[p(DS, 0x6AB0)] == (uint8_t)c.ax);
        }
    /* the cell result: inside the grid the byte at map + 200h + (y SHL 4) + x */
    c = setup(3, 4, 2, 1); left = 1000; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_RETURNED);
    CHECK(m[p(DS, 0x6AAD)] == 4 && m[p(DS, 0x6AB1)] == (uint8_t)((0x200u + 0x40u + 4u) * 7u + 3u));
    /* an unmatched facing moves nothing */
    c = setup(3, 4, 5, 1); left = 1000; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_RETURNED);
    CHECK(m[p(DS, 0x6AAD)] == 3 && m[p(DS, 0x6AAE)] == 4);
    /* an unloaded stub (INT 3Fh): STUB at the LCALL, after the step, before the frame; the step is already written */
    c = setup(3, 4, 4, 1); m[p(STUB, 0x0034)] = 0xCD; m[p(STUB, 0x0035)] = 0x3F; left = 1000;
    CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_STUB && c.ip == 0x1A47 && c.cs == CS && c.sp == SP0 - 6u && m[p(DS, 0x6AAE)] == 5);
    c = setup(3, 4, 4, 1); put(STUB, 0x002B, 0x0350); left = 1000;                  /* a loaded slot to another entry */
    CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_STUB && c.ip == 0x1A5B && m[p(DS, 0x6AB1)] != 0x5A);
    /* the budget: LIMIT at the next unexecuted instruction; the far call costs one unit */
    c = setup(3, 4, 0, 1); left = 2; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_LIMIT && c.ip == 0x19DE && left == 0);
    c = setup(3, 4, 0, 1); saved = c; memcpy(before, m, RAM); left = 0;
    CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_LIMIT && !memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM));
    /* argument rejection is atomic */
    c = setup(3, 4, 0, 1); memcpy(before, m, RAM); left = 100;
    c.ip = 0x19DC; saved = c; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_ARGUMENT);
    c.ip = 0x19DB; c.sp = 0x002F; saved = c; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_ARGUMENT);
    c.sp = 0xFFFD; saved = c; CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, &left) == PR_U25_19DB_ARGUMENT);
    CHECK(!memcmp(&c, &saved, sizeof c) && !memcmp(m, before, RAM) && left == 100); c.sp = SP0;
    CHECK(pr_u25_move_19db_cpu(NULL, RAM, &c, &left) == PR_U25_19DB_ARGUMENT);
    CHECK(pr_u25_move_19db_cpu(&bus, RAM - 1u, &c, &left) == PR_U25_19DB_ARGUMENT);
    CHECK(pr_u25_move_19db_cpu(&bus, RAM, NULL, &left) == PR_U25_19DB_ARGUMENT);
    CHECK(pr_u25_move_19db_cpu(&bus, RAM, &c, NULL) == PR_U25_19DB_ARGUMENT);
    free(m); printf("OVL_0025_19DB synthetic, stub, budget and guard cases passed (%u checks)\n", checks); return 0;
}
