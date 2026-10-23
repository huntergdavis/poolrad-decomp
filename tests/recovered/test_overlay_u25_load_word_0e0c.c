#include "poolrad/recovered/overlay_u25_load_word_0e0c.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

static uint8_t ram[PR_U25_0756_RAM_SIZE];

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_0756_RAM_SIZE - 1u);
}

static void put(uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = at(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_0756_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get(uint16_t seg, uint16_t off)
{
    size_t p = at(seg, off);
    return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & (PR_U25_0756_RAM_SIZE - 1u)] << 8));
}

static void pointer(uint16_t off, uint16_t seg, uint16_t rel)
{
    put(0x1000u, off, rel);
    put(0x1000u, (uint16_t)(off + 2u), seg);
}

static void setup(void)
{
    memset(ram, 0xA5, sizeof ram);
    pointer(0x49D2u, 0x4000u, 0x100u);
    pointer(0x49D6u, 0x4200u, 0x100u);
    pointer(0x49DAu, 0x4400u, 0x100u);
    pointer(0x49DEu, 0x4600u, 0x100u);
    pointer(0x5D92u, 0x5000u, 0x100u);
    pointer(0x5D96u, 0u, 0u);
}

static pr_u25_load_word_cpu cpu(void)
{
    pr_u25_load_word_cpu c = {
        0x9234u, 0x1357u, 0x2468u, 0xABCDu, 0x5678u, 0x6789u,
        0x4321u, 0x8000u, 0x3000u, 0x0E0Cu, 0x1000u, 0x6000u, 0x2000u, 0x202u
    };
    return c;
}

static pr_u25_load_word_status run(uint16_t address, pr_u25_load_word_cpu *c)
{
    *c = cpu();
    put(c->ss, 0x7FFCu, 0xBEEFu);
    put(c->ss, 0x7FEAu, 0xCAFEu);
    put(c->ss, c->sp, 0x1111u);
    put(c->ss, (uint16_t)(c->sp + 2u), 0x7000u);
    put(c->ss, (uint16_t)(c->sp + 4u), address);
    return pr_u25_load_word_0e0c(ram, sizeof ram, c);
}

