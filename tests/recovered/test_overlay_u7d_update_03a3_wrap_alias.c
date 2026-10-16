#include "poolrad/recovered/overlay_u7d_update_03a3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE PR_U7D_03A3_RAM_SIZE
#define MASK (RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static pr_u7d_03a3_cpu frame(uint8_t *ram, uint16_t decoy)
{
    pr_u7d_03a3_cpu cpu = {
        0x1234u, 0x5678u, 0x9ABCu, 0xDEF0u, 0x1111u, 0x2222u,
        3u, 0xFFFEu, 0x3000u, 0x03A3u, 0x2923u, 0x4000u,
        0x2000u, 0x7202u
    };
    unsigned i;
    size_t linear_cs;
    memset(ram, 0, RAM_SIZE);
    put16(ram, cpu.ds, 0x5D96u, 0u);
    put16(ram, cpu.ds, 0x5D98u, 0u);
    for (i = 0u; i < 7u; ++i) {
        put16(ram, cpu.ds, (uint16_t)(0x363Au + 2u * i), 0xFFFFu);
        put16(ram, cpu.ds, (uint16_t)(0x6DC2u + 2u * i), 0u);
    }
    put16(ram, cpu.ds, 0x3642u, 4u);
    put16(ram, cpu.ds, 0x6DCAu, 90u);
    /* U7D +03A3 saves BP at SS:FFFC, the same physical DS:6DCC word.
     * The later pending-count clear must change the BP restored by POP. */
    assert(physical(cpu.ds, 0x6DCCu) == physical(cpu.ss, 0xFFFCu));
    put16(ram, cpu.ss, 0xFFFEu, 0xCAFEu);
    put16(ram, cpu.ss, 0x0000u, 0xBEEFu);
    linear_cs = (physical(cpu.ss, 0xFFFEu) + 2u) & MASK;
    ram[linear_cs] = (uint8_t)decoy;
    ram[(linear_cs + 1u) & MASK] = (uint8_t)(decoy >> 8);
    return cpu;
}

static pr_u7d_03a3_cpu frame_cs_alias(uint8_t *ram, uint16_t decoy)
{
    pr_u7d_03a3_cpu cpu = {
        0x1234u, 0x5678u, 0x9ABCu, 0xDEF0u, 0x1111u, 0x2222u,
        3u, 0xFFFFu, 0x3000u, 0x03A3u, 0x1924u, 0x4000u,
        0x2000u, 0x7202u
    };
    unsigned i;
    size_t linear_cs;
    memset(ram, 0, RAM_SIZE);
    put16(ram, cpu.ds, 0x5D96u, 0u);
    put16(ram, cpu.ds, 0x5D98u, 0u);
    for (i = 0u; i < 7u; ++i) {
        put16(ram, cpu.ds, (uint16_t)(0x363Au + 2u * i), 0xFFFFu);
        put16(ram, cpu.ds, (uint16_t)(0x6DC2u + 2u * i), 0u);
    }
    put16(ram, cpu.ds, 0x363Au, 1u);
    put16(ram, cpu.ds, 0x6DC2u, 2u);
    /* Array[0] low byte is the outer CS high byte at SS:0002.
     * A carry from 2 to 1 must change the far return from 02EF to 01EF. */
    assert(physical(cpu.ds, 0x6DC2u) == physical(cpu.ss, 0x0002u));
    put16(ram, cpu.ss, 0xFFFFu, 0xCAFEu);
    ram[physical(cpu.ss, 0x0001u)] = 0xEFu;
    assert(get16(ram, cpu.ss, 0x0001u) == 0x02EFu);
    linear_cs = (physical(cpu.ss, 0xFFFFu) + 2u) & MASK;
    ram[linear_cs] = (uint8_t)decoy;
    ram[(linear_cs + 1u) & MASK] = (uint8_t)(decoy >> 8);
    return cpu;
}

