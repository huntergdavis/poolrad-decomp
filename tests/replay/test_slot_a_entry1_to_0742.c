#include "poolrad/replay/slot_a_entry1_to_0742.h"
#include "poolrad/recovered/overlay_u3bd_field.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Synthetic cases only: no game data. The exact stored set is pinned, never
 * "unchanged" or "empty" (see docs/re/SLOT_A_ENTRY1_TO_0742.md). Explicit
 * checks, not assert(), so a Release build cannot compile them out. */
#define RAM_SIZE 655360u
#define DS 0x14A0u
#define SS 0x1D16u
#define CS 0x2E60u

static uint8_t ram[RAM_SIZE];
static uint8_t pristine[RAM_SIZE];
static int failures;

#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static size_t ph(unsigned seg, unsigned off) { return ((size_t)seg << 4) + (off & 0xFFFFu); }
static void setw(unsigned seg, unsigned off, unsigned v)
{
    ram[ph(seg, off)] = (uint8_t)v;
    ram[ph(seg, off) + 1u] = (uint8_t)(v >> 8);
}

static void setup(void)
{
    memset(ram, 0, sizeof ram);
    ram[ph(DS, 0x49FAu)] = 1;                      /* selector */
    ram[ph(DS, 0x6AADu)] = 15; ram[ph(DS, 0x6AAEu)] = 4; ram[ph(DS, 0x6AAFu)] = 6;
    ram[ph(DS, 0x84DCu)] = 20;                     /* map id */
    setw(DS, 0x2628u, 10);
    setw(DS, 0x49D6u, 0); setw(DS, 0x49D8u, 0x3A16u);   /* window 1, word 0x592 = 0 */
    setw(DS, 0x6A5Cu, 0); setw(DS, 0x6A5Eu, 0x3DA1u);   /* 1,024-byte buffer */
    /* Non-zero stack filler so every store, including a stored zero, is a real change. */
    memset(ram + ph(SS, 0x3D00u), 0xA5, 0x300u);
    memset(ram + ph(0x1000u, 0xFF00u), 0xA5, 0x100u);
    memset(ram + ph(0x1000u, 0x0000u), 0xA5, 0x200u);
    memcpy(pristine, ram, sizeof ram);
}

static pr_slot_a_e1_regs regs(void)
{
    pr_slot_a_e1_regs r = {DS, SS, CS, 0x3EF2u, 0x3EF8u};
    return r;
}

static size_t changed(size_t *list, size_t cap)
{
    size_t n = 0, i;
    for (i = 0; i < RAM_SIZE; i++)
        if (ram[i] != pristine[i]) { if (n < cap) list[n] = i; n++; }
    return n;
}