static int test_pseudo_addresses(void)
{
    pr_u25_load_word_cpu c;
    unsigned value;
    setup();
    put(0x1000u, 0x6F6Eu, 0xFEDCu);
    put(0x1000u, 0x6F6Au, 0x1234u);
    put(0x1000u, 0x6F6Cu, 0xABCDu);
    CHECK(run(0xB1u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xFEDCu);
    CHECK(c.bp == 0x4321u && c.sp == 0x8006u && c.cs == 0x7000u && c.ip == 0x1111u);
    CHECK(c.bx == 0x1357u && c.cx == 0x2468u && c.dx == 0xABCDu && c.si == 0x5678u);
    CHECK(run(0xFBu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x1234u);
    CHECK(run(0xFCu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xABCDu);
    for (value = 0u; value < 256u; ++value) {
        uint16_t sx = (uint16_t)(value < 128u ? value : value | 0xFF00u);
        ram[at(0x1000u, 0x6AADu)] = (uint8_t)value;
        ram[at(0x1000u, 0x6AAEu)] = (uint8_t)value;
        ram[at(0x1000u, 0x6AAFu)] = (uint8_t)value;
        ram[at(0x1000u, 0x6AB0u)] = (uint8_t)value;
        ram[at(0x1000u, 0x6AB1u)] = (uint8_t)value;
        CHECK(run(0x33Du, &c) == PR_U25_LOAD_WORD_OK && c.ax == value);
        CHECK(run(0xC04Bu, &c) == PR_U25_LOAD_WORD_OK && c.ax == sx);
        CHECK(get(c.ss, 0x8004u) == 0u);
        CHECK(run(0xC04Cu, &c) == PR_U25_LOAD_WORD_OK && c.ax == sx);
        CHECK(get(c.ss, 0x8004u) == 1u);
        CHECK(run(0xC04Du, &c) == PR_U25_LOAD_WORD_OK && c.ax == value / 2u);
        CHECK(run(0xC04Eu, &c) == PR_U25_LOAD_WORD_OK && c.ax == value);
        CHECK(run(0xC04Fu, &c) == PR_U25_LOAD_WORD_OK && c.ax == value);
    }
    CHECK(run(0x35Fu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xBEEFu);
    CHECK(get(c.ss, 0x8004u) == 0x35Fu);
    CHECK(run(0xC059u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xBEEFu);
    CHECK(get(c.ss, 0x8004u) == 14u);
    CHECK(run(0xFFFFu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xBEEFu);
    return 0;
}

static int test_windows_and_hook(void)
{
    pr_u25_load_word_cpu c;
    unsigned i;
    setup();
    put(0x4000u, 0x100u, 0x1234u);
    put(0x4400u, 0x100u, 0xABCDu);
    ram[at(0x4600u, 0x100u)] = 0xFFu;
    CHECK(run(0x4900u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x1234u);
    CHECK(run(0x9700u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xABCDu);
    CHECK(run(0x9900u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xFFu);
    /* The published controlled native case: record 7 overrides window 10. */
    ram[at(0x5000u, 0x114u)] = 7u;
    put(0x4200u, 0x130u, 10u);
    CHECK(run(0x6B18u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 7u);
    CHECK(get(0x4200u, 0x130u) == 10u && ram[at(0x5000u, 0x114u)] == 7u);
    ram[at(0x5000u, 0x16Eu)] = 0xFFu;
    ram[at(0x5000u, 0x173u)] = 0xFFu;
    CHECK(run(0x6B9Bu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xFFu);
    CHECK(run(0x6BA0u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xFFFFu);
    for (i = 0u; i < 8u; ++i) {
        ram[at(0x5000u, (uint16_t)(0x177u + i))] = (uint8_t)(i * 7u);
        CHECK(run((uint16_t)(0x6BA5u + i), &c) == PR_U25_LOAD_WORD_OK && c.ax == i * 7u);
    }
    put(0x4200u, 0x6AAu, 0x3456u);
    CHECK(run(0x6DD5u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x3456u);
    CHECK(run(0x6C0Du, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xCAFEu);
    ram[at(0x5000u, 0x20Du)] = 0u;
    CHECK(run(0x6C00u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x80u);
    ram[at(0x5000u, 0x20Du)] = 1u;
    CHECK(run(0x6C00u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 1u);
    ram[at(0x5000u, 0x20Eu)] = 0u;
    ram[at(0x5000u, 0x20Fu)] = 1u;
    CHECK(run(0x6C0Cu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x80u);
    ram[at(0x5000u, 0x20Eu)] = 1u;
    CHECK(run(0x6C0Cu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x81u);
    ram[at(0x5000u, 0x20Eu)] = 2u;
    CHECK(run(0x6C0Cu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0u);
    for (i = 0u; i < 256u; ++i) {
        uint16_t expected = 0xCAFEu;
        if (i == 3u) expected = 0u;
        else if (i >= 4u && i <= 7u) expected = (uint16_t)((i - 3u) * 5u);
        else if (i >= 8u && i <= 12u) expected = 25u;
        else if (i >= 13u && i <= 15u) expected = (uint16_t)((i - 7u) * 5u);
        else if (i == 16u || i == 17u) expected = (uint16_t)((i - 6u) * 5u);
        else if (i >= 18u && i <= 25u) expected = 60u;
        ram[at(0x5000u, 0x115u)] = (uint8_t)i;
        CHECK(run(0x6DCFu, &c) == PR_U25_LOAD_WORD_OK && c.ax == expected);
    }
    CHECK(run(0x6DB1u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0u); /* empty party */
    pointer(0x5D96u, 0x5100u, 0x100u);
    put(0x5100u, 0x204u, 0u);
    put(0x5100u, 0x206u, 0u);
    CHECK(run(0x6DB4u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 1u); /* active not in list */
    return 0;
}

static int test_guards(void)
{
    pr_u25_load_word_cpu c = cpu();
    setup();
    CHECK(pr_u25_load_word_0e0c(NULL, sizeof ram, &c) == PR_U25_LOAD_WORD_ARGUMENT);
    CHECK(pr_u25_load_word_0e0c(ram, sizeof ram - 1u, &c) == PR_U25_LOAD_WORD_ARGUMENT);
    CHECK(pr_u25_load_word_0e0c(ram, sizeof ram, NULL) == PR_U25_LOAD_WORD_ARGUMENT);
    c.ip = 0xE0Du;
    CHECK(pr_u25_load_word_0e0c(ram, sizeof ram, &c) == PR_U25_LOAD_WORD_ARGUMENT);
    CHECK(c.sp == 0x8000u && ram[at(c.ss, 0x7FFEu)] == 0xA5u);
    return 0;
}

/* +07BC entered on its own: frame [SP] IP, [SP+2] CS, [SP+4]/[SP+6] the flag pointer, [SP+8] the address; RETF 6. */
static pr_u25_load_word_status hook(uint16_t address, pr_u25_load_word_cpu *c)
{
    *c = cpu();
    c->ip = 0x07BCu;
    put(c->ss, 0x7FFCu, 0xBEEFu);                  /* the result local [BP-2] at entry SP 8000h */
    put(c->ss, c->sp, 0x1111u);
    put(c->ss, (uint16_t)(c->sp + 2u), 0x7000u);
    put(c->ss, (uint16_t)(c->sp + 4u), 0x0040u);
    put(c->ss, (uint16_t)(c->sp + 6u), 0x7800u);
    put(c->ss, (uint16_t)(c->sp + 8u), address);
    ram[at(0x7800u, 0x0040u)] = 0x77u;
    return pr_u25_load_hook_07bc(ram, sizeof ram, c);
}

static int test_hook_entry(void)
{
    pr_u25_load_word_cpu c;
    setup();
    ram[at(0x5000u, 0x0114u)] = 0x9Cu;             /* record +14h, key 18h: zero-extended */
    CHECK(hook(0x6B18u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0x009Cu && ram[at(0x7800u, 0x0040u)] == 1u);
    CHECK(c.bp == 0x4321u && c.sp == 0x800Au && c.cs == 0x7000u && c.ip == 0x1111u);
    ram[at(0x5000u, 0x0173u)] = 0x9Cu;             /* record +73h, key A0h: sign-extended */
    CHECK(hook(0x6BA0u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xFF9Cu);
    CHECK(hook(0x6C0Du, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xBEEFu && ram[at(0x7800u, 0x0040u)] == 1u);   /* key 10Dh: flag stays 1, local unwritten */
    CHECK(hook(0x9900u, &c) == PR_U25_LOAD_WORD_OK && c.ax == 0xBEEFu && ram[at(0x7800u, 0x0040u)] == 0u);   /* not found: flag cleared, local unwritten */
    ram[at(0x5000u, 0x0115u)] = 0x0Au;             /* key 2CFh: class 0Ah (8..0Ch) gives 25 */
    CHECK(hook(0x6DCFu, &c) == PR_U25_LOAD_WORD_OK && c.ax == 25u);
    c = cpu();
    c.ip = 0x0E0Cu;
    CHECK(pr_u25_load_hook_07bc(ram, sizeof ram, &c) == PR_U25_LOAD_WORD_ARGUMENT && c.sp == 0x8000u);
    c.ip = 0x07BCu;
    CHECK(pr_u25_load_hook_07bc(ram, sizeof ram - 1u, &c) == PR_U25_LOAD_WORD_ARGUMENT);
    CHECK(pr_u25_load_hook_07bc(NULL, sizeof ram, &c) == PR_U25_LOAD_WORD_ARGUMENT);
    CHECK(pr_u25_load_hook_07bc(ram, sizeof ram, NULL) == PR_U25_LOAD_WORD_ARGUMENT);
    return 0;
}

int main(void)
{
    if (test_pseudo_addresses() || test_windows_and_hook() || test_guards() || test_hook_entry())
        return 1;
    puts("u25 load word synthetic tests passed");
    return 0;
}
