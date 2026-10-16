#include "poolrad/recovered/overlay_u3bd_in_grid_031f.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_MASK (PR_U3BD_031F_RAM_SIZE - 1u)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static void put_word(uint8_t *ram, uint16_t segment,
                     uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void check_case(uint8_t x, uint8_t y, uint16_t ss, uint16_t sp,
                       uint8_t result, uint16_t status_flags)
{
    uint8_t *ram = malloc(PR_U3BD_031F_RAM_SIZE);
    uint8_t *expected_ram = malloc(PR_U3BD_031F_RAM_SIZE);
    pr_u3bd_031f_cpu cpu = {
        .ax = 0xABCDu, .bx = 0x1245u, .cx = 0x9234u, .dx = 0xF011u,
        .si = 0x2244u, .di = 0x3366u, .bp = 0x3EB6u, .sp = sp,
        .cs = 0x2D14u, .ip = 0x031Fu, .ds = 0x14A0u,
        .es = 0x3DA1u, .ss = ss, .flags = 0xF202u
    };
    pr_u3bd_031f_cpu expected_cpu = cpu;
    uint16_t saved_bp_at = (uint16_t)(sp - 2u);
    assert(ram != NULL && expected_ram != NULL);
    memset(ram, 0xA5, PR_U3BD_031F_RAM_SIZE);
    put_word(ram, ss, sp, 0x0808u); /* return IP */
    put_word(ram, ss, (uint16_t)(sp + 2u), 0x2D14u); /* return CS */
    put_word(ram, ss, (uint16_t)(sp + 4u), (uint16_t)(0xAA00u | y));
    put_word(ram, ss, (uint16_t)(sp + 6u), (uint16_t)(0xBB00u | x));
    memcpy(expected_ram, ram, PR_U3BD_031F_RAM_SIZE);
    put_word(expected_ram, ss, saved_bp_at, cpu.bp);
    expected_ram[physical(ss, (uint16_t)(saved_bp_at - 1u))] = result;
    expected_cpu.ax = (uint16_t)((expected_cpu.ax & 0xFF00u) | result);
    expected_cpu.sp = (uint16_t)(sp + 8u);
    expected_cpu.ip = 0x0808u;
    expected_cpu.flags = (uint16_t)((expected_cpu.flags & (uint16_t)~0x08D5u) |
                                    status_flags);

    assert(pr_u3bd_in_grid_031f(ram, PR_U3BD_031F_RAM_SIZE, &cpu) ==
           PR_U3BD_031F_OK);
    assert(memcmp(ram, expected_ram, PR_U3BD_031F_RAM_SIZE) == 0);
    if (memcmp(&cpu, &expected_cpu, sizeof cpu) != 0) {
        fprintf(stderr, "x=%02X y=%02X ss=%04X sp=%04X: ", x, y, ss, sp);
        fprintf(stderr, "AX %04X/%04X BP %04X/%04X SP %04X/%04X ",
                cpu.ax, expected_cpu.ax, cpu.bp, expected_cpu.bp,
                cpu.sp, expected_cpu.sp);
        fprintf(stderr, "CS %04X/%04X IP %04X/%04X FLAGS %04X/%04X\n",
                cpu.cs, expected_cpu.cs, cpu.ip, expected_cpu.ip,
                cpu.flags, expected_cpu.flags);
        abort();
    }
    free(expected_ram);
    free(ram);
}

static void invalid_inputs_leave_state_untouched(void)
{
    uint8_t *ram = malloc(PR_U3BD_031F_RAM_SIZE);
    pr_u3bd_031f_cpu cpu = {.ax = 0x1234u, .sp = 0xFFFEu};
    pr_u3bd_031f_cpu before = cpu;
    assert(ram != NULL);
    memset(ram, 0xA5, PR_U3BD_031F_RAM_SIZE);
    assert(pr_u3bd_in_grid_031f(NULL, PR_U3BD_031F_RAM_SIZE, &cpu) ==
           PR_U3BD_031F_ARGUMENT);
    assert(pr_u3bd_in_grid_031f(ram, PR_U3BD_031F_RAM_SIZE, NULL) ==
           PR_U3BD_031F_ARGUMENT);
    assert(pr_u3bd_in_grid_031f(ram, PR_U3BD_031F_RAM_SIZE - 1u, &cpu) ==
           PR_U3BD_031F_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    assert(ram[0] == 0xA5u && ram[PR_U3BD_031F_RAM_SIZE - 1u] == 0xA5u);
    free(ram);
}

int main(void)
{
    /* Last CMP's fixed 8086 status flags: x or y gate, or accepted y>=0. */
    check_case(13u, 4u, 0x1D16u, 0x3E96u, 1u, 0x0000u);
    check_case(0u, 0u, 0x1D16u, 0x3E96u, 1u, 0x0044u);
    check_case(15u, 15u, 0x1D16u, 0x3E96u, 1u, 0x0004u);
    check_case(16u, 4u, 0x1D16u, 0x3E96u, 0u, 0x0010u);
    check_case(0xFFu, 4u, 0x1D16u, 0x3E96u, 0u, 0x0084u);
    check_case(0x80u, 4u, 0x1D16u, 0x3E96u, 0u, 0x0080u);
    check_case(4u, 16u, 0x1D16u, 0x3E96u, 0u, 0x0010u);
    check_case(4u, 0xFFu, 0x1D16u, 0x3E96u, 0u, 0x0084u);
    check_case(4u, 0x80u, 0x1D16u, 0x3E96u, 0u, 0x0080u);
    /* PUSH BP word at FFFF and RETF words crossing distinct offset wraps. */
    check_case(13u, 4u, 0x1D16u, 0x0001u, 1u, 0x0000u);
    check_case(0u, 0u, 0x1D16u, 0xFFFFu, 1u, 0x0044u);
    check_case(13u, 4u, 0xFFFFu, 0xFFF9u, 1u, 0x0000u);
    invalid_inputs_leave_state_untouched();
    puts("U3BD +031F complete CPU/RAM tests passed");
    return 0;
}
