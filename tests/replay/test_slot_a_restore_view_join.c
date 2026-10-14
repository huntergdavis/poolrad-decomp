#include "poolrad/replay/slot_a_restore_view_join.h"
#include "poolrad/formats/dax.h"
#include "poolrad/formats/dos_savgam.h"
#include "poolrad/formats/geo.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS 0x14A0u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint8_t ram[RAM_SIZE], original_ram[RAM_SIZE];
static pr_slot_a_restore_view_input prepared, original;

static int synthetic(void)
{
    uint8_t save[PR_DOS_SAVGAM_SIZE] = {0};
    uint8_t geo[PR_GEO_SIZE] = {0};
    pr_slot_a_restore_view_gate_report report;
    static const unsigned counts[] = {6u, 0u, 255u};
    unsigned case_index, count;
    save[1u + 0x1E4u] = 7u;
    save[0x3201u] = 3u;
    save[0x3202u] = 2u;
    save[0x3203u] = 4u;
    geo[2u + 2u * 16u + 3u] = 0xABu;
    memset(&prepared, 0xA5, sizeof prepared);
    memcpy(&original, &prepared, sizeof original);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save - 1u,
          geo, sizeof geo, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_SAVE);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
          geo, sizeof geo, 8u, &prepared) == PR_SLOT_A_RESTORE_VIEW_GEO);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
          geo, sizeof geo - 1u, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_GEO);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    save[0x3201u] = 16u;
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
          geo, sizeof geo, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_GEO);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    save[0x3201u] = 3u;
    CHECK(pr_slot_a_restore_view_prepare(NULL, sizeof save,
          geo, sizeof geo, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_ARGUMENT);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
          NULL, sizeof geo, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_ARGUMENT);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
          geo, sizeof geo, 7u, NULL) == PR_SLOT_A_RESTORE_VIEW_ARGUMENT);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);

    /* Count changes the queued boundary, never the first close blocker. */
    for (case_index = 0u; case_index < sizeof counts / sizeof counts[0]; ++case_index) {
        count = counts[case_index];
        save[0x3208u] = (uint8_t)count;
        CHECK(pr_slot_a_restore_view_prepare(save, sizeof save,
              geo, sizeof geo, 7u, &prepared) == PR_SLOT_A_RESTORE_VIEW_OK);
        CHECK(prepared.view.map_id == 7u && prepared.view.saved.x == 3u);
        CHECK(prepared.view.saved.y == 2u && prepared.view.saved.facing == 4u);
        CHECK(prepared.view.wall[0] == 10u && prepared.view.wall[1] == 11u);
        memcpy(&original, &prepared, sizeof original);
        CHECK(pr_slot_a_restore_view_gate(&prepared, &report) ==
              PR_SLOT_A_RESTORE_VIEW_BLOCKED_CLOSE_1491);
        CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
        CHECK(report.blocked.caller_unit == 0x6Fu);
        CHECK(report.blocked.call_pc == 0x18E5u);
        CHECK(report.blocked.callee_segment == 0x0AF8u);
        CHECK(report.blocked.callee_pc == 0x1491u);
        CHECK(report.blocked.return_pc == 0x18EAu);
        CHECK(report.blocked.requirement == PR_SLOT_A_RESTORE_VIEW_LIVE_SAVE_HANDLE);
        CHECK(report.blocked.argument_bp_offset == -0x1C8);
        CHECK(report.blocked.source_cs_offset == 0u);
        CHECK(report.after_close_pc == 0x18EAu);
        CHECK(report.queued_valid == (count != 0u));
        CHECK(report.after_party_prefix_pc == (count ? 0x1965u : 0x19F7u));
        if (count) {
            CHECK(report.queued.caller_unit == 0x6Fu);
            CHECK(report.queued.call_pc == 0x1965u);
            CHECK(report.queued.callee_segment == 0x0AF8u);
            CHECK(report.queued.callee_pc == 0x0B0Bu);
            CHECK(report.queued.return_pc == 0x196Au);
            CHECK(report.queued.argument_bp_offset == -0x214);
            CHECK(report.queued.source_cs_offset == 0x15E7u);
            CHECK(report.queued.requirement ==
                  PR_SLOT_A_RESTORE_VIEW_LIVE_PASCAL_SOURCE_AND_DESTINATION);
        } else {
            CHECK(report.queued.call_pc == 0u);
        }
    }
    CHECK(pr_slot_a_restore_view_gate(NULL, &report) ==
          PR_SLOT_A_RESTORE_VIEW_ARGUMENT);
    CHECK(pr_slot_a_restore_view_gate(&prepared, NULL) ==
          PR_SLOT_A_RESTORE_VIEW_ARGUMENT);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    return 0;
}