int main(void)
{
    pr_slot_a_e1_report rep;
    pr_slot_a_e1_regs r;
    size_t list[64], n, k;
    uint8_t res = 9;
    pr_u3bd_field_state fs;
    /* Independent expectation for the entry SP 3EF2 / BP 3EF8 / CS 2E60 case. */
    static const struct { unsigned off; uint8_t v; } expect[] = {
        {0x3EEFu, 0x01}, {0x3EF0u, 0xF8}, {0x3EF1u, 0x3E},
        {0x3DC2u, 0x60}, {0x3DC3u, 0x2E}, {0x3DC0u, 0x31}, {0x3DC1u, 0x10},
        {0x3DBEu, 0xF0}, {0x3DBFu, 0x3E}, {0x3DBCu, 0x0A}, {0x3DBDu, 0x00}};

    /* 1. Happy path. The synthetic buffer must really give entry 3 result 1. */
    setup();
    fs = (pr_u3bd_field_state){20, ram + ph(0x3DA1u, 0), 0};
    CHECK(pr_u3bd_entry3(&fs, 15, 4, 6, &res, NULL) == PR_U3BD_FIELD_OK && res == 1);
    r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_STOP_0742);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_BEFORE_073B && rep.entry3_valid == 1 && rep.entry3_result == 1);
    CHECK(rep.sp == 0x3DBCu && rep.bp == 0x3DBEu && rep.ip == 0x0742u && rep.cs == CS);
    CHECK(rep.frame_bytes_written == 11u);
    n = changed(list, 64);
    CHECK(n == 11u);
    for (k = 0; k < sizeof expect / sizeof expect[0]; k++) {
        size_t at = ph(SS, expect[k].off);
        CHECK(ram[at] == expect[k].v);
        CHECK(pristine[at] != expect[k].v);    /* every expected byte really changed (non-vacuous) */
    }
    for (k = 0; k < n && k < 64; k++) {        /* the changed set is exactly the expected set */
        size_t j; int found = 0;
        for (j = 0; j < sizeof expect / sizeof expect[0]; j++)
            if (list[k] == ph(SS, expect[j].off)) found = 1;
        CHECK(found);
    }

    /* 2. Each non-0742 projection stop leaves RAM byte-identical. */
    setup(); ram[ph(DS, 0x49FAu)] = 2; memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_NOT_REACHED);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_SELECTOR_PATH && changed(list, 1) == 0);
    setup(); setw(0x3A16u, 0x592u, 0x00FFu); memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_NOT_REACHED);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_WINDOW_CLEAR && changed(list, 1) == 0);
    setup(); setw(0x3A16u, 0x592u, 0x00FEu); memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_STOP_0742); /* boundary: FE passes */
    setup(); ram[ph(DS, 0x84DCu)] = 0; ram[ph(DS, 0x6AADu)] = 0x80; memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_NOT_REACHED);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_BEFORE_8AE && rep.entry3_valid == 1 && rep.entry3_result == 0);
    CHECK(changed(list, 1) == 0);

    /* 2b. Inputs that must be read from the right DS field (found by probing pr_u3bd_entry3):
     * with the first 0x300 buffer bytes 0x55 and the quarter-3 cell byte 0x40, selector 6 gives
     * result 1 and selector 4 gives result 0, so a wrong facing byte changes the stop. */
    setup(); memset(ram + ph(0x3DA1u, 0), 0x55, 0x300u); ram[ph(0x3DA1u, 0x300u + 16u * 4u + 15u)] = 0x40;
    memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_STOP_0742);
    CHECK(rep.entry3_result == 1 && changed(list, 64) == 11u);
    setup(); memset(ram + ph(0x3DA1u, 0), 0x55, 0x300u); ram[ph(0x3DA1u, 0x300u + 16u * 4u + 15u)] = 0x40;
    ram[ph(DS, 0x6AAFu)] = 4; memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_NOT_REACHED);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_BEFORE_8AE && rep.entry3_result == 0 && changed(list, 1) == 0);
    /* Map id 20 wraps an out-of-range x and proceeds; map 0 (case above) returns 0 at once. */
    setup(); ram[ph(DS, 0x6AADu)] = 0x80; memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_STOP_0742);
    CHECK(rep.entry3_valid == 1 && rep.entry3_result == 1);

    /* 3. Missing input: window, buffer and stack stores outside the image; nothing written. */
    setup(); setw(DS, 0x49D8u, 0x9FFFu); setw(DS, 0x49D6u, 0xFFFFu); memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT && changed(list, 1) == 0);
    setup(); setw(DS, 0x6A5Eu, 0x9FF0u); setw(DS, 0x6A5Cu, 0x0100u); memcpy(pristine, ram, sizeof ram); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT && changed(list, 1) == 0);
    setup(); r = regs(); r.ss = 0x9FFFu; r.sp = 0x0010u;
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT && changed(list, 1) == 0);
    setup(); r = regs(); r.sp = 0x0001u;      /* bp1 = 0xFFFF: a wrapped word store, refused */
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT && changed(list, 1) == 0);
    setup(); r = regs(); r.ds = 0x9FFFu;
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT && changed(list, 1) == 0);
    setup(); /* the store failure must not leak a partial write after a passed projection */
    r = regs(); r.ss = 0x9FFFu; r.sp = 0x0010u;
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_MISSING_INPUT);
    CHECK(rep.prefix_stop == PR_U68_PREFIX_BEFORE_073B && rep.frame_bytes_written == 0u);

    /* 4. SP wrap inside the stack segment (even SP): offsets wrap mod 64K like SP does. */
    setup(); r = regs(); r.ss = 0x1000u; r.sp = 0x0102u; r.bp = 0xBEEFu; r.cs = 0xC0DEu;
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_STOP_0742);
    /* bp1 = 0x0100; sp1 = 0x0100 - 0x12C = 0xFFD4; stores at FFD2, FFD0, FFCE, FFCC */
    CHECK(rep.sp == 0xFFCCu && rep.bp == 0xFFCEu);
    CHECK(ram[ph(0x1000u, 0x00FFu)] == 1 && ram[ph(0x1000u, 0x0100u)] == 0xEF && ram[ph(0x1000u, 0x0101u)] == 0xBE);
    CHECK(ram[ph(0x1000u, 0xFFD2u)] == 0xDE && ram[ph(0x1000u, 0xFFD3u)] == 0xC0);
    CHECK(ram[ph(0x1000u, 0xFFD0u)] == 0x31 && ram[ph(0x1000u, 0xFFD1u)] == 0x10);
    CHECK(ram[ph(0x1000u, 0xFFCEu)] == 0x00 && ram[ph(0x1000u, 0xFFCFu)] == 0x01);   /* bp1 = 0x0100 */
    CHECK(ram[ph(0x1000u, 0xFFCCu)] == 0x0A && ram[ph(0x1000u, 0xFFCDu)] == 0x00);

    /* 5. Argument guards. */
    setup(); r = regs();
    CHECK(pr_slot_a_entry1_to_0742(NULL, RAM_SIZE, &r, &rep) == PR_SLOT_A_E1_ARGUMENT);
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, NULL, &rep) == PR_SLOT_A_E1_ARGUMENT);
    CHECK(pr_slot_a_entry1_to_0742(ram, 0, &r, &rep) == PR_SLOT_A_E1_ARGUMENT);
    CHECK(pr_slot_a_entry1_to_0742(ram, ((size_t)1u << 20) + 1u, &r, &rep) == PR_SLOT_A_E1_ARGUMENT);
    CHECK(pr_slot_a_entry1_to_0742(ram, RAM_SIZE, &r, NULL) == PR_SLOT_A_E1_ARGUMENT);
    CHECK(changed(list, 1) == 0);

    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    puts("slot_a_entry1_to_0742 synthetic cases pass");
    return 0;
}
