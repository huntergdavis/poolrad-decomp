#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_RECORD_SUBTRACT_0020_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_RECORD_SUBTRACT_0020_H
#include <stddef.h>
#include <stdint.h>
#define PR_U7F_0020_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_0020_cpu;
typedef enum { PR_U7F_0020_OK = 0, PR_U7F_0020_ARGUMENT } pr_u7f_0020_status;
/* Complete +0020..+0039, entered after CALL. Word args at SS:SP+4
 * (amount), +6 (record offset), +8 (record segment). Subtracts the amount
 * from the word at record+0102; RETF 6. Includes ordered stack/record
 * aliases, all CPU words and defined SUB16 flags under the immutable-code,
 * A20-off 1 MiB RAM contract. */
pr_u7f_0020_status pr_u7f_record_subtract_0020(
    uint8_t *ram, size_t ram_size, pr_u7f_0020_cpu *cpu);
#endif
