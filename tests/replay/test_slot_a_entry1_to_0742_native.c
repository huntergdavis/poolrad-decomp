#include "poolrad/replay/slot_a_entry1_to_0742.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Native comparison: apply the slice-1 adapter to a copy of the retained direct
 * +0x0D6D RAM and compare ALL 655,360 bytes against the direct +0x0742 RAM.
 * Expected: the model writes 11 bytes that equal native; native differs from the
 * model by exactly the pinned 28 dead-stack residue bytes and nowhere else. */
#define RAM_SIZE 655360u
#define SS_BASE 0x1D160u   /* SS 1D16 physical base, from the CPU files */

static uint8_t start_ram[RAM_SIZE], native_ram[RAM_SIZE], model_ram[RAM_SIZE];

/* The residue set, SS-relative: SS:3D9D..3DA6 and SS:3DAA..3DBB (all below the stop SP 3DBC). */
static const unsigned residue_lo[] = {0x3D9Du, 0x3DAAu};
static const unsigned residue_hi[] = {0x3DA6u, 0x3DBBu};

static int load(const char *path, uint8_t *ram)
{
    FILE *f = fopen(path, "rb");
    int ok;
    if (!f) return 0;
    ok = fread(ram, 1u, RAM_SIZE, f) == RAM_SIZE && fgetc(f) == EOF;
    return fclose(f) == 0 && ok;
}

static int parse_cpu(const char *path, unsigned *ds, unsigned *ss, unsigned *cs, unsigned *sp,
                     unsigned *bp, unsigned *ip)
{
    char line[256];
    unsigned ax, bx, cx, dx, si, di, es, fl;
    FILE *f = fopen(path, "r");
    int n;
    if (!f) return 0;
    n = fgets(line, sizeof line, f) != NULL;
    fclose(f);
    if (!n) return 0;
    return sscanf(line, "AX=%x BX=%x CX=%x DX=%x SI=%x DI=%x BP=%x SP=%x CS=%x IP=%x DS=%x ES=%x SS=%x FLAGS=%x",
                  &ax, &bx, &cx, &dx, &si, &di, bp, sp, cs, ip, ds, &es, ss, &fl) == 14;
}

static int in_residue(size_t i)
{
    unsigned k;
    for (k = 0; k < 2u; k++)
        if (i >= SS_BASE + residue_lo[k] && i <= SS_BASE + residue_hi[k]) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    pr_slot_a_e1_regs r;
    pr_slot_a_e1_report rep;
    unsigned ds, ss, cs, sp, bp, ip, n_cs, n_sp, n_bp, n_ip, n_ds, n_ss;
    size_t i, model_writes = 0, residue = 0, other = 0, residue_expected = 0;
    unsigned k;

    if (argc != 5 || !load(argv[1], start_ram) || !load(argv[2], native_ram) ||
        !parse_cpu(argv[3], &ds, &ss, &cs, &sp, &bp, &ip) ||
        !parse_cpu(argv[4], &n_ds, &n_ss, &n_cs, &n_sp, &n_bp, &n_ip)) {
        fputs("usage: test_slot_a_entry1_to_0742_native RAM_0D6D RAM_0742 CPU_0D6D CPU_0742\n", stderr);
        return 2;
    }
    if (ip != 0x0D6Du || n_ip != 0x0742u) { fputs("CPU files are not the +0D6D/+0742 stops\n", stderr); return 1; }
    memcpy(model_ram, start_ram, RAM_SIZE);
    r = (pr_slot_a_e1_regs){(uint16_t)ds, (uint16_t)ss, (uint16_t)cs, (uint16_t)sp, (uint16_t)bp};
    if (pr_slot_a_entry1_to_0742(model_ram, RAM_SIZE, &r, &rep) != PR_SLOT_A_E1_STOP_0742) {
        fprintf(stderr, "adapter stop %d prefix %d\n", 0, (int)rep.prefix_stop); return 1;
    }
    /* Registers at the stop against the native CPU transcription. */
    if (rep.sp != n_sp || rep.bp != n_bp || rep.ip != n_ip || rep.cs != n_cs || ds != n_ds || ss != n_ss) {
        fprintf(stderr, "register mismatch sp %04X/%04X bp %04X/%04X ip %04X/%04X cs %04X/%04X\n",
                rep.sp, n_sp, rep.bp, n_bp, rep.ip, n_ip, rep.cs, n_cs);
        return 1;
    }
    for (i = 0; i < RAM_SIZE; i++) {
        int model_changed = model_ram[i] != start_ram[i];
        int differs = model_ram[i] != native_ram[i];
        if (model_changed) { model_writes++; if (differs) { fprintf(stderr, "model byte %zx != native\n", i); return 1; } }
        if (differs) { if (in_residue(i)) residue++; else { other++; fprintf(stderr, "unexpected diff at %zx\n", i); } }
    }
    for (k = 0; k < 2u; k++) residue_expected += residue_hi[k] - residue_lo[k] + 1u;
    /* Pinned exact sets: 11 model writes (every one of them a real change at +0D6D) and 28 residue bytes, 0 other. */
    printf("model_writes=%zu residue_diffs=%zu (pinned %zu) other_diffs=%zu\n", model_writes, residue, residue_expected, other);
    if (model_writes != 11u || other != 0u) return 1;
    /* 28 bytes are in the pinned ranges; 31 positions in the ranges include 3 bytes equal in both (3DA7..3DA9 excluded by range). */
    if (residue != 28u) return 1;
    puts("native 0D6D->0742: model + pinned residue == native over all 655360 bytes");
    return 0;
}
