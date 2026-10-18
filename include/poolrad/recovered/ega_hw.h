#ifndef POOLRAD_RECOVERED_EGA_HW_H
#define POOLRAD_RECOVERED_EGA_HW_H

#include <stddef.h>
#include <stdint.h>

/* EGA graphics pipeline in planar mode (as used by video mode 0Dh), for raw-CPU models of the port-bound RES 00BA:16EC
 * closure (docs/re/EGA_HW_MODEL.md). Written from the public EGA/VGA register documentation; no emulator code. It
 * models the sequencer and graphics-controller index/data ports, four 64 KiB planes behind A000:0000..FFFF, the four
 * latches, write modes 0-2 and read modes 0-1. Odd/even and chain-4 addressing, write mode 3 (VGA only) and the CRT,
 * attribute and DAC registers are outside it. */
typedef struct {
    uint8_t plane[4][0x10000];
    uint8_t latch[4];
    uint8_t seq_index;
    uint8_t seq[5];   /* 2: map mask; the rest are stored only */
    uint8_t gc_index;
    uint8_t gc[9];    /* 0 set/reset, 1 enable set/reset, 2 color compare, 3 rotate/function, 4 read map select, 5 mode,
                         6 misc (stored only), 7 color don't care, 8 bit mask */
} pr_ega;

typedef enum {
    PR_EGA_OK = 0,
    PR_EGA_IGNORED,      /* a port outside 3C4h/3C5h/3CEh/3CFh, or an index beyond the register file; nothing changed */
    PR_EGA_UNSUPPORTED   /* write mode 3; nothing changed */
} pr_ega_status;

/* Power-on state used by the audits after video mode 0Dh: planes and latches zero, map mask 0Fh, color don't care
 * 0Fh, bit mask FFh, every other register zero. */
void pr_ega_reset(pr_ega *ega);
/* OUT of `size` bytes (1 or 2) to `port`. A word OUT to an index port writes the index (low byte), then that
 * register's data (high byte), as on the hardware. */
pr_ega_status pr_ega_out(pr_ega *ega, uint16_t port, uint16_t value, unsigned size);
/* CPU byte read at A000:offset: loads all four latches, then returns read mode 0 (the plane named by read map select)
 * or read mode 1 (bits whose four-plane color matches color compare in the planes set in color don't care). */
uint8_t pr_ega_read(pr_ega *ega, uint16_t offset);
/* CPU byte write at A000:offset through the write mode, rotate, set/reset, ALU function, bit mask and map mask. */
pr_ega_status pr_ega_write(pr_ega *ega, uint16_t offset, uint8_t value);

#endif
