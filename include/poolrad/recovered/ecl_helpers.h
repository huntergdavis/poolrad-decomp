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
#define PR_REC_ECL_SLOT_COUNT 64u           /* slots 1..63 modelled: the engine's tag/high/low arrays [0x7005+i], [0x7045+i],
                                               [0x7085+i] are 0x40 apart, so indices below 0x40 do not overlap. Fixed-count opcodes
                                               prefetch at most 14; ON GOTO/GOSUB (U1C +0x18F8) load a script-given count. A count
                                               of 64 or more is a model bound (LOADER_COUNT). */
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
    /* ---- Script-state initializer (issue #56, unit 0x25 + 0x01B6) --------
     * entry_addr[k] = engine word [0x49E3 + 2k]: the five entry addresses the
     * initializer reads from the block head (run, search, pre-camp check,
     * camp interrupted, initial entry; ECL_CONTRACTS.md section 1).
     * restored = byte [0x49F8]: the party menu sets it after a saved game is
     * loaded (unit 0x6c + 0x0412); the initializer then keeps the window
     * words it would otherwise clear and resets the flag. The other fields
     * are engine bytes and words the initializer writes, named by address
     * because their meaning is not asserted. */
    uint16_t entry_addr[5];
    uint8_t restored;
    uint8_t cmp_flags[6];                 /* [0x6F78..0x6F7D] */
    uint16_t w6f74;
    uint16_t w6f76;
    uint8_t b84e0;
    uint8_t b84e2;
    uint8_t b84d2;
    uint8_t b84d3;
    uint8_t b84e5;
    uint8_t b6f68;
    uint8_t b6f69;
    uint8_t b84c8;
    uint8_t b84c9;
    uint8_t b6f83;
    uint8_t b70c6;
    /* ---- ECL block load (issue #68, unit 0x25 + 0x0341) ------------------
     * file_number = byte [0x5376]: the digit of the ECL<n>.DAX name (1 from the
     * allocator, 3 from the party menu, restored by the saved-game loader).
     * b557c = byte [0x557C], cleared by the block load; its meaning is not
     * asserted (the resident file-open helper reads it). */
    uint8_t file_number;
    uint8_t b557c;
    /* ---- Run loop and dispatcher (issue #75, unit 0x1c + 0x3171 / + 0x2F2D) --
     * latch = byte [0x6F7F] (the opcode the dispatcher reads), prev_opcode =
     * byte [0x6F7E] (the previous latch, copied before each fetch), b442e =
     * byte [0x442E] (stop byte the run loop clears on entry and exit), b49ff =
     * byte [0x49FF] (the second stop byte, also tested by the gameplay loop). */
    uint8_t latch;
    uint8_t prev_opcode;
    uint8_t b442e;
    uint8_t b49ff;
    /* ---- LOAD FILES / LOAD PIECES handler (issue #92, unit 0x1c + 0x0C74) --
     * Four flag bytes the gameplay loop clears on entry ([0x4432] is set to 1
     * there) and the handler sets: [0x4431] on every call, [0x4434] on the
     * 0x21 branch, [0x4433] on the 0x37 branch; when all of [0x4432],
     * [0x4433], [0x4434] are set the handler runs two resident routines and
     * clears [0x4432]. piece_words[i-1] = the words [0x6AAE + 4i] and
     * [0x6AB0 + 4i], i = 1..3, which the default 0x37 path sets to 0xFFFF for
     * an operand of 0xFF. Meanings are not asserted. */
    uint8_t b4431;
    uint8_t b4432;
    uint8_t b4433;
    uint8_t b4434;
    uint16_t piece_words[3][2];
    /* ---- Class-4 pseudo-address sources of load word (unit 0x25 + 0x0E0C) ------
     * The data-segment words and bytes the class-4 branch reads: w6f6e = word
     * [0x6F6E] (address 0x00B1), w6f6a = [0x6F6A] (0x00FB), w6f6c = [0x6F6C]
     * (0x00FC); b6aaf = byte [0x6AAF] (0x033D, and 0xC04B+2 as half its value);
     * b6aad, b6aae = the party bytes [0x6AAD], [0x6AAE] (0xC04B+0, +1, sign
     * extended); b6ab0, b6ab1 = bytes [0x6AB0], [0x6AB1] (0xC04B+3, +4). Meanings
     * are not asserted; the host supplies whatever the engine holds. */
    uint16_t w6f6e;
    uint16_t w6f6a;
    uint16_t w6f6c;
    uint8_t b6aad;
    uint8_t b6aae;
    uint8_t b6aaf;
    uint8_t b6ab0;
    uint8_t b6ab1;
    /* ---- Store word class 0 side effect (unit 0x25 + 0x0C77, +0x0CA1..+0x0CAF) -------
     * byte [0x84E8]: set to 1 by a store word to window-0 address 0x49FD or 0x49FE (the
     * window write comes first); the model never clears it (the only other direct
     * store to it, unit 0x1c +0x2D42, writes 0). Its meaning is not asserted. */
    uint8_t b84e8;
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

