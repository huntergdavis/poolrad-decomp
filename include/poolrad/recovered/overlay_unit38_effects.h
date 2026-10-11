#ifndef POOLRAD_RECOVERED_OVERLAY_UNIT38_EFFECTS_H
#define POOLRAD_RECOVERED_OVERLAY_UNIT38_EFFECTS_H

/* Portable C translations of sixteen bounded routines in DOS v1.3 GAME.OVR
 * overlay unit 0x38 (file offset 60,507; run-time stub segment 0x41):
 * public entries 2, 3, 4, 13, 16, 18, 25, 28, 32, 36, 48, 59, 74, 77, 98
 * and 102 of the far-pointer dispatch table that the installer (entry 1,
 * +0x31A2) fills at DS:0x682C.. and that unit 0x9e entry 1 invokes through
 * `lcall [di + 0x6828]` (table item = entry - 1 for every routine here).
 *
 * Every routine is a far Pascal procedure ending in `retf 0xA`: the
 * dispatcher pushes two far pointers and one byte, so the callee frame holds
 *   [bp+0xC..0xF]  far pointer A: a 285-byte character record (the active
 *                  record [0x5D92] in the unit 0x77 caller; its slot +0x108
 *                  is a documented in-memory far-pointer field);
 *   [bp+0x8..0xB]  far pointer B: a second object (a 9-byte sidecar node in
 *                  two callers, a different structure in another, 0:0 in
 *                  two); only byte 3 of it is touched here;
 *   [bp+0x6]       a byte argument no routine here reads.
 * Nine routines touch only engine state bytes; seven also read or write the
 * two objects. None calls anything. Provenance, instruction tables, branch
 * proofs and tests: docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md.
 * Field meanings are not asserted; names below are addresses, not roles.
 * No game data is embedded here. */

#include <stddef.h>
#include <stdint.h>

#define PR_U38_RECORD_SIZE 285u         /* verified record frame (docs/formats/DOS_CHARACTER.md) */
#define PR_U38_RECORD_SLOT_108 0x108u   /* far-pointer slot read by entries 4, 32, 48 */
#define PR_U38_RECORD_BYTE_15 0x15u     /* written by entry 13 */
#define PR_U38_RECORD_BYTE_111 0x111u   /* entries 16, 28 */
#define PR_U38_RECORD_BYTE_112 0x112u   /* entry 28 */
#define PR_U38_SOURCE_MIN_SIZE 4u       /* object B: byte 3 is read (13) or updated (74) */

/* A raw DOS far pointer as stored in memory: offset word, then segment word. */
typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_u38_far_ptr;

/* Resolves a far pointer to host memory with at least `need` readable (and,
 * for entry 48, writable) bytes, or returns NULL when the pointer cannot be
 * followed. The engine performs the raw dereference without any check. */
typedef uint8_t *(*pr_u38_resolve_fn)(void *context, pr_u38_far_ptr pointer, size_t need);

/* Engine data-segment bytes these routines read or write, named by address.
 * `scratch_6826` is the far-pointer temporary at [0x6826]/[0x6828] that
 * entry 4 (and fourteen other unit 0x38 routines) overwrite; it overlaps
 * table item 0, which the installer never fills. */
typedef struct {
    uint8_t b6816;
    uint8_t b6818;
    uint8_t b6819;
    uint8_t b681a;
    uint8_t b681b;
    uint8_t b681d;
    uint8_t b681e;
    uint8_t b6822;
    uint8_t b6825;
    uint8_t b6de3;
    pr_u38_far_ptr scratch_6826;
} pr_u38_state;

typedef enum {
    PR_U38_OK = 0,
    PR_U38_ARGUMENT      /* null state/record/source or an unresolvable far pointer:
                            the engine would read or write through a bad pointer */
} pr_u38_status;

/* Path flags reported through the optional `paths` output. */
#define PR_U38_PATH_EFFECT   0x1u   /* the conditional effect block ran */
#define PR_U38_PATH_SECOND   0x2u   /* the second conditional block ran (entry 16: [0x6818] cleared;
                                       entry 74: the mark-and-set block) */

/* ---- State-only routines (no record access) ---------------------------- */

/* Entry 2, +0x010D (file 0x0ED68), 16 bytes, 6 instructions, straight line:
 * [0x6825] += 5; [0x6822] += 1 (both modulo 256). Table item 1. */
pr_u38_status pr_u38_entry2(pr_u38_state *state);

/* Entry 3, +0x011D (file 0x0ED78), 30 bytes, 10 instructions, two paths:
 * if [0x6825] < 5 (unsigned) then [0x6825] = 0 else [0x6825] -= 5;
 * then [0x6822] -= 1 (modulo 256). `paths` gets PR_U38_PATH_EFFECT when the
 * subtract path ran, 0 when the clamp-to-zero path ran. Table item 2. */
pr_u38_status pr_u38_entry3(pr_u38_state *state, unsigned *paths);

/* Entry 18, +0x06E4 (file 0x0F33F), 31 bytes, 13 instructions, two paths:
 * if bit 0 of [0x6819] is set: [0x6818] >>= 1 (zero-extended shift, low byte
 * stored) and [0x6816] += 3 (modulo 256). Differs from the merged entry 10
 * (+0x03E1) only in the tested bit (1 instead of 2). Table item 20. */
pr_u38_status pr_u38_entry18(pr_u38_state *state, unsigned *paths);

