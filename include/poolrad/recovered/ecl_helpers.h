#ifndef POOLRAD_RECOVERED_ECL_HELPERS_H
#define POOLRAD_RECOVERED_ECL_HELPERS_H

/* Portable C recovered from the DOS v1.3 GAME.OVR overlay unit 0x25: one
 * fully translated leaf function (pr_rec_ecl_char_remap) and two extracted
 * behavior cores (pr_rec_ecl_operand_count, pr_rec_ecl_unpack_text) that
 * reproduce the verified decision table or decoding logic of their original
 * but deliberately omit the engine-memory side effects (script buffer, PC,
 * string runtime). Provenance, status, and checks: docs/re/FUNCTION_LEDGER.md.
 * No game data is embedded here. */

#include <stddef.h>
#include <stdint.h>

/* Engine string buffers are Turbo Pascal string[255]; the unpacker's output
 * cannot exceed this many characters. */
#define PR_REC_ECL_TEXT_MAX 255u

/* GAME.OVR 0x07BA6 (unit 0x25 + 0x06D3). Six-bit text code to ASCII:
 * codes 0x00..0x1F map to code + 0x40, all other values pass through.
 * The engine computes the sum in a 16-bit register and returns the low byte. */
uint8_t pr_rec_ecl_char_remap(uint8_t code);

/* Extracted behavior core of GAME.OVR 0x09379 (unit 0x25 + 0x1EA6), the
 * operand-count dispatcher. The original fetches the opcode byte at
 * buffer + PC, latches it, and either calls the operand loader with a count
 * or advances the PC by one; this function reproduces only the verified
 * opcode-to-count decision. It returns the number of tagged operands the
 * engine prefetches for an opcode: 1, 2, 3, 4, 5, 6, 8 or 14, and 0 for every
 * other opcode value (the PC-advance case). Menus and ON GOTO/GOSUB (0x15,
 * 0x25, 0x26, 0x2B) return 0 because their handlers read their own lists.
 * This is the prefetch count, not the wire-format operand count. */
unsigned pr_rec_ecl_operand_count(uint8_t opcode);

/* Extracted behavior core of GAME.OVR 0x08626 (unit 0x25 + 0x1153), the
 * six-bit text unpacker. The original reads its bytes through the script
 * buffer pointer while advancing the engine PC and appends characters to a
 * Pascal string buffer through four runtime calls; this function decodes
 * from a caller-supplied byte array instead and omits those side effects.
 * Verified decoding: consumes exactly packed_len bytes, emits one character
 * per non-zero six-bit code (four codes per three bytes, MSB first, each
 * remapped with pr_rec_ecl_char_remap); zero codes are skipped, not treated
 * as terminators; the fourth code of a group is emitted only when the final
 * byte completes a three-byte group. PROVISIONAL: at most PR_REC_ECL_TEXT_MAX
 * characters are produced, modelling the 0xFF maximum length the original
 * passes to the runtime string store; the store routine itself is not yet
 * identified. Writes up to capacity characters into out (no NUL) and
 * returns the number of characters the engine would have produced. */
size_t pr_rec_ecl_unpack_text(const uint8_t *packed, size_t packed_len,
                              char *out, size_t capacity);

/* GAME.OVR 0x07B82 (unit 0x25 + 0x06AF, public entry 9, called as 0x2B:0x4D).
 * Full translation. Assembles a 16-bit operand value from the two value
 * bytes of a prefetched operand slot: the first Pascal argument is the byte
 * stored at [0x7085+i] (low), the second the byte at [0x7045+i] (high);
 * the engine computes (second << 8) + first in AX. */
uint16_t pr_rec_ecl_operand_word(uint8_t low, uint8_t high);

