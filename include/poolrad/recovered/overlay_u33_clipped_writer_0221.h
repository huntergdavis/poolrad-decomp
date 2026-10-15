#ifndef POOLRAD_RECOVERED_OVERLAY_U33_CLIPPED_WRITER_0221_H
#define POOLRAD_RECOVERED_OVERLAY_U33_CLIPPED_WRITER_0221_H
#include <stddef.h>
#include <stdint.h>
#define PR_U33_0221_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0221_cpu;
typedef enum { PR_U33_0221_OK = 0, PR_U33_0221_ARGUMENT } pr_u33_0221_status;
/* Complete +0221..+02B9, entered after CALL. Word args at SS:SP+4
 * (inherited frame offset), +6 (glyph), +8 (row delta), +10 (column delta).
 * Only low argument bytes matter. Signed frame bytes at frame-2/-1 form
 * the coordinates. Writes glyph+1 through DS:6716/6718 only inside 50x25.
 * RETF 8; preserves ordered aliases, all CPU words and defined flags under
 * the immutable-code, A20-off 1 MiB RAM contract. */
pr_u33_0221_status pr_u33_clipped_writer_0221(
    uint8_t *ram, size_t ram_size, pr_u33_0221_cpu *cpu);
#endif
