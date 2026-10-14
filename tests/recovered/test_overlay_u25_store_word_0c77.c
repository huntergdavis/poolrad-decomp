#include "poolrad/recovered/overlay_u25_store_word_0c77.h"

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
    ram[at(0x1000u, 0x84D2u)] = 0u;
    ram[at(0x1000u, 0x84D3u)] = 0u;
    ram[at(0x1000u, 0x84E5u)] = 0u;
    ram[at(0x1000u, 0x84E6u)] = 0u;
    ram[at(0x1000u, 0x84E8u)] = 0u;
}

static pr_u25_store_word_cpu cpu(void)
{
    pr_u25_store_word_cpu c = {
        0x9234u, 0x1357u, 0x2468u, 0xABCDu, 0x5678u, 0x6789u,
        0x4321u, 0x8000u, 0x3000u, 0x0C77u, 0x1000u, 0x6000u, 0x2000u, 0x202u
    };
    return c;
}

static pr_u25_store_word_status run(uint16_t address, uint16_t value, pr_u25_store_word_cpu *c)
{
    *c = cpu();
    put(c->ss, 0x7FFCu, 0xBEEFu);
    put(c->ss, 0x7FEAu, 0xCAFEu);
    put(c->ss, c->sp, 0x1111u);
    put(c->ss, (uint16_t)(c->sp + 2u), 0x7000u);
    put(c->ss, (uint16_t)(c->sp + 4u), value);
    put(c->ss, (uint16_t)(c->sp + 6u), address);
    return pr_u25_store_word_0c77(ram, sizeof ram, c);
}

static int test_class4(void)
{
    pr_u25_store_word_cpu c;
    unsigned i;
    setup();
    CHECK(run(0x3DEu, 0xFEDCu, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x1000u, 0x84CCu) == 0xFEDCu);
    CHECK(c.sp == 0x8008u && c.bp == 0x4321u && c.cs == 0x7000u && c.ip == 0x1111u);
    CHECK(run(0xB8u, 0xABCDu, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x1000u, 0x84CEu) == 0xABCDu);
    CHECK(run(0xB9u, 0x1234u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x1000u, 0x84D0u) == 0x1234u);
    for (i = 0u; i < 256u; ++i) {
        CHECK(run(0xC04Bu, (uint16_t)i, &c) == PR_U25_STORE_WORD_OK);
        CHECK(ram[at(0x1000u, 0x6AADu)] == i && get(c.ss, 0x8006u) == 0xE3u);
        CHECK(run(0xC04Cu, (uint16_t)i, &c) == PR_U25_STORE_WORD_OK);
        CHECK(ram[at(0x1000u, 0x6AAEu)] == i && get(c.ss, 0x8006u) == 0xE4u);
        CHECK(run(0xC04Du, (uint16_t)i, &c) == PR_U25_STORE_WORD_OK);
        CHECK(ram[at(0x1000u, 0x6AAFu)] == 2u * (i % 4u));
        CHECK(get(c.ss, 0x8004u) == i % 4u && get(c.ss, 0x8006u) == 0xE5u);
        CHECK(run(0xC059u, (uint16_t)i, &c) == PR_U25_STORE_WORD_OK);
        CHECK(ram[at(0x1000u, 0x6F68u)] == i && ram[at(0x1000u, 0x84E5u)] == 1u);
        CHECK(run(0xC05Fu, (uint16_t)i, &c) == PR_U25_STORE_WORD_OK);
        CHECK(ram[at(0x1000u, 0x6F69u)] == i);
    }
    CHECK(run(0xC04Du, 0xFFFFu, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(c.ss, 0x8004u) == 3u && ram[at(0x1000u, 0x6AAFu)] == 6u);
    CHECK(ram[at(0x1000u, 0x84E6u)] == 1u);
    CHECK(run(0xC04Fu, 7u, &c) == PR_U25_STORE_WORD_OK && get(c.ss, 0x8006u) == 0xE7u);
    CHECK(run(0xFBu, 0u, &c) == PR_U25_STORE_WORD_OK && get(0x1000u, 0x84CCu) == 0xFEDCu);
    return 0;
}

static int test_windows_and_record_order(void)
{
    pr_u25_store_word_cpu c;
    setup();
    CHECK(run(0x49FDu, 7u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x4000u, 0x2FAu) == 7u && ram[at(0x1000u, 0x84E8u)] == 1u);
    CHECK(run(0x9808u, 0xFFFFu, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x4400u, 0x310u) == 0xFFFFu);
    CHECK(run(0x9900u, 0x1234u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x4600u, 0x100u)] == 0x34u && c.dx == 0xAB34u);
    put(0x4200u, 0x140u, 0xBEEFu);
    CHECK(run(0x6B20u, 7u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x4200u, 0x140u) == 7u && ram[at(0x5000u, 0x118u)] == 7u);
    setup();
    pointer(0x49D6u, 0x1000u, 0x5D52u);
    CHECK(run(0x6B20u, 7u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x1000u, 0x5D92u) == 7u);
    CHECK(ram[at(0x5000u, 0x1Fu)] == 7u && ram[at(0x5000u, 0x118u)] == 0xA5u);
    setup();
    pointer(0x5D92u, 0x1000u, 0x5C84u);
    CHECK(run(0x6C0Cu, 0x81u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(get(0x1000u, 0x5D92u) == 0x5C01u);
    CHECK(ram[at(0x1000u, 0x5D10u)] == 1u);
    CHECK(ram[at(0x1000u, 0x5D93u)] == 0x5Cu);
    return 0;
}

static int test_local_hook_conditions(void)
{
    pr_u25_store_word_cpu c;
    setup();
    CHECK(run(0x6BB8u, 0xB2u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x184u)] == 0xB2u);
    CHECK(run(0x6BB8u, 0xB3u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x184u)] == 0x81u);
    CHECK(run(0x6BB8u, 0xFFFFu, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x184u)] == 0xCDu);
    CHECK(get(c.ss, 0x8004u) == 0xFFFFu && get(c.ss, 0x7FF8u) == 0xFFCDu);
    CHECK(run(0x6C00u, 0x87u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x20Du)] == 0u && ram[at(0x5000u, 0x20Cu)] == 7u);
    CHECK(run(0x6C00u, 0u, &c) == PR_U25_STORE_WORD_OK && ram[at(0x1000u, 0x84D2u)] == 1u);
    CHECK(run(0x6B00u, 0u, &c) == PR_U25_STORE_WORD_OK && ram[at(0x1000u, 0x84D3u)] == 1u);
    CHECK(run(0x6C0Cu, 0x80u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x20Eu)] == 0u && ram[at(0x5000u, 0x20Fu)] == 1u);
    CHECK(run(0x6C0Cu, 0u, &c) == PR_U25_STORE_WORD_OK);
    CHECK(ram[at(0x5000u, 0x20Eu)] == 0u && ram[at(0x5000u, 0x20Fu)] == 0u);
    return 0;
}