/* GAME.OVR 0x07BCE (unit 0x25 + 0x06FB, internal). Full translation.
 * Classifies a 16-bit script-space address by inclusive range, in the order
 * the engine tests them (the ranges do not overlap):
 *   0: 0x4900..0x4CFF   1: 0x6B00..0x6EFF   2: 0x9700..0x98FF
 *   3: 0x9900..0xB6FF (the loaded script block; 0x9900 is unpacked offset 2)
 *   4: any other value.
 * The meaning of ranges 0 to 2 (engine data windows the scripts may address)
 * is not asserted here; see docs/re/FUNCTION_LEDGER.md. */
uint8_t pr_rec_ecl_address_class(uint16_t address);

/* ---- Operand loader state transition (issue #16) --------------------------
 * Recovered core of GAME.OVR 0x074D3 (unit 0x25 + 0x0000, public entry 2,
 * called as 0x2B:0x2A from 42 handler sites and near-called by the skip
 * dispatcher). The original loads `count` tagged operands from the script at
 * the engine PC into three byte arrays indexed 1..count (tag at 0x7005+i,
 * first value byte at 0x7085+i, second value byte at 0x7045+i), advances the
 * PC, and for string operands fills Pascal string buffers 0x6FC8 + 256*k.
 * This translation keeps all of that in a caller-owned state object with
 * explicit bounds checks that the engine does not have. The 0x81 path
 * (string by address) is translated only for addresses inside the script
 * window; addresses in the engine data windows are recorded and flagged
 * unresolved. See docs/re/FUNCTION_LEDGER.md section 1.6. */

#define PR_REC_ECL_SCRIPT_BASE 0x9900u      /* virtual address of script byte 0 */
#define PR_REC_ECL_SLOT_COUNT 16u           /* slots 1..15 modelled (engine prefetches at most 14) */
#define PR_REC_ECL_WINDOW0_WORDS 1024u      /* GetMem(0x800) in unit 0x36 + 0x002D */
#define PR_REC_ECL_WINDOW1_WORDS 1024u      /* GetMem(0x800) */
#define PR_REC_ECL_WINDOW2_WORDS 512u       /* GetMem(0x400) */
#define PR_REC_ECL_NAME_MAX 15u             /* string[15] assignment limit passed to the runtime store */
#define PR_REC_ECL_CALL_STACK_MAX 64u       /* model bound; the engine's list is heap-bound */

typedef enum {
    PR_REC_ECL_LOADER_OK = 0,
    PR_REC_ECL_LOADER_ARGUMENT,   /* null state or script with non-zero size */
    PR_REC_ECL_LOADER_COUNT,      /* count exceeds the modelled slots; state unchanged */
    PR_REC_ECL_LOADER_BOUNDS      /* a script read would leave the buffer; state partially updated */
} pr_rec_ecl_loader_status;

