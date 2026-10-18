/* Focused synthetic test for the raw-CPU resident 0709:00A0 EGA page select (docs/re/RES_0709_PAGE_SELECT_00A0_CPU.md).
 * No game bytes. */
#include "poolrad/recovered/resident_0709_page_select_00a0_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "page select line %d: %s\n", __LINE__, #x);           \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { RAM = 1u << 20, DS0 = 0x1000, SS0 = 0x2000, SP0 = 0x8000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(uint16_t segment, uint16_t offset) { return (((size_t)segment << 4) + offset) & (RAM - 1u); }
static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[at(segment, offset)] = (uint8_t)value;
    ram[at(segment, (uint16_t)(offset + 1u))] = (uint8_t)(value >> 8);
}
static uint16_t get16(uint16_t segment, uint16_t offset) { return (uint16_t)(ram[at(segment, offset)] | (ram[at(segment, (uint16_t)(offset + 1u))] << 8)); }

static pr_r0709_page_cpu frame(uint16_t page_word, uint16_t ds, uint16_t ss, uint16_t sp)
{
    pr_r0709_page_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, sp, 0x0709u, 0x00A0u, ds, 0x1111u, ss, 0x7202u};
    memset(ram, 0xA5, RAM);
    put16(ss, sp, 0x7000u); put16(ss, (uint16_t)(sp + 2u), 0x7001u); put16(ss, (uint16_t)(sp + 4u), page_word);
    return cpu;
}

int main(void)
{
    pr_r0709_page_cpu cpu;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* Page 1 (high byte ignored): A200h; ADD 0200h,A000h sets SF and PF only. */
    cpu = frame(0xEE01u, DS0, SS0, SP0);
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM, &cpu) == PR_R0709_PAGE_OK);
    CHECK(get16(DS0, 0x4A18u) == 0xA200u && cpu.ax == 0xA200u && cpu.cx == 9u && cpu.flags == 0x7286u);
    CHECK(cpu.cs == 0x7001u && cpu.ip == 0x7000u && cpu.sp == SP0 + 6u && cpu.bp == 0x7777u && get16(SS0, SP0 - 2u) == 0x7777u);
    CHECK(cpu.bx == 0x3456u && cpu.dx == 0x789Au && cpu.ds == DS0 && cpu.es == 0x1111u);

    /* Page 7Fh: FE00h + A000h carries to 9E00h (CF, AF clear, PF from 00h). Page 80h wraps the shift to A000h. */
    cpu = frame(0x007Fu, DS0, SS0, SP0);
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM, &cpu) == PR_R0709_PAGE_OK && cpu.ax == 0x9E00u && cpu.flags == 0x7287u);
    cpu = frame(0x0080u, DS0, SS0, SP0);
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM, &cpu) == PR_R0709_PAGE_OK && cpu.ax == 0xA000u && get16(DS0, 0x4A18u) == 0xA000u);

    /* DS:[4A18h] on the return IP (SS:SP0 = DS:4A18h with DS = SS = 2000h - 0, SP0 = 4A18h): the RETF pops the stored word. */
    cpu = frame(0x0002u, 0x2000u, 0x2000u, 0x4A18u);
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM, &cpu) == PR_R0709_PAGE_OK && cpu.ip == 0xA400u && cpu.cs == 0x7001u);

    /* Guards change nothing. */
    cpu = frame(1u, DS0, SS0, SP0);
    cpu.ip = 0x00A1u;
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM, &cpu) == PR_R0709_PAGE_ARGUMENT && cpu.sp == SP0 && get16(DS0, 0x4A18u) == 0xA5A5u);
    CHECK(pr_r0709_page_select_00a0_cpu(NULL, RAM, &cpu) == PR_R0709_PAGE_ARGUMENT);
    CHECK(pr_r0709_page_select_00a0_cpu(ram, RAM - 1u, &cpu) == PR_R0709_PAGE_ARGUMENT);

    free(ram);
    printf("page select 00a0: %u checks passed\n", checks);
    return 0;
}
