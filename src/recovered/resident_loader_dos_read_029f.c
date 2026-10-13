/* Resident cold-loader INT 21h file-read boundary at loaded +029F.
 * Evidence: docs/re/RESIDENT_LOADER_DOS_READ_029F.md. */
#include "poolrad/recovered/resident_loader_dos_read_029f.h"

static int valid_journal(const uint8_t *ram, size_t ram_size,
                         const pr_loader_dos_read_029f_write *writes,
                         size_t count)
{
    size_t i, j;
    for (i = 0; i < count; ++i) {
        uint8_t previous;
        if (writes[i].linear >= ram_size || writes[i].linear >= 0x100000u)
            return 0;
        previous = ram[writes[i].linear];
        for (j = 0; j < i; ++j)
            if (writes[j].linear == writes[i].linear)
                previous = writes[j].new_byte;
        if (previous != writes[i].old_byte)
            return 0;
    }
    return 1;
}

pr_loader_dos_read_029f_status pr_resident_loader_dos_read_029f(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu,
    pr_loader_dos_read_029f_service service, void *context,
    pr_loader_dos_read_029f_write *journal, size_t journal_capacity)
{
    pr_handler_02e6_cpu after;
    size_t count = 0, i;

    if (ram == NULL || ram_size != PR_HANDLER_02E6_RAM_SIZE || cpu == NULL ||
        (journal_capacity != 0u && journal == NULL) ||
        cpu->cs != 0x12C6u || cpu->ip != 0x029Fu ||
        (cpu->ax >> 8) != 0x3Fu)
        return PR_LOADER_DOS_READ_029F_ARGUMENT;
    if (service == NULL)
        return PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE;

    after = *cpu;
    if (service(context, cpu, ram, ram_size, &after, journal,
                journal_capacity, &count) != 0)
        return PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE;
    if (count > journal_capacity || count > PR_LOADER_DOS_READ_029F_MAX_WRITES ||
        after.cs != cpu->cs || after.ip != 0x02A1u ||
        after.ss != cpu->ss || after.sp != cpu->sp ||
        !valid_journal(ram, ram_size, journal, count))
        return PR_LOADER_DOS_READ_029F_BAD_EVENT;

    for (i = 0; i < count; ++i)
        ram[journal[i].linear] = journal[i].new_byte;
    *cpu = after;
    return PR_LOADER_DOS_READ_029F_POST_SERVICE;
}