/* Entry 25, +0x0A3D (file 0x0F698), 28 bytes, 13 instructions, straight
 * line: [0x6818] = [0x6818] - ([0x6818] >> 2), computed on the zero-extended
 * byte in 16-bit registers and stored as a byte (no wrap is possible).
 * Table item 29. */
pr_u38_status pr_u38_entry25(pr_u38_state *state);

/* Entry 36, +0x10C1 (file 0x0FD1C), 17 bytes, 8 instructions, straight line:
 * [0x681A] >>= 1 (zero-extended shift, low byte stored). Table item 42. */
pr_u38_status pr_u38_entry36(pr_u38_state *state);

/* Entry 59, +0x171F (file 0x1037A), 17 bytes, 6 instructions, straight line:
 * [0x681E] = 1; [0x6822] -= 4 (modulo 256). Table item 71. */
pr_u38_status pr_u38_entry59(pr_u38_state *state);

/* Entries 77 (+0x25A2, file 0x111FD, item 93), 98 (+0x2A6D, file 0x116C8,
 * item 114) and 102 (+0x2B63, file 0x117BE, item 118): 26 bytes, 12
 * instructions, two paths each; identical except for the tested bit of
 * [0x6819] (1, 4 and 2 respectively): when it is set, [0x6818] >>= 1
 * (zero-extended shift, low byte stored); nothing else changes. */
pr_u38_status pr_u38_entry77(pr_u38_state *state, unsigned *paths);
pr_u38_status pr_u38_entry98(pr_u38_state *state, unsigned *paths);
pr_u38_status pr_u38_entry102(pr_u38_state *state, unsigned *paths);

/* ---- Routines that touch the record (A) or the second object (B) -------- */

/* Entry 13, +0x05D6 (file 0x0F231), 21 bytes, 8 instructions, straight line:
 * record[0x15] = source[3]. No engine state is touched. Table item 14. */
pr_u38_status pr_u38_entry13(uint8_t *record, const uint8_t *source);

/* Entry 16, +0x064E (file 0x0F2A9), 43 bytes, 13 instructions, four paths:
 * if record[0x111] < 0x39 (unsigned) then record[0x111] = 0x39
 * (PR_U38_PATH_EFFECT); [0x6816] += 1 (modulo 256); if [0x681B] == 0x0F then
 * [0x6818] = 0 (PR_U38_PATH_SECOND). Table item 17. */
pr_u38_status pr_u38_entry16(pr_u38_state *state, uint8_t *record, unsigned *paths);

/* Entry 28, +0x0BAC (file 0x0F807), 35 bytes, 10 instructions, straight
 * line, in this order: [0x6822] -= 4; record[0x111] -= 4; record[0x112] -= 4;
 * [0x6816] -= 4 (all modulo 256). Table item 33. */
pr_u38_status pr_u38_entry28(pr_u38_state *state, uint8_t *record);

/* Entry 32, +0x0C26 (file 0x0F881), 32 bytes, 10 instructions, two paths:
 * P = far pointer stored at record[0x108..0x10B]; if the signed byte P[3]
 * is > 0 then [0x681E] = 1 and [0x6822] = 0xFF. `resolve` must yield at
 * least 4 bytes for P. Table item 37. */
pr_u38_status pr_u38_entry32(pr_u38_state *state, const uint8_t *record,
                             pr_u38_resolve_fn resolve, void *context, unsigned *paths);

/* Entry 48, +0x1452 (file 0x100AD), 32 bytes, 10 instructions, two paths:
 * P = far pointer stored at record[0x108..0x10B]; P[6] = 0 unconditionally;
 * then if [0x681D] != 0, [0x681A] = 0 (PR_U38_PATH_EFFECT). `resolve` must
 * yield at least 7 writable bytes for P. Reached with B = 0:0 from +0x18B5
 * and +0x1935 (index 0x3A), consistent with B being unused. Table item 58. */
pr_u38_status pr_u38_entry48(pr_u38_state *state, const uint8_t *record,
                             pr_u38_resolve_fn resolve, void *context, unsigned *paths);

/* Entry 74, +0x2490 (file 0x110EB), 73 bytes, 25 instructions, three paths:
 * if [0x6DE3] == 0 and [0x6822] == 0: source[3] &= 0x0F (PR_U38_PATH_EFFECT);
 * else if bit 4 of source[3] is clear: [0x6822] = 0xFF and source[3] |= 0x10
 * (PR_U38_PATH_SECOND); else nothing. The record is not touched. Table item 89. */
pr_u38_status pr_u38_entry74(pr_u38_state *state, uint8_t *source, unsigned *paths);

/* Entry 4, +0x013B (file 0x0ED96), 52 bytes, 15 instructions, two paths:
 * P = far pointer at record[0x108..0x10B]; Q = far pointer at P[0xA..0xD];
 * Q is stored into the scratch temporary [0x6826]/[0x6828] (always, before
 * the test); if Q[0x9F] == 4 then [0x6822] += 2 and [0x6818] += 2 (modulo
 * 256). `resolve` must yield 14 bytes for P and 0xA0 bytes for Q; when Q
 * cannot be resolved the scratch words are already updated and
 * PR_U38_ARGUMENT is returned. Table item 3. */
pr_u38_status pr_u38_entry4(pr_u38_state *state, const uint8_t *record,
                            pr_u38_resolve_fn resolve, void *context, unsigned *paths);

#endif