typedef struct {
    const uint8_t *script;        /* bytes addressed from PR_REC_ECL_SCRIPT_BASE (unpacked block + 2) */
    size_t script_size;
    uint16_t pc;                  /* engine [0x49ED]: virtual address of the current opcode */
    uint8_t tag[PR_REC_ECL_SLOT_COUNT];    /* engine [0x7005+i] */
    uint8_t low[PR_REC_ECL_SLOT_COUNT];    /* engine [0x7085+i] */
    uint8_t high[PR_REC_ECL_SLOT_COUNT];   /* engine [0x7045+i] */
    uint8_t string_count;         /* engine local: string operands seen in this call, k = 1.. */
    uint8_t string_len[PR_REC_ECL_SLOT_COUNT];        /* length byte of buffer 0x6FC8 + 256*k */
    char string_text[PR_REC_ECL_SLOT_COUNT][PR_REC_ECL_TEXT_MAX];
    uint16_t string_address[PR_REC_ECL_SLOT_COUNT];   /* 0x81 operands: the address passed on */
    uint8_t string_unresolved[PR_REC_ECL_SLOT_COUNT]; /* 1 = 0x81 target needs state this model lacks (see below) */
    /* ---- engine string/variable windows (issue #22) ----------------------
     * Allocated and zero-filled once by unit 0x36 + 0x002D: three heap arrays of
     * 16-bit words addressed by script addresses minus the window base:
     *   window 0: addresses 0x4900..0x4CFF, 1024 words, far pointer [0x49D2]
     *   window 1: addresses 0x6B00..0x6EFF, 1024 words, far pointer [0x49D6]
     *   window 2: addresses 0x9700..0x98FF,  512 words, far pointer [0x49DA]
     * Strings live in them one character per word, zero word terminated. */
    uint16_t window0[PR_REC_ECL_WINDOW0_WORDS];
    uint16_t window1[PR_REC_ECL_WINDOW1_WORDS];
    uint16_t window2[PR_REC_ECL_WINDOW2_WORDS];
    /* Address 0x6B00 is not a window word: it names the active character's
     * name string (a string[15] at far pointer [0x5D92]). Supply it here;
     * with active_name_valid == 0 the engine would read through a null or
     * stale pointer, so the model reports that path as unresolved. */
    char active_name[PR_REC_ECL_NAME_MAX];
    uint8_t active_name_len;
    uint8_t active_name_valid;
    /* Optional writable script buffer for class-3 stores (the engine writes
     * into its script buffer); NULL leaves such stores unresolved. */
    uint8_t *script_rw;
    /* ---- GOSUB call stack and PRINT RETURN state (issue #34) -------------
     * The engine keeps a linked list of 6-byte heap nodes headed by the far
     * pointer [0x6F70]: word 0 = saved PC, words 1-2 = far pointer to the
     * previous node (null for the first). Nodes are pushed by unit 0x25 +0x1951
     * and popped/freed by the RETURN and EXIT handlers. This model keeps the
     * saved PCs in a bounded array; depth 0 = empty list. */
    uint16_t call_stack[PR_REC_ECL_CALL_STACK_MAX];
    unsigned call_depth;
    /* Engine bytes written by PRINT RETURN, named by address because their
     * meaning is not asserted here: [0x84DF] flag, [0x5E24], [0x5E25]. */
    uint8_t b84df;
    uint8_t b5e24;
    uint8_t b5e25;
} pr_rec_ecl_loader_state;

typedef enum {
    PR_REC_ECL_WINDOW_OK = 0,
    PR_REC_ECL_WINDOW_ARGUMENT,
    PR_REC_ECL_WINDOW_BOUNDS,       /* string would run past a window or the script buffer */
    PR_REC_ECL_WINDOW_UNRESOLVED,   /* path needs engine state this model lacks (unset active
                                     * name, class-4 pseudo-variables, class-3 store without
                                     * script_rw); no window or script byte is written, but a
                                     * string read still records the slot's address, empty
                                     * length and unresolved flag */
    PR_REC_ECL_WINDOW_HOOK          /* class-1 word access done, but the engine also runs an un-modelled hook */
} pr_rec_ecl_window_status;

/* Zero-fills the three windows and the string slots and invalidates the
 * active name, as the initializer (which also nulls [0x5D92]) does. */
void pr_rec_ecl_windows_clear(pr_rec_ecl_loader_state *state);

/* Unit 0x25 + 0x12FB (GAME.OVR 0x087CE): copy the string at `address` into
 * string slot k. Classes 0-2 read words until a zero word; class 3 reads
 * script bytes until a zero byte; class 4 leaves the slot empty; address
 * 0x6B00 copies the active character's name (unresolved when not valid). */
pr_rec_ecl_window_status pr_rec_ecl_string_by_address(pr_rec_ecl_loader_state *state,
                                                      unsigned k, uint16_t address);

/* Unit 0x25 + 0x0F53 (GAME.OVR 0x08426): store a string at `address`: one
 * character per word plus a zero word (classes 0-2), bytes plus a zero byte
 * (class 3, needs script_rw), the active name for 0x6B00 (max 15), nothing
 * for class 4. len is capped at 255 as the engine's local string[255]. */
pr_rec_ecl_window_status pr_rec_ecl_store_string(pr_rec_ecl_loader_state *state,
                                                 uint16_t address, const char *text, size_t len);