/* Cores of unit 0x25 + 0x0E0C (load word) and + 0x0C77 (entry 10, store word).
 *
 * Store word: a store to window-0 address 0x49FD or 0x49FE also sets b84e8 := 1 after the
 * window write (+0x0CA1..+0x0CAF, docs/re/ECL_STORE_WORD_84E8.md).
 *
 * Load word: resolves nine class-4 pseudo-addresses from the state's
 * w6f6e/w6f6a/w6f6c/b6aad/b6aae/b6aaf/b6ab0/b6ab1 fields (0x00B1, 0x00FB, 0x00FC, 0x033D,
 * 0xC04B+0..4; docs/re/ECL_LOAD_WORD_CLASS4.md); every other class-4 address (0x035F,
 * 0xC04B+0xE and the rest) is UNRESOLVED, because the engine returns an uninitialized local
 * there. Store word's class-4 table is not modelled.
 *
 * Class-1 hooks (+0x07BC on load, +0x0A74 on store; they mirror character-record fields)
 * are not modelled in general: both cores report HOOK for class 1 after the window access,
 * except at the addresses below, each proven separately from the original bytes (the hook
 * subtracts 0x6B00 and compares that offset `rel` against its cases):
 *   load word returns OK at
 *     0x6DD5 (rel 0x2D5; docs/re/ECL_LOAD_HOOK_6DD5.md),
 *     0x6E7D (rel 0x37D; docs/re/ECL_HOOKS_6E7D.md),
 *     0x6E82 (rel 0x382; docs/re/ECL_LOAD_HOOK_6E82.md) and
 *     0x6DCA (rel 0x2CA; docs/re/ECL_LOAD_HOOK_6DCA.md; load only) and
 *     0x6E7F (rel 0x37F; docs/re/ECL_LOAD_HOOK_6E7F.md; load only):
 *     for each, every case compare in +0x07BC falls through, the hook clears its
 *     by-reference flag to 0 (+0x0A67) and reads and writes nothing else, and the caller
 *     (+0x0E41..+0x0E69) then reads the window word;
 *   store word: the hook's decision is classified for EVERY class-1 address by
 *     pr_rec_ecl_store_hook_may_effect (docs/re/ECL_STORE_HOOK_CLASSIFIER.md): the hook can write
 *     or call only for rel = 0x000, 0x020..0x070 and the 14 exact values 0xB8, 0xBB, 0xBD, 0xBF,
 *     0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312, 0x322, 0x324, 0x326 (96 addresses, whatever the
 *     value); for every other rel all compares fall through to the epilogue with no DS/ES
 *     game-state write and no call (it only adjusts its own stack argument), so the store word
 *     returns OK after the window write. The 96 potential-effect addresses report HOOK.
 * Load-word exceptions stay as listed above; every other class-1 load address reports HOOK. */
