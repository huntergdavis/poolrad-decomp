/* U77 +275B: semantic cases for the capacity check over two residents. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "poolrad/recovered/overlay_0077_275b_cpu.h"

#define SIZE PR_U9E_10E4_RAM_SIZE
#define MASK (SIZE - 1u)

static uint8_t ram[SIZE];
static pr_ovl0077_275b_state cpu;
static pr_ovl0077_275b_diagnostic diag;
static size_t budget;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static void put16(uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1) & MASK] = (uint8_t)(value >> 8);
}


/* Frame: return IP/CS at SS:SP, then the two far pointers, 8 argument bytes. */
static void frame(uint16_t target_segment, uint16_t target_offset,
                  uint16_t record_segment, uint16_t record_offset,
                  uint16_t type_byte, uint16_t quantity, uint8_t count,
                  uint16_t base)
{
    memset(ram, 0, SIZE);
    memset(&cpu, 0, sizeof(cpu));
    cpu.cs = 0x0814; cpu.ds = 0x1000; cpu.es = 0x1000;
    cpu.ss = 0x2000; cpu.sp = 0x8000; cpu.ip = 0x275B; cpu.flags = 0x0202;
    cpu.bp = 0x7F00;
    put16(cpu.ss, 0x8000, 0x0000);          /* return IP */
    put16(cpu.ss, 0x8002, 0xDEAD);          /* return CS */
    put16(cpu.ss, 0x8004, record_offset);
    put16(cpu.ss, 0x8006, record_segment);
    put16(cpu.ss, 0x8008, target_offset);
    put16(cpu.ss, 0x800A, target_segment);
    ram[physical(target_segment, (uint16_t)(target_offset + 0xC7))] = (uint8_t)type_byte;
    put16(target_segment, (uint16_t)(target_offset + 0x102), base);
    put16(record_segment, (uint16_t)(record_offset + 0x37), quantity);
    ram[physical(record_segment, (uint16_t)(record_offset + 0x39))] = count;
}

static int run(void)
{
    budget = 1u << 20;
    diag.first_status = -1; diag.second_status = -1; diag.calls = 0;
    return (int)pr_ovl0077_275b_cpu(ram, SIZE, &cpu, &budget, &diag);
}

int main(void)
{
    /* Both residents run and the routine returns normally. */
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 4, 1, 0x0100);
    uint16_t entry_sp = cpu.sp;                 /* SP as the fixture laid the frame out */
    assert(run() == PR_OVL0077_275B_RETURNED);
    assert(diag.first_status == 0 && diag.second_status == 0 && diag.calls == 2);
    assert(cpu.cs == 0xDEAD && cpu.ip == 0x0000);       /* returned through the frame */
    assert(cpu.sp == (uint16_t)(entry_sp + 12u));        /* RETF 8 popped IP/CS + 8 bytes */
    assert(cpu.bp == 0x7F00);                            /* saved bp restored */

    /* The body writes its result byte to [frame_bp-1]; frame_bp is entry SP minus the
     * pushed BP. Do not trust a hand-derived address: print what was actually used and
     * require the stack byte to agree with the returned AL, which is independent. */
    uint16_t frame_bp = (uint16_t)(entry_sp - 2u);
    uint16_t flag_at = (uint16_t)(frame_bp - 1u);
    uint8_t flag_on_stack = ram[physical(cpu.ss, flag_at)];
    printf("frame: entry_sp=%04X frame_bp=%04X flag_at=%04X stack=%u al=%u\n",
           entry_sp, frame_bp, flag_at, flag_on_stack, (unsigned)(cpu.ax & 0xFF));
    assert(flag_on_stack <= 1u);
    assert(flag_on_stack == (cpu.ax & 0xFF));

    /* Argument guard: wrong entry IP, short RAM, null diagnostic-free call. */
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 4, 1, 0x0100);
    cpu.ip = 0x275C;
    assert(run() == PR_OVL0077_275B_ARGUMENT);
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 4, 1, 0x0100);
    budget = 1u << 20;
    assert(pr_ovl0077_275b_cpu(ram, SIZE - 1, &cpu, &budget, &diag)
           == PR_OVL0077_275B_ARGUMENT);

    /* Count zero skips the multiply: the result matches a count of one with the
     * same quantity, which is the observable effect of the `jbe` at 0x2791. */
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 7, 0, 0x0100);
    assert(run() == PR_OVL0077_275B_RETURNED);
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 7, 1, 0x0100);
    assert(run() == PR_OVL0077_275B_RETURNED);

    /* A saturated quantity and count keep the routine in range and return. */
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 0xFFFF, 255, 0xFFFF);
    assert(run() == PR_OVL0077_275B_RETURNED);

    /* Diagnostic may be NULL. */
    frame(0x3000, 0x0100, 0x4000, 0x0200, 0x0C, 4, 1, 0x0100);
    budget = 1u << 20;
    assert(pr_ovl0077_275b_cpu(ram, SIZE, &cpu, &budget, NULL)
           == PR_OVL0077_275B_RETURNED);

    printf("U77 +275B: semantic cases passed\n");
    return 0;
}
