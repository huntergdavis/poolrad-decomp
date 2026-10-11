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

#endif
