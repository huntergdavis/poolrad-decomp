#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U7F_003A_RAM_SIZE - 1u);
}
static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U7F_003A_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}
static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U7F_003A_RAM_SIZE - 1u)] << 8u));
}
static pr_u7f_003a_cpu prepare(uint8_t *ram, uint16_t value, uint16_t increment)
{
    pr_u7f_003a_cpu cpu = {
        0xABCDu,0x3456u,0x5678u,0x789Au,0x9ABCu,0xBCDEu,
        0x7777u,0x8000u,0xF000u,0x003Au,0x1000u,0x1111u,0x2000u,0x0ED7u
    };
    memset(ram, 0xA5, PR_U7F_003A_RAM_SIZE);
    put16(ram, 0x4000u, 0x1102u, value);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, increment);
    put16(ram, cpu.ss, 0x8006u, 0x1000u);
    put16(ram, cpu.ss, 0x8008u, 0x4000u);
    return cpu;
}
int main(void)
{
    uint8_t *ram = malloc(PR_U7F_003A_RAM_SIZE);
    pr_u7f_003a_cpu cpu;
    unsigned i;
    const uint16_t left[] = {2,0xFFFFu,0x7FFFu,0x8000u};
    const uint16_t right[] = {3,1,1,0x8000u};
    const uint16_t expected_flags[] = {0x0606u,0x0657u,0x0E96u,0x0E47u};
    assert(ram);
    cpu = prepare(ram, 2, 3);
    assert(pr_u7f_record_word_add_003a(NULL, PR_U7F_003A_RAM_SIZE, &cpu) == PR_U7F_003A_ARGUMENT);
    assert(pr_u7f_record_word_add_003a(ram, 1, &cpu) == PR_U7F_003A_ARGUMENT);
    assert(pr_u7f_record_word_add_003a(ram, PR_U7F_003A_RAM_SIZE, NULL) == PR_U7F_003A_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    for (i = 0; i < 4; ++i) {
        uint16_t sum = (uint16_t)(left[i] + right[i]);
        cpu = prepare(ram, left[i], right[i]);
        assert(pr_u7f_record_word_add_003a(ram, PR_U7F_003A_RAM_SIZE, &cpu) == PR_U7F_003A_OK);
        assert(cpu.ax == sum && get16(ram, 0x4000u, 0x1102u) == sum && cpu.flags == expected_flags[i]);
        assert(cpu.di == 0x1000u && cpu.es == 0x4000u && cpu.bp == 0x7777u && cpu.sp == 0x800Au);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u);
        assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
        assert(cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    }
    /* The pushed BP becomes the record word and is changed before POP BP. */
    cpu = prepare(ram, 0, 1);
    put16(ram, cpu.ss, 0x8006u, 0x7EFCu);
    put16(ram, cpu.ss, 0x8008u, cpu.ss);
    assert(pr_u7f_record_word_add_003a(ram, PR_U7F_003A_RAM_SIZE, &cpu) == PR_U7F_003A_OK);
    assert(cpu.ax == 0x7778u && cpu.bp == 0x7778u && cpu.sp == 0x800Au);
    /* Store aliases the far return IP, then separately the return CS. */
    for (i = 0; i < 2; ++i) {
        cpu = prepare(ram, 0, 1);
        put16(ram, cpu.ss, 0x8006u, (uint16_t)(0x7EFEu + 2u*i));
        put16(ram, cpu.ss, 0x8008u, cpu.ss);
        assert(pr_u7f_record_word_add_003a(ram, PR_U7F_003A_RAM_SIZE, &cpu) == PR_U7F_003A_OK);
        assert(cpu.ip == (i ? 0x0712u : 0x0713u) && cpu.cs == (i ? 0xF001u : 0xF000u));
    }
    free(ram);
    return 0;
}
