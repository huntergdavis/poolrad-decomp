#include "poolrad/recovered/overlay_u6a_party_before_14a9.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6A_14A9_RAM_SIZE - 1u);
}
static void put(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8u);
}
static uint16_t word(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8u));
}
int main(void)
{
    uint8_t *ram = malloc(PR_U6A_14A9_RAM_SIZE);
    unsigned n, active, mixed, cases = 0u;
    CHECK(ram != NULL);
    for (mixed = 0; mixed < 2u; ++mixed) for (n = 1; n <= 8u; ++n)
        for (active = 0; active < n; ++active) {
            uint16_t offsets[8], segments[8], at_offset, at_segment;
            unsigned expected[8], i;
            pr_u6a_14a9_cpu cpu = {0x1234,0x3456,0x5678,0x789A,0x9ABC,
                0xBCDE,0x7777,0x8000,0x3000,0x14A9,0x1000,0x1111,0x2000,0x7202};
            memset(ram, 0xA5, PR_U6A_14A9_RAM_SIZE);
            for (i = 0; i < n; ++i) {
                offsets[i] = (uint16_t)(0x100u + 0x200u * i);
                segments[i] = (uint16_t)(0x4000u + (mixed ? (i % 2u) * 0x1000u : 0u));
                expected[i] = active == 0u ? (i + 1u) % n : i;
            }
            if (active != 0u) { expected[active] = active - 1u; expected[active - 1u] = active; }
            for (i = 0; i < n; ++i) {
                size_t at = physical(segments[i], (uint16_t)(offsets[i] + 0x104u));
                put(ram, at, i + 1u < n ? offsets[i + 1u] : 0u);
                put(ram, at + 2u, i + 1u < n ? segments[i + 1u] : 0u);
            }
            put(ram, 0x15D92, offsets[active]); put(ram, 0x15D94, segments[active]);
            put(ram, 0x15D96, offsets[0]); put(ram, 0x15D98, segments[0]);
            put(ram, 0x28000, 0x7000); put(ram, 0x28002, 0x3000);
            CHECK(pr_u6a_party_before_14a9(ram, PR_U6A_14A9_RAM_SIZE, &cpu) == PR_U6A_14A9_OK);
            at_offset = word(ram, 0x15D96); at_segment = word(ram, 0x15D98);
            for (i = 0; i < n; ++i) {
                size_t at;
                CHECK(at_offset == offsets[expected[i]] && at_segment == segments[expected[i]]);
                at = physical(at_segment, (uint16_t)(at_offset + 0x104u));
                at_offset = word(ram, at); at_segment = word(ram, at + 2u);
            }
            CHECK(at_offset == 0u && at_segment == 0u);
            CHECK(word(ram, 0x15D92) == offsets[active] && word(ram, 0x15D94) == segments[active]);
            CHECK(cpu.bx == 0x3456 && cpu.cx == 0x5678 && cpu.si == 0x9ABC);
            CHECK(cpu.bp == 0x7777 && cpu.sp == 0x8004 && cpu.ip == 0x7000 && cpu.cs == 0x3000);
            CHECK(cpu.ds == 0x1000 && cpu.ss == 0x2000 && word(ram, 0x27FFE) == 0x7777);
            ++cases;
        }
    {
        pr_u6a_14a9_cpu cpu = {0}, before = cpu;
        CHECK(pr_u6a_party_before_14a9(NULL, PR_U6A_14A9_RAM_SIZE, &cpu) == PR_U6A_14A9_ARGUMENT);
        CHECK(pr_u6a_party_before_14a9(ram, 0u, &cpu) == PR_U6A_14A9_ARGUMENT);
        CHECK(pr_u6a_party_before_14a9(ram, PR_U6A_14A9_RAM_SIZE, NULL) == PR_U6A_14A9_ARGUMENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    }
    free(ram);
    printf("u6A +14A9: %u list permutations and host guards passed\n", cases);
    return 0;
}