/* Cores of unit 0x25 + 0x0E0C (load word) and + 0x0C77 (entry 10, store
 * word) for classes 0-3. Class 4 pseudo-addresses and the class-1 hooks
 * (+0x07BC on load, +0x0A74 on store, which mirror character-record fields)
 * are not modelled: load returns UNRESOLVED for class 4 and HOOK for class 1
 * (value read from the window); store returns HOOK after storing for class 1. */
pr_rec_ecl_window_status pr_rec_ecl_load_word(pr_rec_ecl_loader_state *state,
                                              uint16_t address, uint16_t *value);
pr_rec_ecl_window_status pr_rec_ecl_store_word(pr_rec_ecl_loader_state *state,
                                               uint16_t address, uint16_t value);

/* ---- Short ECL handlers (issue #34) --------------------------------------- */

typedef enum {
    PR_REC_ECL_HANDLER_OK = 0,
    PR_REC_ECL_HANDLER_ARGUMENT,
    PR_REC_ECL_HANDLER_BOUNDS,          /* operand loader hit the script bounds */
    PR_REC_ECL_HANDLER_STACK_FULL       /* model bound reached; the engine would allocate */
} pr_rec_ecl_handler_status;

/* Unit 0x25 + 0x1951 (GAME.OVR 0x08E24, public entry 17, called as 0x2B:0x75).
 * Path-complete callee model with a stated heap limit: the original allocates
 * a 6-byte heap node through the runtime, makes it the head of the call list
 * (first node has a null link; later nodes link to the previous head) and
 * stores the current PC in it; this model keeps the saved PCs in a bounded
 * array instead (PR_REC_ECL_CALL_STACK_MAX, status STACK_FULL). Then the PC
 * becomes the word assembled from operand slot `slot` (first value byte
 * [0x7085+slot], second [0x7045+slot]). */
pr_rec_ecl_handler_status pr_rec_ecl_push_call_frame(pr_rec_ecl_loader_state *state,
                                                     unsigned slot);

/* Unit 0x1c + 0x00F4 (GAME.OVR 0x00E3A), opcode 0x01 GOTO. Path-complete
 * handler model: its own 13 instructions are translated, but it composes with
 * the operand loader core (stated dependency). Loads one operand (0x2B:0x2A
 * with count 1), then stores the word assembled from slot 1 ([0x7086],
 * [0x7046]) into the PC. */
pr_rec_ecl_handler_status pr_rec_ecl_op_goto(pr_rec_ecl_loader_state *state);

/* Unit 0x1c + 0x0111 (GAME.OVR 0x00E57), opcode 0x02 GOSUB. Path-complete
 * handler model: its own 10 instructions are translated, composed with the
 * operand loader core and the bounded call-frame model (stated dependencies).
 * Loads one operand, then calls pr_rec_ecl_push_call_frame with slot 1. */
pr_rec_ecl_handler_status pr_rec_ecl_op_gosub(pr_rec_ecl_loader_state *state);

/* Unit 0x1c + 0x2AEB (GAME.OVR 0x03831), opcode 0x33 PRINT RETURN. Full
 * translation: PC += 1; then in both branches [0x5E24] = 1 and [0x5E25] += 1;
 * when [0x84DF] was non-zero it is cleared. No calls. */
pr_rec_ecl_handler_status pr_rec_ecl_op_print_return(pr_rec_ecl_loader_state *state);


/* Resets slots, string buffers and flags; keeps script and pc. */
void pr_rec_ecl_loader_reset(pr_rec_ecl_loader_state *state);

/* The state transition. On OK, pc points at the byte after the last operand
 * (the next opcode) exactly as the engine leaves [0x49ED]. */
pr_rec_ecl_loader_status pr_rec_ecl_load_operands(pr_rec_ecl_loader_state *state,
                                                  uint8_t count);

