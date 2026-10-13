/* Private checker for the resident startup video-mode selection: runs pr_vi_select_mode with the adapter letter 0x45 and a fabricated BIOS
 * (the mode query answers 3, the previous DOS text mode, a host input) and compares the display-state bytes it writes with a hash-pinned native
 * RAM image of the saved slot-A first-view route (DS = 0x14A0). Reads the ignored image; writes nothing. Usage: first_view_display_state RAM.BIN */
#include "poolrad/recovered/resident_video_init_4426.h"

#include <stdio.h>
#include <string.h>

#define DS_BASE 0x14A00u

static int bios(void *ctx, pr_vi_ds *ds, uint8_t intno, uint8_t regs[PR_VI_REGS_BYTES])
{
    (void)ctx; (void)ds; (void)intno;
    if (regs[1] == 0x0F) regs[0] = 3;
    return 0;
}

int main(int argc, char **argv)
{
    static uint8_t ram[655360];
    const uint8_t *d = ram + DS_BASE;
    pr_vi_hooks h = { bios };
    pr_vi_ds ds;
    FILE *f;
    unsigned i, bad = 0;
    if (argc != 2) { fprintf(stderr, "usage: %s RAM.BIN\n", argv[0]); return 2; }
    f = fopen(argv[1], "rb");
    if (!f || fread(ram, 1, sizeof ram, f) != sizeof ram) { fprintf(stderr, "cannot read a 655,360-byte image\n"); return 2; }
    fclose(f);
    memset(&ds, 0, sizeof ds);
    ds.b484c = PR_VI_ADAPTER_E;
    if (pr_vi_select_mode(&ds, NULL, &h, NULL) != PR_VI_OK) return 3;
    for (i = 0; i < 14; i++) if (ds.regs[i] != d[0x4A02 + i]) { printf("regs[%u]: model %02X native %02X\n", i, ds.regs[i], d[0x4A02 + i]); bad++; }
    if (ds.w4a16 != (uint16_t)(d[0x4A16] | (d[0x4A17] << 8))) { printf("[4A16] differs\n"); bad++; }
    if (ds.w4a18 != (uint16_t)(d[0x4A18] | (d[0x4A19] << 8))) { printf("[4A18] differs\n"); bad++; }
    if (ds.b4a1a != d[0x4A1A] || ds.b4a1b != d[0x4A1B] || ds.b4a1c != d[0x4A1C] || ds.b4a1d != d[0x4A1D] || ds.b4a1e != d[0x4A1E]) { printf("[4A1A..4A1E] differ\n"); bad++; }
    printf("compared 14 register bytes, [4A16], [4A18], [4A1A..4A1E]: %u mismatches\n", bad);
    return bad ? 1 : 0;
}
