#include "poolrad/recovered/overlay_u6c_record_arithmetic_42ed.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6C_42ED_RAM_SIZE - 1u);
}
static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6C_42ED_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}
static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & (PR_U6C_42ED_RAM_SIZE - 1u)] << 8u));
}
static const uint16_t denominations[5] = {1,10,20,100,200};
static pr_u6c_42ed_cpu prepare(uint8_t *ram, uint32_t amount)
{
    unsigned i;
    pr_u6c_42ed_cpu cpu = {
        0xABCDu,0x3456u,0x5678u,0x789Au,0x9ABCu,0xBCDEu,
        0x7777u,0x8000u,0xF000u,0x42EDu,0x1000u,0x1111u,0x2000u,0x0ED7u
    };
    memset(ram, 0xA5, PR_U6C_42ED_RAM_SIZE);
    for (i = 0; i < 5; ++i) {
        put16(ram, cpu.ds, (uint16_t)(0xD24u + 4u*i), denominations[i]);
        put16(ram, cpu.ds, (uint16_t)(0xD26u + 4u*i), 0);
        put16(ram, 0x4000u, (uint16_t)(0x1088u + 2u*i), 10000);
    }
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, 0x1000u);
    put16(ram, cpu.ss, 0x8006u, 0x4000u);
    put16(ram, cpu.ss, 0x8008u, (uint16_t)amount);
    put16(ram, cpu.ss, 0x800Au, (uint16_t)(amount >> 16u));
    return cpu;
}
int main(void)
{
    uint8_t *ram = malloc(PR_U6C_42ED_RAM_SIZE);
    const pr_u6c_42ed_layout layout = {0xD000u};
    pr_u6c_42ed_cpu cpu;
    int amount;
    unsigned i;
    assert(ram);
    cpu = prepare(ram, 1);
    assert(pr_u6c_record_arithmetic_42ed(NULL, PR_U6C_42ED_RAM_SIZE, &cpu, &layout) == PR_U6C_42ED_ARGUMENT);
    assert(pr_u6c_record_arithmetic_42ed(ram, 1, &cpu, &layout) == PR_U6C_42ED_ARGUMENT);
    assert(pr_u6c_record_arithmetic_42ed(ram, PR_U6C_42ED_RAM_SIZE, NULL, &layout) == PR_U6C_42ED_ARGUMENT);
    assert(pr_u6c_record_arithmetic_42ed(ram, PR_U6C_42ED_RAM_SIZE, &cpu, NULL) == PR_U6C_42ED_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    for (amount = -1000; amount <= 1000; amount += 10) {
        int64_t before = 0, after = 0;
        cpu = prepare(ram, (uint32_t)amount);
        for (i = 0; i < 5; ++i) before += (int64_t)10000 * denominations[i];
        assert(pr_u6c_record_arithmetic_42ed(ram, PR_U6C_42ED_RAM_SIZE, &cpu, &layout) == PR_U6C_42ED_OK);
        for (i = 0; i < 5; ++i) after += (int64_t)get16(ram, 0x4000u, (uint16_t)(0x1088u + 2u*i)) * denominations[i];
        assert(after == before - (int64_t)amount * 200);
        assert(cpu.sp == 0x800Cu && cpu.bp == 0x7777u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
        assert(cpu.flags == 0x0646u && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
        assert(get16(ram, cpu.ss, 0x8008u) == 0 && get16(ram, cpu.ss, 0x800Au) == 0);
    }
    /* Scaled amount wraps to INT32_MIN; abs leaves it negative and the
     * second loop exits immediately, retaining the source's final CMP. */
    cpu = prepare(ram, 0x10000000u);
    assert(pr_u6c_record_arithmetic_42ed(ram, PR_U6C_42ED_RAM_SIZE, &cpu, &layout) == PR_U6C_42ED_OK);
    assert(cpu.ax == 0 && cpu.dx == 0x8000u && cpu.flags == 0x0686u);
    for (i = 0; i < 5; ++i) assert(get16(ram, 0x4000u, (uint16_t)(0x1088u + 2u*i)) == 10000u);
    cpu = prepare(ram, 1);
    put16(ram, cpu.ds, 0xD24u, 0);
    assert(pr_u6c_record_arithmetic_42ed(ram, PR_U6C_42ED_RAM_SIZE, &cpu, &layout) == PR_U6C_42ED_UNSUPPORTED_RUNTIME);
    assert(cpu.cs == layout.system_cs && cpu.ip == 0x00E2u && cpu.ax == 0x00C8u);
    assert(cpu.bp == 0x7FFEu && cpu.sp == 0x7FF2u && cpu.flags == 0x0646u);
    free(ram);
    return 0;
}