/* ---- Character-effect dispatch entry 8 (issue #39) ------------------------
 * GAME.OVR 0x0EFD2, unit 0x38 + 0x0377, public entry 8 (run-time stub
 * 0x41:0x48). Full translation, no calls, 53 bytes, 15 instructions.
 * Role evidence: unit 0x38 + 0x31A2 (entry 1) stores this entry's far pointer
 * into slots 8 and 45 of the far-pointer table at DS:0x6828 that the engine
 * invokes through `lcall [di + 0x6828]`; every table item is called with five
 * word arguments, which this routine discards (`retf 0xA`).
 * Behavior: reads the byte at offset 0xA0 of the active character record
 * (far pointer [0x5D92]); when it is 2, 5 or 8 it adds 2 to the byte [0x6816]
 * and subtracts 2 from the byte [0x6822]; otherwise nothing changes. Both
 * bytes wrap modulo 256 as the original `add/sub byte` do.
 * Provisional readings, not asserted by the code: the value sets {2,5,8} and
 * {0,3,6} (the sibling entry 9 at +0x03AC) match the evil and good columns of
 * the nine AD&D alignments, and the two bytes look like modifier accumulators
 * (16 and 24 writers in unit 0x38). */

#define PR_REC_CHAR_RECORD_SIZE 285u          /* verified record frame (docs/formats/DOS_CHARACTER.md) */
#define PR_REC_CHAR_OFFSET_A0 0xA0u

typedef struct {
    uint8_t b6816;                        /* engine byte [0x6816] */
    uint8_t b6822;                        /* engine byte [0x6822] */
    uint8_t b6818;                        /* engine byte [0x6818] (issue #46) */
    uint8_t b6819;                        /* engine byte [0x6819] (issue #46) */
} pr_rec_modifier_state;

typedef enum {
    PR_REC_EFFECT_OK = 0,                 /* branch taken or not; see applied */
    PR_REC_EFFECT_ARGUMENT                /* null state or null record: the engine would read through a null pointer */
} pr_rec_effect_status;

/* `record` must point at a full character record (PR_REC_CHAR_RECORD_SIZE
 * bytes). `applied`, if given, receives 1 when the modifiers were changed. */
pr_rec_effect_status pr_rec_u38_entry8(pr_rec_modifier_state *mods,
                                       const uint8_t *record, unsigned *applied);

/* GAME.OVR 0x0F007, unit 0x38 + 0x03AC, public entry 9 (run-time stub 0x41:0x4D).
 * Full translation, independently re-decoded (issue #43): 53 bytes, 15
 * instructions, no calls, `retf 0xA` (five discarded word arguments). It
 * differs from entry 8 in exactly three immediate bytes: the record byte at
 * offset 0xA0 is compared with 0, 3 and 6; on a match it adds 2 to [0x6816]
 * and subtracts 2 from [0x6822] (byte arithmetic). Installed at items 9 and
 * 46 of the far-pointer dispatch table at DS:0x6828 by unit 0x38 + 0x31A2.
 * Provisional reading, not asserted: {0,3,6} is the good column of the nine
 * AD&D alignments. Same contract as pr_rec_u38_entry8. */
pr_rec_effect_status pr_rec_u38_entry9(pr_rec_modifier_state *mods,
                                       const uint8_t *record, unsigned *applied);

/* GAME.OVR 0x0F03C, unit 0x38 + 0x03E1, public entry 10 (run-time stub 0x41:0x52),
 * dispatch item 10 of the table at DS:0x6828. Full translation: 31 bytes, 13
 * instructions, no calls, `retf 0xA` (five discarded word arguments). It reads
 * no character record. If bit 1 of [0x6819] is set: [0x6818] is replaced by
 * its value shifted right by one (zero-extended `shr ax,1`, low byte stored)
 * and 3 is added to [0x6816] (byte arithmetic); otherwise nothing changes.
 * Meanings of the three bytes are not asserted. */
pr_rec_effect_status pr_rec_u38_entry10(pr_rec_modifier_state *mods, unsigned *applied);

#endif