static size_t physical(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static uint16_t word(unsigned seg, unsigned off)
{
    size_t at = physical(seg, off);
    return (uint16_t)((uint16_t)ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static int read_exact(const char *path, uint8_t *out, size_t size)
{
    FILE *f = fopen(path, "rb");
    int ok;
    if (!f) return 0;
    ok = fread(out, 1u, size, f) == size && fgetc(f) == EOF;
    return fclose(f) == 0 && ok;
}

static int native(const char *save_path, const char *geo_path, const char *ram_path)
{
    uint8_t save[PR_DOS_SAVGAM_SIZE], dax[2572u], geo[PR_GEO_SIZE];
    pr_dax_archive archive;
    pr_slot_a_restore_view_gate_report report;
    const pr_slot_a_view_context *view;
    unsigned seg, off, w0seg, w0off, direction;
    size_t i, written = 0u, map_at, cell;
    CHECK(read_exact(save_path, save, sizeof save));
    CHECK(read_exact(geo_path, dax, sizeof dax));
    CHECK(read_exact(ram_path, ram, sizeof ram));
    memcpy(original_ram, ram, sizeof ram);
    CHECK(pr_dax_open(dax, sizeof dax, &archive) == PR_DAX_OK);
    for (i = 0u; i < archive.count; ++i) {
        if (archive.entries[i].id == save[1u + 0x1E4u]) break;
    }
    CHECK(i < archive.count);
    CHECK(pr_dax_read(&archive, i, geo, sizeof geo, &written) == PR_DAX_OK);
    CHECK(written == sizeof geo);
    CHECK(pr_slot_a_restore_view_prepare(save, sizeof save, geo, sizeof geo,
          archive.entries[i].id, &prepared) == PR_SLOT_A_RESTORE_VIEW_OK);
    memcpy(&original, &prepared, sizeof original);
    view = &prepared.view;
    seg = word(DS, 0x6A5Eu);
    off = word(DS, 0x6A5Cu);
    map_at = physical(seg, off);
    CHECK(off <= 0xFC00u && map_at <= RAM_SIZE - 0x400u);
    CHECK(memcmp(geo + 2u, ram + map_at, 0x400u) == 0);
    CHECK(view->map_id == ram[physical(DS, 0x84DCu)]);
    CHECK(view->saved.x == ram[physical(DS, 0x6AADu)]);
    CHECK(view->saved.y == ram[physical(DS, 0x6AAEu)]);
    CHECK(view->saved.facing == ram[physical(DS, 0x6AAFu)]);
    w0off = word(DS, 0x49D2u);
    w0seg = word(DS, 0x49D4u);
    CHECK(w0off <= 0xF800u && physical(w0seg, w0off) <= RAM_SIZE - 0x800u);
    CHECK(view->saved.minute == word(w0seg, w0off + PR_SLOT_A_CLOCK_MINUTE_OFFSET));
    CHECK(view->saved.tens == word(w0seg, w0off + PR_SLOT_A_CLOCK_TENS_OFFSET));
    CHECK(view->saved.hour == word(w0seg, w0off + PR_SLOT_A_CLOCK_HOUR_OFFSET));
    cell = (size_t)view->saved.y * 16u + view->saved.x;
    for (direction = 0u; direction < 4u; ++direction) {
        uint8_t packed = ram[map_at + cell + (direction >= 2u ? 256u : 0u)];
        CHECK(view->wall[direction] ==
              ((packed >> (direction % 2u ? 0u : 4u)) & 15u));
        CHECK(view->door[direction] ==
              ((ram[map_at + 0x300u + cell] >> (2u * direction)) & 3u));
    }
    CHECK(view->event == ram[map_at + 0x200u + cell]);
    CHECK(pr_slot_a_restore_view_gate(&prepared, &report) ==
          PR_SLOT_A_RESTORE_VIEW_BLOCKED_CLOSE_1491);
    CHECK(memcmp(&prepared, &original, sizeof prepared) == 0);
    CHECK(memcmp(ram, original_ram, sizeof ram) == 0);
    CHECK(report.blocked.call_pc == 0x18E5u && report.queued_valid == 1u);
    CHECK(report.queued.call_pc == 0x1965u);
    puts("native: 1024 map bytes, 7 saved fields, 9 context fields match; blocked state and RAM unchanged");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 4) return 2;
    if (synthetic()) return 1;
    if (argc == 4 && native(argv[1], argv[2], argv[3])) return 1;
    puts("slot-A restore/view join and typed close blocker checks passed");
    return 0;
}