#define PR_REC_ECL_STORE_HOOK_EFFECT_COUNT 96u   /* the potential-effect addresses among the 1,024 class-1 addresses */
/* Nonzero when the store hook +0x0A74 may write game state or call for this address (class 1 is
 * 0x6B00..0x6EFF; an address outside it was never classified and also returns nonzero). Independent of
 * the value stored. */
int pr_rec_ecl_store_hook_may_effect(uint16_t address);
#define PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS 0x6DD5u
#define PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_B 0x6E7Du   /* proven the same way, docs/re/ECL_HOOKS_6E7D.md */
#define PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_C 0x6E82u   /* proven the same way, docs/re/ECL_LOAD_HOOK_6E82.md */
#define PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_D 0x6DCAu   /* proven the same way, docs/re/ECL_LOAD_HOOK_6DCA.md */
#define PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_E 0x6E7Fu   /* proven the same way, docs/re/ECL_LOAD_HOOK_6E7F.md */
pr_rec_ecl_window_status pr_rec_ecl_load_word(pr_rec_ecl_loader_state *state,
                                              uint16_t address, uint16_t *value);
pr_rec_ecl_window_status pr_rec_ecl_store_word(pr_rec_ecl_loader_state *state,
                                               uint16_t address, uint16_t value);

/* ---- Short ECL handlers (issue #34) --------------------------------------- */

