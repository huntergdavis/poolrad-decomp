#include "poolrad/recovered/overlay_u6a_record_remaining_025c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static void word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8u);
}

static pr_u6a_025c_cpu fixture(uint8_t *ram, uint8_t low, uint8_t high)
{
    pr_u6a_025c_cpu cpu = {0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC,
        0xBCDE, 0x7777, 0x8000, 0x3000, 0x025C, 0x1000, 0x1111,
        0x2000, 0x7202};
    memset(ram, 0xA5, PR_U6A_025C_RAM_SIZE);
    word(ram, 0x15D92, 0x0100);
    word(ram, 0x15D94, 0x4000);
    word(ram, 0x28000, 0x7000);
    word(ram, 0x28002, 0x3000);
    word(ram, 0x28004, (uint16_t)(0x5A00u | low));
    word(ram, 0x28006, (uint16_t)(0xC300u | high));
    memset(ram + 0x40117, 0, 21u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6A_025C_RAM_SIZE);
    pr_u6a_025c_cpu cpu, before;
    unsigned i;
    CHECK(ram != NULL);
    cpu = fixture(ram, 1u, 2u);
    for (i = 0; i < 21u; ++i) {
        static const uint8_t list[] = {0u, 0x81u, 3u, 0x82u};
        ram[0x40117u + i] = list[i % 4u];
    }
    ram[0x131FA + 16] = ram[0x131FA + 48] = 1u;
    ram[0x131FB + 16] = ram[0x131FB + 48] = 2u;
    ram[0x401B6] = 7u;
    CHECK(pr_u6a_record_remaining_025c(ram, PR_U6A_025C_RAM_SIZE, &cpu) == PR_U6A_025C_OK);
    CHECK(cpu.ax == 0xFFFD && cpu.cx == 10u && cpu.dx == 2u && cpu.si == 1u);
    CHECK(cpu.di == 0x0105 && cpu.es == 0x4000 && cpu.bx == 0x3456);
    CHECK((cpu.flags & 0x08D5u) == 0x0091u);
    CHECK(cpu.bp == 0x7777 && cpu.sp == 0x8008 && cpu.ip == 0x7000 && cpu.cs == 0x3000);
    CHECK(ram[0x27FFB] == 20u && ram[0x27FFC] == 10u && ram[0x27FFD] == 0xFDu);
    CHECK(ram[0x27FFA] == 0xA5u && ram[0x28008] == 0xA5u);

    cpu = fixture(ram, 0xFFu, 0x80u);
    ram[0x4012E] = 0x81u; /* B1 + 3*(-1) + (-128) = 2E */
    CHECK(pr_u6a_record_remaining_025c(ram, PR_U6A_025C_RAM_SIZE, &cpu) == PR_U6A_025C_OK);
    CHECK(cpu.ax == 0x81u && cpu.cx == 0u && cpu.dx == 0xFF80u && cpu.si == 0xFFFFu);
    CHECK(cpu.di == 0x007Du);

    cpu = fixture(ram, 0u, 0u);
    memset(ram + 0x40117, 0x80, 21u); /* nonzero byte, masked descriptor zero */
    ram[0x131FA] = ram[0x131FB] = 0u;
    ram[0x401B1] = 21u;
    CHECK(pr_u6a_record_remaining_025c(ram, PR_U6A_025C_RAM_SIZE, &cpu) == PR_U6A_025C_OK);
    CHECK(cpu.ax == 0u && cpu.cx == 21u && (cpu.flags & 0x08D5u) == 0x0044u);

    before = cpu;
    CHECK(pr_u6a_record_remaining_025c(NULL, PR_U6A_025C_RAM_SIZE, &cpu) == PR_U6A_025C_ARGUMENT);
    CHECK(pr_u6a_record_remaining_025c(ram, 0u, &cpu) == PR_U6A_025C_ARGUMENT);
    CHECK(pr_u6a_record_remaining_025c(ram, PR_U6A_025C_RAM_SIZE, NULL) == PR_U6A_025C_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    free(ram);
    puts("u6A +025C record scan, signed selectors, flags, frame and guards passed");
    return 0;
}