static int test_explicit_dependencies_and_guards(void)
{
    pr_u25_store_word_cpu c;
    setup();
    CHECK(run(0x6E12u, 0xFF42u, &c) == PR_U25_STORE_WORD_DEPENDENCY);
    CHECK(c.cs == 0x3000u && c.ip == 0x0C03u && c.ax == 0x0342u);
    CHECK(get(c.ss, c.sp) == 0x0342u && c.sp == (uint16_t)(c.bp - 4u));
    CHECK(get(0x4200u, 0x724u) == 0xFF42u); /* window already stored */
    CHECK(run(0x6E22u, 0x81u, &c) == PR_U25_STORE_WORD_DEPENDENCY);
    CHECK(c.ip == 0x0C26u && get(c.ss, c.sp) == 1u);
    CHECK(get(c.ss, (uint16_t)(c.sp + 2u)) == 1u && get(c.ss, (uint16_t)(c.bp + 6u)) == 1u);
    CHECK(run(0x6E24u, 0xFF42u, &c) == PR_U25_STORE_WORD_DEPENDENCY);
    CHECK(c.ip == 0x0C49u && get(c.ss, c.sp) == 2u && get(c.ss, (uint16_t)(c.sp + 2u)) == 0x42u);
    CHECK(run(0x6E26u, 0x100u, &c) == PR_U25_STORE_WORD_DEPENDENCY);
    CHECK(c.ip == 0x0C6Cu && get(c.ss, c.sp) == 3u && get(c.ss, (uint16_t)(c.sp + 2u)) == 0u);
    CHECK(run(0x6E26u, 0x80u, &c) == PR_U25_STORE_WORD_OK && c.sp == 0x8008u);
    c = cpu();
    CHECK(pr_u25_store_word_0c77(NULL, sizeof ram, &c) == PR_U25_STORE_WORD_ARGUMENT);
    CHECK(pr_u25_store_word_0c77(ram, sizeof ram - 1u, &c) == PR_U25_STORE_WORD_ARGUMENT);
    CHECK(pr_u25_store_word_0c77(ram, sizeof ram, NULL) == PR_U25_STORE_WORD_ARGUMENT);
    c.ip = 0x0C78u;
    CHECK(pr_u25_store_word_0c77(ram, sizeof ram, &c) == PR_U25_STORE_WORD_ARGUMENT);
    return 0;
}

int main(void)
{
    if (test_class4() || test_windows_and_record_order() ||
        test_local_hook_conditions() || test_explicit_dependencies_and_guards())
        return 1;
    puts("bounded u25 store-word tests passed");
    return 0;
}