typedef enum {
    PR_REC_ECL_HANDLER_OK = 0,
    PR_REC_ECL_HANDLER_ARGUMENT,
    PR_REC_ECL_HANDLER_BOUNDS,          /* operand loader hit the script bounds */
    PR_REC_ECL_HANDLER_STACK_FULL,      /* model bound reached; the engine would allocate */
    PR_REC_ECL_HANDLER_HOOK,            /* a class-1 window access was done but the engine also runs
                                         * an un-modelled hook (issue #84) */
    PR_REC_ECL_HANDLER_UNRESOLVED       /* a path needs engine state the model lacks; the write that
                                         * depends on it was not made (issue #84) */
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

/* GAME.OVR 0x0F870, unit 0x38 + 0x0C15, public entry 31 (run-time stub 0x41:0xBB),
 * dispatch item 36 of the table at DS:0x6828. Full translation: 17 bytes, 6
 * instructions, straight line, no calls, `retf 0xA` (five discarded word
 * arguments): `sub byte [0x6822],4` then `sub byte [0x6816],4`, both wrapping
 * modulo 256. Meanings of the two bytes are not asserted. */
pr_rec_effect_status pr_rec_u38_entry31(pr_rec_modifier_state *mods);

/* GAME.OVR 0x0F861, unit 0x38 + 0x0C06, public entry 30 (run-time stub 0x41:0xB6),
 * dispatch item 35 of the table at DS:0x6828. Full translation: 15 bytes, 6
 * instructions, straight line, no calls, `retf 0xA` (five discarded word
 * arguments): `inc byte [0x6816]` then `inc byte [0x6822]`, both wrapping
 * modulo 256. Meanings of the two bytes are not asserted. */
pr_rec_effect_status pr_rec_u38_entry30(pr_rec_modifier_state *mods);

/* ---- Issue #56: two routines on the first-gameplay path --------------------
 * See docs/re/GAMEPLAY_PATH.md and FUNCTION_LEDGER.md section 1.14. */

/* GAME.OVR 0x2EA4C, unit 0x3bd + 0x0155 (bytes +0x0155..+0x0173), public
 * entry 1 (run-time stub 0x3D5:0x25). Full translation: 31 bytes, 12
 * instructions, straight line, no calls, `retf 8`. Copies four byte
 * arguments, first pushed ([bp+0xC]) to last pushed ([bp+6]), to the bytes
 * [0x6A60], [0x6A61], [0x6A62], [0x6A63]. Callers: the program body (image
 * 0x0138, constants 0x0B, 0, 7, 6) right before the gameplay loop, and the
 * view refresh unit 0x3b4 + 0x016D (site +0x0207) with computed values. The
 * meanings of the four bytes are not asserted. */
typedef struct {
    uint8_t b6a60;
    uint8_t b6a61;
    uint8_t b6a62;
    uint8_t b6a63;
} pr_rec_view_state;

pr_rec_effect_status pr_rec_u3bd_entry1(pr_rec_view_state *view, uint8_t first,
                                        uint8_t second, uint8_t third, uint8_t fourth);

/* GAME.OVR 0x07689, unit 0x25 + 0x01B6 (bytes +0x01B6..+0x0321), public entry 3
 * (run-time stub 0x2B:0x2F), 364 bytes, 135 instructions. Called by the
 * gameplay loop (unit 0x1c + 0x335D) right after the ECL block load and before
 * the first run of the initial entry. Path-complete model with stated
 * dependencies: the two runtime calls at 0xAF8:0x16A8 (FillChar's argument
 * shape: destination, count, value 0) become direct zeroing; the five
 * operand reads go through the loader core (function 6) with count 1 and the
 * operand word assembly (function 4), so the model shares the loader's
 * limits. Window words are addressed as in the engine: window 0 index i is
 * script address 0x4900 + i, window 1 index i is 0x6B00 + i. */
#define PR_REC_ECL_INIT_CLEAR0_FIRST 0x100u   /* window 0: script addresses 0x4A00..0x4A1F */
#define PR_REC_ECL_INIT_CLEAR0_COUNT 32u
#define PR_REC_ECL_INIT_CLEAR1_FIRST 0x379u   /* window 1: script addresses 0x6E79..0x6E82 */
#define PR_REC_ECL_INIT_CLEAR1_COUNT 10u

pr_rec_ecl_handler_status pr_rec_ecl_script_init(pr_rec_ecl_loader_state *state);

/* ---- Issue #68: ECL block load, unit 0x25 + 0x0341 ------------------------
 * GAME.OVR 0x07814, unit 0x25 + 0x0341 (bytes +0x0341..+0x0410), public entry 4
 * (run-time stub 0x2B:0x34), 208 bytes, 89 instructions, one backward branch
 * (+0x03DC `jb +0x035F`), 12 call sites, `retf 2` (one byte argument: the block
 * id, which the gameplay loop passes as the map id). Path-complete model: the
 * routine's own instructions are translated; every external call crosses one
 * of the boundaries below. See docs/re/ECL_BLOCK_LOAD.md. */
#define PR_REC_ECL_SCRIPT_BUFFER_SIZE 0x1E00u   /* fill length at +0x034E; the engine never checks it on copy */
#define PR_REC_ECL_BLOCK_NAME_MAX 8u            /* "ECL" + up to three digits + NUL */
#define PR_REC_ECL_BLOCK_EXT ".dax"             /* the 4-character constant at +0x033C */

typedef struct {
    void *ctx;
    /* Resident 0x802:0x0716 (RES_0802_0716), called at +0x03D3 with (name base,
     * extension, block id, out size, out block). The engine passes two Pascal
     * strings; here NUL-terminated ASCII. On a missing file or id the resident
     * routine returns size 0 and a null block; a non-zero return here is
     * treated the same way. The block stays owned by the callee until
     * release() is called with the same pointer and size. */
    int (*load_block)(void *ctx, const char *name, const char *ext, uint8_t block_id,
                      uint16_t *size, const uint8_t **block);
    /* Runtime 0xAF8:0x0254 at +0x0401 with (block, size): FreeMem's argument shape. */
    void (*release)(void *ctx, const uint8_t *block, uint16_t size);
    /* Unit 0x3b0 entry 3 at +0x035F and +0x0406 (clears a text row through
     * resident 0x709:0x05EB with 0, 0x18, 0, 0x28); optional. */
    void (*clear_row)(void *ctx);
    /* The wait message at +0x0364..+0x037C (a 21-character constant through
     * resident 0x709:0x0626 at row 0x18, column 0x0F); optional, no text passed. */
    void (*show_wait)(void *ctx);
} pr_rec_ecl_block_io;

typedef enum {
    PR_REC_ECL_BLOCK_OK = 0,
    PR_REC_ECL_BLOCK_ARGUMENT,
    PR_REC_ECL_BLOCK_RETRY_LIMIT,   /* the engine loops at +0x035F while size < 2; the model bounds it */
    PR_REC_ECL_BLOCK_OVERFLOW       /* size - 2 exceeds the buffer: the engine would copy past it;
                                     * the model copies nothing, still releases and clears */
} pr_rec_ecl_block_status;

/* Writes "ECL" followed by the decimal of file_number (Str with width 0). */
void pr_rec_ecl_block_name(uint8_t file_number, char name[PR_REC_ECL_BLOCK_NAME_MAX]);

/* buffer must hold PR_REC_ECL_SCRIPT_BUFFER_SIZE bytes (the engine's [0x49DE]
 * buffer). On OK the state's script pointers are bound to it. attempts counts
 * load_block calls (optional). */
pr_rec_ecl_block_status pr_rec_ecl_load_block(pr_rec_ecl_loader_state *state, uint8_t *buffer,
                                              uint8_t block_id, const pr_rec_ecl_block_io *io,
                                              unsigned max_attempts, unsigned *attempts);

/* ---- Issue #75: ECL run loop and opcode dispatcher (model) ------------------
 * Run function: GAME.OVR 0x03EB7, unit 0x1c + 0x3171 (bytes +0x3171..+0x31B2),
 * 66 bytes, 22 instructions, one near call (+0x2F2D), `retf 2` at +0x31B0 (one word
 * argument: the entry address). Dispatcher: GAME.OVR 0x03C73, unit 0x1c +
 * 0x2F2D (bytes +0x2F2D..+0x3170), 580 bytes, 273 instructions, a compare
 * chain on the latch [0x6F7F] for 0x00..0x3D (0x1F has no case) calling 49
 * distinct near handlers, `retf`. Both are translated instruction for
 * instruction; the handler bodies are callbacks, so this is a model. */
#define PR_REC_ECL_OPCODE_COUNT 0x3Eu          /* compare chain covers 0x00..0x3D */
#define PR_REC_ECL_FETCH_BIAS 0x6700u          /* es:[di + 0x6700] with di = buffer offset + PC */

/* Handler offset inside unit 0x1c for each opcode as the compare chain calls
 * it; 0 marks the opcode without a case (0x1F). Transcribed from +0x2F2D. */
extern const uint16_t pr_rec_ecl_handler_offset[PR_REC_ECL_OPCODE_COUNT];

typedef pr_rec_ecl_handler_status (*pr_rec_ecl_handler_fn)(pr_rec_ecl_loader_state *state, void *ctx);

typedef struct {
    void *ctx;
    pr_rec_ecl_handler_fn handler[PR_REC_ECL_OPCODE_COUNT];   /* NULL = body not modelled */
} pr_rec_ecl_dispatch_table;

typedef enum {
    PR_REC_ECL_RUN_OK = 0,          /* stopped by [0x442E] or [0x49FF]; dispatcher: a case ran */
    PR_REC_ECL_RUN_ARGUMENT,
    PR_REC_ECL_RUN_NO_CASE,         /* opcode 0x1F or above 0x3D: the chain falls through to the
                                     * epilogue with nothing done; the engine's loop would then
                                     * re-fetch the same byte forever */
    PR_REC_ECL_RUN_UNMODELLED,      /* the opcode has a case but its handler slot is NULL */
    PR_REC_ECL_RUN_HANDLER,         /* a handler returned a non-OK status (see last_handler_status) */
    PR_REC_ECL_RUN_FETCH_BOUNDS,    /* the PC addresses a byte outside the script */
    PR_REC_ECL_RUN_STEP_LIMIT       /* model bound; the engine has none */
} pr_rec_ecl_run_status;

/* The dispatcher body (+0x2F2D): reads state->latch and calls the matching
 * handler slot. shared handlers (0x04..0x07, 0x11/0x12, 0x16..0x1B, 0x21/0x37,
 * 0x25/0x26, 0x2F/0x30) are the caller's business, as in the engine, where the
 * handler branches on the latch. */
pr_rec_ecl_run_status pr_rec_ecl_dispatch(pr_rec_ecl_loader_state *state,
                                          const pr_rec_ecl_dispatch_table *table,
                                          pr_rec_ecl_handler_status *handler_status);

/* The run function (+0x3171): pc = entry; [0x442E] = 0; while both stop bytes
 * are 0: prev_opcode = latch; latch = script byte at pc; dispatch. On exit
 * [0x442E] = 0. steps counts dispatches (optional). Any non-OK dispatch result
 * ends the model where the engine would continue or spin. */
pr_rec_ecl_run_status pr_rec_ecl_run(pr_rec_ecl_loader_state *state,
                                     const pr_rec_ecl_dispatch_table *table, uint16_t entry,
                                     unsigned max_steps, unsigned *steps,
                                     pr_rec_ecl_handler_status *handler_status);

/* ---- Issue #84: slot value and the SAVE handler (models) -------------------
 * Slot value: GAME.OVR 0x07609, unit 0x25 + 0x0136 (bytes +0x0136..+0x01B5),
 * public entry 1 (run-time stub 0x2B:0x25, 52 fixup-confirmed sites in unit
 * 0x1c), 128 bytes, 57 instructions, `retf 2` (one word argument, the slot
 * index in its low byte), result in AX. Reads tag [0x7005+slot], low
 * [0x7085+slot], high [0x7045+slot]:
 *   tag 0x00            -> the low byte, zero-extended;
 *   tags 0x01,0x03,0x80 -> load word (+0x0E0C) at (high << 8) + low;
 *   tags 0x02,0x81      -> (high << 8) + low through +0x06AF;
 *   any other tag       -> the local [bp-2] is returned uninitialized.
 * Path-complete model: the load-word core carries its own HOOK/UNRESOLVED
 * statuses; the uninitialized case reports UNRESOLVED with value 0. */
pr_rec_ecl_window_status pr_rec_ecl_slot_value(pr_rec_ecl_loader_state *state, uint8_t slot,
                                               uint16_t *value);

/* SAVE (opcode 0x09): GAME.OVR 0x00FD8, unit 0x1c + 0x0292 (bytes +0x0292..
 * +0x02DF), 78 bytes, 30 instructions, called only from the dispatcher case
 * at +0x2F86, `retf`. loader(2); dest = word(slot 2) through +0x06AF; if
 * tag[1] < 0x80: value = slot value(1), store word (+0x0C77) (dest, value);
 * else: store string (+0x0F53) of the first string buffer (DS:0x70C8) at
 * dest. Two paths. Path-complete model composed of the loader core, the slot
 * value model, the store-word core and the store-string translation; when the
 * value is UNRESOLVED the store is not made (the engine would store an
 * unmodelled value). store_status receives the window status of the store
 * (optional). */
pr_rec_ecl_handler_status pr_rec_ecl_op_save(pr_rec_ecl_loader_state *state,
                                             pr_rec_ecl_window_status *store_status);

/* ---- Issue #92: LOAD FILES / LOAD PIECES handler (model) --------------------
 * GAME.OVR 0x019BA, unit 0x1c + 0x0C74 (bytes +0x0C74..+0x0DEE), opcodes 0x21
 * and 0x37 (dispatcher cases at +0x3054 and +0x3058 share it), 379 bytes, 129
 * instructions, `retf`, no near calls, 11 far-call sites. Paths: loader with
 * count 3; [0x4431] = 1; the three slot values (function 21) truncated to
 * bytes v1..v3; branch on the latch == 0x21:
 *   0x21: [0x4434] = 1; if v1 not 0xFF, not 0x7F and window 0 word 0xE6 != 0:
 *         window 0 word 0xC5 = v1, unit 0x3bd entry 7 (v1), window 1 word
 *         0x2C9 = 0; if v3 != 0xFF and window 0 word 0xE6 == 0: unit 0x3ce
 *         entry 1 with the 6-character constant at cs:0x0C6D, 0, 0x28.
 *   else: [0x4433] = 1; if v1 == 0x7F: unit 0x3bd entry 6 (0, 1); else if
 *         window 0 words 0xE7 and 0xE8 are both non-zero: entry 6 (v1, 1) and
 *         entry 6 (v3, 3); else for i = 1..3: entry 6 (vi, i) when vi != 0xFF,
 *         otherwise the two piece words of slot i are set to 0xFFFF.
 *   then: if [0x4432], [0x4433] and [0x4434] are all set: resident 0xBA:0x27D9,
 *         resident 0xBA:0x0767 with the active-character pointer, [0x4432] = 0.
 * Path-complete model: the loaders, the text routine and the two resident
 * routines are callbacks. Each is required at the point its site is reached:
 * a NULL callback there returns HANDLER_UNRESOLVED after the writes that
 * precede the site and before the effects that follow it, so a run cannot
 * report a completed route with DOS work omitted. A branch that reaches no
 * site needs no callback. The loader core and function 21 are dependencies.
 * An UNRESOLVED or BOUNDS slot value returns before the branch-specific
 * effects (the operand load and [0x4431] have already happened; the engine
 * would continue with an unmodelled byte). */
typedef struct {
    void *ctx;
    void (*load_wallset)(void *ctx, uint8_t id);               /* unit 0x3bd entry 7 (+0x10C7), `retf 2`; required when reached */
    void (*load_pieces)(void *ctx, uint8_t id, uint8_t slot);  /* unit 0x3bd entry 6 (+0x0E08), `retf 4`, (id, slot) as pushed */
    void (*show_text)(void *ctx);                              /* unit 0x3ce entry 1 (+0x0022) with the constant, 0, 0x28 */
    void (*resident_27d9)(void *ctx);                          /* 0xBA:0x27D9 (RES_00BA_27D9), no arguments */
    void (*resident_0767)(void *ctx);                          /* 0xBA:0x0767 (RES_00BA_0767) with [0x5D92] */
} pr_rec_ecl_load_files_io;

pr_rec_ecl_handler_status pr_rec_ecl_op_load_files(pr_rec_ecl_loader_state *state,
                                                   const pr_rec_ecl_load_files_io *io);

/* ---- Issue #102: COMPARE, its two flag helpers, and the IF family ----------
 * Flags: the six bytes [0x6F78..0x6F7D] (state->cmp_flags[0..5]) in the order
 * equal, not equal, below, above, below or equal, above or equal, each 0 or 1.
 *
 * Word compare: GAME.OVR 0x08DBE, unit 0x25 + 0x18EB (bytes +0x18EB..+0x1950),
 * public entry 16 (run-time stub 0x2B:0x70), 102 bytes, 36 instructions,
 * `retf 4`: two word arguments, a at [bp+8] (pushed first), b at [bp+6].
 * Clears the six flags through 0xAF8:0x16A8 with (DS:0x6F78, 6, 0) (FillChar's
 * argument shape), then six unsigned compares of a against b set the flags.
 * Every instruction translated; the runtime call was decoded independently
 * by the PM (loaded image 0x0C628: rep stosb, retf 8), i.e. FillChar.
 *
 * String compare: GAME.OVR 0x08CE8, unit 0x25 + 0x1815 (bytes +0x1815..+0x18EA),
 * public entry 15 (run-time stub 0x2B:0x6B), 214 bytes, 86 instructions,
 * `retf 8`: two far Pascal-string arguments, a at [bp+0xA] (pushed first) and
 * b at [bp+6], each copied to a 255-byte local (0xAF8:0x0B25), the flags
 * cleared as above, then six calls of the runtime string compare 0xAF8:0x0C10
 * on (a, b) with the same six conditions. The copies happen before the
 * clear, so an input aliasing the flag bytes is compared as on entry. The
 * runtime routines were decoded independently by the PM (0x0BAA5: copy
 * min(length, limit) bytes, retf 0xA; 0x0BB90: compare lengths, repe cmpsb
 * unsigned, then length compare), which is the ordering implemented here.
 *
 * COMPARE (opcode 0x03): GAME.OVR 0x00E6C, unit 0x1c + 0x0126 (bytes +0x0126..
 * +0x01A2), 125 bytes, 50 instructions, `retf`, dispatcher case +0x2F59.
 * loader(2); if tag[1] >= 0x80 or tag[2] >= 0x80: string compare of string
 * buffer 1 (DS:0x70C8) against buffer 2 (DS:0x71C8) as they stand after the
 * load (a lone string operand fills buffer 1 whatever its position; buffer 2
 * then holds what an earlier instruction left); else word compare of slot
 * value 1 against slot value 2. Model (loader core and function 21). In the
 * word path a slot value that is not OK (HOOK from a class-1 load hook,
 * UNRESOLVED, BOUNDS) returns at once: slot 2 is not read after a slot-1
 * stop and the six flags are left as they were.
 *
 * IF family (opcodes 0x16..0x1B): GAME.OVR 0x01898, unit 0x1c + 0x0B52 (bytes
 * +0x0B52..+0x0BC7), 118 bytes, 41 instructions, `retf`, dispatcher ladder
 * +0x3009..+0x3022. PC += 1; the opcode selects the flag byte (0x16 -> equal
 * ... 0x1B -> above or equal); when that byte is 0 the next statement is
 * skipped through unit 0x25 entry 22 (0x2B:0x8E): the opcode at the PC is
 * fetched into the latch [0x6F7F], its prefetch count (function 2) drives
 * one loader call, or the PC advances by one when the count is 0. Model
 * (function 2 core and the loader core). */
pr_rec_ecl_handler_status pr_rec_ecl_compare_words(pr_rec_ecl_loader_state *state, uint16_t a, uint16_t b);
pr_rec_ecl_handler_status pr_rec_ecl_compare_strings(pr_rec_ecl_loader_state *state,
                                                     const char *a, size_t a_len,
                                                     const char *b, size_t b_len);
pr_rec_ecl_handler_status pr_rec_ecl_op_compare(pr_rec_ecl_loader_state *state);
pr_rec_ecl_handler_status pr_rec_ecl_op_if(pr_rec_ecl_loader_state *state);

/* ---- Issue #113: RETURN (opcode 0x13) ---------------------------------------
 * GAME.OVR 0x017E5, unit 0x1c + 0x0A9F (bytes +0x0A9F..+0x0AEC), 78 bytes, 25
 * instructions, `retf`, dispatcher case +0x2FED, no near calls, one far call.
 * PC += 1; if the call-list head [0x6F70:0x6F72] is nil: return; else PC =
 * head word 0 (the PC the GOSUB saved: the address after its operand), the
 * temporaries [0x6F74:0x6F76] = head words 1-2 (the link to the previous
 * node), FreeMem(head, 6) through 0xAF8:0x0254, head = the link. Two paths.
 * Path-complete model over the bounded array that function 7 uses for the
 * list (depth 0 = nil head; the node release is implicit; the temporaries
 * are not modelled, as for function 7). An invalid host depth above the
 * array bound returns BOUNDS after the PC advance, before any array read. */
pr_rec_ecl_handler_status pr_rec_ecl_op_return(pr_rec_ecl_loader_state *state);

#endif
