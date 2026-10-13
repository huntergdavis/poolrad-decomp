/* Composed saved resident cold-loader seek/read/return path.
 * Evidence: docs/re/RESIDENT_LOADER_COMPOSED_028F.md. */
#include "poolrad/recovered/resident_loader_composed_028f.h"

#include "poolrad/recovered/resident_loader_read_gate_0291.h"
#include "poolrad/recovered/resident_loader_read_branch_02a1.h"

static uint32_t physical(uint16_t segment, uint16_t offset)
{
    return (((uint32_t)segment << 4) + offset) & 0xFFFFFu;
}

static uint16_t stack_word(const uint8_t *ram, uint16_t segment,
                           uint16_t offset)
{
    uint32_t low = physical(segment, offset);
    uint32_t high = physical(segment, (uint16_t)(offset + 1u));
    return (uint16_t)(ram[low] | ((uint16_t)ram[high] << 8));
}

pr_loader_composed_028f_status pr_resident_loader_composed_028f(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu,
    const pr_loader_composed_028f_services *services)
{
    if (ram == NULL || ram_size != PR_HANDLER_02E6_RAM_SIZE ||
        cpu == NULL || services == NULL || cpu->cs != 0x12C6u)
        return PR_LOADER_COMPOSED_028F_ARGUMENT;

    for (;;) {
        switch (cpu->ip) {
        case 0x028Fu: {
            pr_loader_seek_028f_status status = pr_resident_loader_seek_028f(
                ram, ram_size, cpu, services->seek, services->seek_context,
                services->seek_journal, services->seek_capacity);
            if (status == PR_LOADER_SEEK_028F_BLOCKED_DOS_SERVICE)
                return PR_LOADER_COMPOSED_028F_NEED_SEEK;
            if (status == PR_LOADER_SEEK_028F_BAD_EVENT)
                return PR_LOADER_COMPOSED_028F_BAD_SEEK_EVENT;
            if (status != PR_LOADER_SEEK_028F_POST_SERVICE)
                return PR_LOADER_COMPOSED_028F_ARGUMENT;
            break;
        }
        case 0x0291u:
            if (pr_resident_loader_read_gate_0291(ram, ram_size, cpu) !=
                PR_LOADER_READ_GATE_0291_AT_DOS_READ)
                return PR_LOADER_COMPOSED_028F_ARGUMENT;
            break;
        case 0x029Fu: {
            pr_loader_dos_read_029f_status status =
                pr_resident_loader_dos_read_029f(
                    ram, ram_size, cpu, services->read, services->read_context,
                    services->read_journal, services->read_capacity);
            if (status == PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE)
                return PR_LOADER_COMPOSED_028F_NEED_FIRST_READ;
            if (status == PR_LOADER_DOS_READ_029F_BAD_EVENT)
                return PR_LOADER_COMPOSED_028F_BAD_READ_EVENT;
            if (status != PR_LOADER_DOS_READ_029F_POST_SERVICE)
                return PR_LOADER_COMPOSED_028F_ARGUMENT;
            break;
        }
        case 0x02A1u: {
            pr_loader_read_branch_02a1_status status =
                pr_resident_loader_read_branch_02a1(ram, ram_size, cpu);
            if (status == PR_LOADER_READ_BRANCH_02A1_AT_SECOND_DOS_READ)
                return PR_LOADER_COMPOSED_028F_NEED_SECOND_READ;
            if (status != PR_LOADER_READ_BRANCH_02A1_AT_EPILOGUE)
                return PR_LOADER_COMPOSED_028F_ARGUMENT;
            break;
        }
        case 0x02E1u: {
            uint16_t sp = cpu->sp;
            uint16_t restored_ds = stack_word(ram, cpu->ss, sp);
            uint16_t return_ip = stack_word(ram, cpu->ss,
                                            (uint16_t)(sp + 2u));
            uint16_t return_cs = stack_word(ram, cpu->ss,
                                            (uint16_t)(sp + 4u));
            cpu->ds = restored_ds; /* +02E1 POP DS */
            cpu->ip = return_ip;  /* +02E2 RETF 2 */
            cpu->cs = return_cs;
            cpu->sp = (uint16_t)(sp + 8u);
            return PR_LOADER_COMPOSED_028F_AT_CALLER;
        }
        default:
            return PR_LOADER_COMPOSED_028F_ARGUMENT;
        }
    }
}