int main(void)
{
    uint8_t *a = malloc(RAM_SIZE);
    uint8_t *b = malloc(RAM_SIZE);
    uint8_t *before = malloc(RAM_SIZE);
    pr_u7d_03a3_cpu cpu_a, cpu_b;
    size_t linear_cs;
    size_t i;
    assert(a != NULL && b != NULL && before != NULL);

    cpu_a = frame(a, 0xDEADu);
    cpu_b = frame(b, 0x1234u);
    linear_cs = (physical(cpu_a.ss, 0xFFFEu) + 2u) & MASK;
    memcpy(before, a, RAM_SIZE);
    assert(pr_u7d_update_03a3(a, RAM_SIZE, &cpu_a) == PR_U7D_03A3_OK);
    assert(pr_u7d_update_03a3(b, RAM_SIZE, &cpu_b) == PR_U7D_03A3_OK);

    /* The wrapped CS is at SS:0000; the linear decoy cannot affect any
     * of the 14 returned CPU words or any other RAM byte. */
    assert(memcmp(&cpu_a, &cpu_b, sizeof cpu_a) == 0);
    assert(cpu_a.ip == 0xCAFEu && cpu_a.cs == 0xBEEFu && cpu_a.sp == 2u);
    assert(cpu_a.bp == 0u && cpu_a.ds == 0x2923u && cpu_a.ss == 0x2000u);
    assert(get16(a, cpu_a.ds, 0x6DCCu) == 0u);
    assert(get16(a, cpu_a.ds, 0x6DCAu) == 99u);
    assert(get16(a, cpu_a.ss, 0x0000u) == 0xBEEFu);
    for (i = 0u; i < RAM_SIZE; ++i) {
        if (i == linear_cs || i == ((linear_cs + 1u) & MASK)) continue;
        assert(a[i] == b[i]);
        /* This fixture has no party traversal or carry. Its writes stay
         * in the original stack/array overlap SS:FFEC..FFFF. */
        if (i < physical(cpu_a.ss, 0xFFECu) ||
            i > physical(cpu_a.ss, 0xFFFFu))
            assert(a[i] == before[i]);
    }

    cpu_a = frame_cs_alias(a, 0xDEADu);
    cpu_b = frame_cs_alias(b, 0x1234u);
    linear_cs = (physical(cpu_a.ss, 0xFFFFu) + 2u) & MASK;
    memcpy(before, a, RAM_SIZE);
    assert(pr_u7d_update_03a3(a, RAM_SIZE, &cpu_a) == PR_U7D_03A3_OK);
    assert(pr_u7d_update_03a3(b, RAM_SIZE, &cpu_b) == PR_U7D_03A3_OK);
    assert(memcmp(&cpu_a, &cpu_b, sizeof cpu_a) == 0);
    assert(cpu_a.ip == 0xCAFEu && cpu_a.cs == 0x01EFu && cpu_a.sp == 3u);
    assert(cpu_a.bp == 3u && cpu_a.ds == 0x1924u && cpu_a.ss == 0x2000u);
    assert(get16(a, cpu_a.ds, 0x6DC2u) == 1u);
    assert(get16(a, cpu_a.ds, 0x6DC4u) == 1u);
    assert(get16(a, cpu_a.ss, 0x0001u) == 0x01EFu);
    for (i = 0u; i < RAM_SIZE; ++i) {
        int stack = i >= physical(cpu_a.ss, 0xFFEDu) &&
                    i <= physical(cpu_a.ss, 0xFFFFu);
        int array = i >= physical(cpu_a.ss, 0x0002u) &&
                    i <= physical(cpu_a.ss, 0x0005u);
        if (i == linear_cs || i == ((linear_cs + 1u) & MASK)) continue;
        assert(a[i] == b[i]);
        if (!stack && !array) assert(a[i] == before[i]);
    }
    free(before);
    free(b);
    free(a);
    return 0;
}
