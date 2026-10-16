#include "poolrad/recovered/overlay_u3c6_table_pair_0000.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t ram[PR_U6C_3F31_RAM_SIZE], want[PR_U6C_3F31_RAM_SIZE];
static unsigned cases;
_Static_assert(sizeof(pr_u3c6_0000_cpu) == 28, "fourteen CPU words");
enum fixture { ZERO, POSITIVE, NEGATIVE, FIRST_TO_X, FIRST_TO_Y,
    FIRST_TO_TABLE, SECOND_TO_LOCAL, SECOND_TO_POINTER_OFFSET,
    SECOND_TO_POINTER_SEGMENT, SECOND_TO_SAVED_BP, SECOND_TO_IP,
    SECOND_TO_CS, FIXTURE_COUNT };

static size_t addr(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment * 16u + offset) % sizeof(ram);
}
static void put_word(uint8_t *memory, size_t at, uint16_t value)
{
    memory[at] = (uint8_t)value;
    memory[(at + 1u) % sizeof(ram)] = (uint8_t)(value / 256u);
}
static void put_far(size_t at, uint16_t offset, uint16_t segment)
{
    put_word(ram, at, offset);
    put_word(ram, (at + 2u) % sizeof(ram), segment);
}
static uint16_t cmp_flags(uint8_t byte)
{
    unsigned i, ones = 0;
    uint16_t flags = 0;
    for (i = 0; i < 8; ++i) ones += (byte >> i) & 1u;
    if (ones % 2u == 0) flags += 4u;
    if (byte == 0) flags += 0x40u;
    if (byte >= 128u) flags += 0x80u;
    return flags;
}
static void verify(pr_u3c6_0000_cpu *cpu, pr_u3c6_0000_cpu expected,
    uint8_t *memory, size_t size, pr_u3c6_0000_retf_policy policy,
    pr_u3c6_0000_status status, unsigned fixture)
{
    pr_u3c6_0000_status got;
    ++cases;
    got = pr_u3c6_table_pair_0000(memory, size, cpu, policy);
    if (got != status || (cpu && memcmp(cpu, &expected, sizeof(expected))) ||
        memcmp(ram, want, sizeof(ram))) {
        fprintf(stderr, "case%u fixture%u policy%d status%d expected%d failed\n",
            cases, fixture, (int)policy, (int)got, (int)status);
        if (cpu) {
            const unsigned char *a = (const unsigned char *)cpu;
            const unsigned char *b = (const unsigned char *)&expected;
            size_t i;
            for (i = 0; i < sizeof(expected); ++i)
                if (a[i] != b[i]) fprintf(stderr,"CPU byte%zu %02X != %02X\n",i,a[i],b[i]);
        }
        exit(1);
    }
}
static void fixture(enum fixture kind, uint16_t stack, uint16_t ss,
    pr_u3c6_0000_retf_policy policy, uint16_t flags)
{
    pr_u3c6_0000_cpu cpu = {0xABCD,0x1234,0x5678,0x9ABC,0x1357,0x2468,
        0xBEEF,stack,0x4321,0,0x1000,0x5555,ss,flags};
    pr_u3c6_0000_cpu expected = cpu;
    uint16_t first_offset = 0x1000, first_segment = 0x2000;
    uint16_t second_offset = 0x1100, second_segment = 0x2000;
    uint16_t final_offset = first_offset, final_segment = first_segment;
    uint16_t popped_bp = 0xBEEF, return_ip = 0x4567, return_cs = 0x1234;
    uint16_t final_dx = 6, second_index = 22;
    uint8_t first = 0x7F, second = 0x52, readback = 0x7F, local = 1;
    size_t first_at, second_at;

    /* Expectations are fixture constants, not an instruction interpreter.
     * Each scenario fixes the changed second lookup and final readback. */
    switch (kind) {
    case ZERO: local = 0; readback = 0; break;
    case POSITIVE: break;
    case NEGATIVE: first = readback = 0x80; local = 0; break;
    case FIRST_TO_X:
        first_segment = ss; first_offset = (uint16_t)(stack + 12u);
        first = readback = 9; final_dx = 18; second_index = 34; break;
    case FIRST_TO_Y:
        first_segment = ss; first_offset = (uint16_t)(stack + 14u);
        first = readback = 0; second_index = 6; second = 0x44; break;
    case FIRST_TO_TABLE:
        first_segment = cpu.ds; first_offset = 0x2845u + 22u;
        first = second = readback = 0x91; local = 0; break;
    case SECOND_TO_LOCAL:
        second_segment = ss; second_offset = (uint16_t)(stack - 3u);
        first = readback = 0x80; second = local = 0xD7; break;
    case SECOND_TO_POINTER_OFFSET:
        second_segment = ss; second_offset = (uint16_t)(stack + 8u);
        second = 0x20; readback = 0x80; local = 0; break;
    case SECOND_TO_POINTER_SEGMENT:
        second_segment = ss; second_offset = (uint16_t)(stack + 10u);
        second = 0x21; readback = 0; break;
    case SECOND_TO_SAVED_BP:
        second_segment = ss; second_offset = (uint16_t)(stack - 2u);
        second = 0x34; popped_bp = 0xBE34; break;
    case SECOND_TO_IP:
        second_segment = ss; second_offset = stack;
        second = 0x56; return_ip = 0x4556; break;
    case SECOND_TO_CS:
        second_segment = ss; second_offset = (uint16_t)(stack + 3u);
        second = 0x77; return_cs = 0x7734; break;
    case FIXTURE_COUNT: abort();
    }
    final_offset = first_offset; final_segment = first_segment;
    if (kind == SECOND_TO_POINTER_OFFSET) final_offset = 0x1020;
    if (kind == SECOND_TO_POINTER_SEGMENT) final_segment = 0x2021;
    memset(ram, 0xA5, sizeof(ram));
    put_word(ram, addr(ss, stack), 0x4567);
    put_word(ram, addr(ss, (uint16_t)(stack + 2u)), 0x1234);
    if (stack >= 0xFFFEu)
        put_word(ram, (addr(ss, stack) + 2u) % sizeof(ram), 0x9ABC);
    put_far(addr(ss, (uint16_t)(stack + 4u)), second_offset, second_segment);
    put_far(addr(ss, (uint16_t)(stack + 8u)), first_offset, first_segment);
    ram[addr(ss, (uint16_t)(stack + 12u))] = 3;
    ram[addr(ss, (uint16_t)(stack + 14u))] = kind == ZERO ? 0 : 2;
    ram[addr(cpu.ds, 0x2844u + 22u)] = first;
    ram[addr(cpu.ds, (uint16_t)(0x2845u + second_index))] = second;
    if (kind == FIRST_TO_TABLE) ram[addr(cpu.ds, 0x2845u + 22u)] = 0x52;
    if (kind == SECOND_TO_POINTER_OFFSET || kind == SECOND_TO_POINTER_SEGMENT)
        ram[addr(final_segment, final_offset)] = readback;
    memcpy(want, ram, sizeof(ram));

    put_word(want, addr(ss, (uint16_t)(stack - 2u)), cpu.bp);
    want[addr(ss, (uint16_t)(stack - 3u))] = 0;
    first_at = addr(first_segment, first_offset);
    second_at = addr(second_segment, second_offset);
    if (kind != ZERO) {
        want[first_at] = first;
        want[second_at] = second;
        if (readback < 128u) want[addr(ss, (uint16_t)(stack - 3u))] = 1;
        expected.dx = final_dx;
        expected.cx = 0x5603;
        expected.di = final_offset;
        expected.es = final_segment;
        expected.ax = local;
    } else {
        expected.ax = 0xAB00;
    }
    expected.bp = popped_bp;
    expected.flags = (uint16_t)((flags & ~0x08D5u) | cmp_flags(readback));
    if (policy == PR_U3C6_0000_RETF_UNMEASURED) {
        expected.sp = stack; expected.ip = 0x0065;
    } else {
        expected.sp = (uint16_t)(stack + 16u);
        expected.ip = return_ip;
        expected.cs = stack >= 0xFFFEu &&
            policy == PR_U3C6_0000_RETF_CONTIGUOUS_PHYSICAL ? 0x9ABC : return_cs;
    }
    verify(&cpu,expected,ram,sizeof(ram),policy,
        policy == PR_U3C6_0000_RETF_UNMEASURED ?
            PR_U3C6_0000_RETURN_PENDING : PR_U3C6_0000_RETURNED,kind);
}
int main(void)
{
    const uint16_t stacks[] = {0xFFF7,0xFFFD,0xFFFE,0xFFFF};
    unsigned k,p,f,i;
    pr_u3c6_0000_cpu cpu,expected;
    for (f=0;f<2;++f) for(p=0;p<3;++p) {
        for(k=0;k<FIXTURE_COUNT;++k)
            fixture((enum fixture)k,0x9000,0x3000,(pr_u3c6_0000_retf_policy)p,f?0xFFFF:0);
        for(i=0;i<4;++i) for(k=ZERO;k<=NEGATIVE;++k)
            fixture((enum fixture)k,stacks[i],0x3000,(pr_u3c6_0000_retf_policy)p,f?0xFFFF:0);
        for(k=ZERO;k<=NEGATIVE;++k)
            fixture((enum fixture)k,0x0011,0xFFFF,(pr_u3c6_0000_retf_policy)p,f?0xFFFF:0);
    }
    memset(ram,0xA5,sizeof(ram));memcpy(want,ram,sizeof(ram));
    memset(&cpu,0,sizeof(cpu));expected=cpu;
    verify(&cpu,expected,NULL,sizeof(ram),PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    verify(NULL,expected,ram,sizeof(ram),PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    verify(&cpu,expected,ram,0,PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    verify(&cpu,expected,ram,sizeof(ram)-1u,PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    verify(&cpu,expected,ram,sizeof(ram)+1u,PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    verify(&cpu,expected,ram,sizeof(ram),(pr_u3c6_0000_retf_policy)3,PR_U3C6_0000_ARGUMENT,99);
    cpu.ip=1;expected=cpu;
    verify(&cpu,expected,ram,sizeof(ram),PR_U3C6_0000_RETF_UNMEASURED,PR_U3C6_0000_ARGUMENT,99);
    printf("PASS %u full1MiB/all14CPU synthetic cases; explicit RETF candidates, native unmeasured\n",cases);
    return 0;
}
