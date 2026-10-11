#ifndef POOLRAD_RECOVERED_ECL_HELPERS_H
#define POOLRAD_RECOVERED_ECL_HELPERS_H

/* Portable C translations of three small ECL helper functions recovered from
 * the DOS v1.3 GAME.OVR overlay unit 0x25. Provenance, verification status,
 * and the engine-side details that were deliberately not translated are in
 * docs/re/FUNCTION_LEDGER.md. No game data is embedded here. */

#include <stddef.h>
#include <stdint.h>

/* Engine string buffers are Turbo Pascal string[255]; the unpacker's output
 * cannot exceed this many characters. */
#define PR_REC_ECL_TEXT_MAX 255u

/* GAME.OVR 0x07BA6 (unit 0x25 + 0x06D3). Six-bit text code to ASCII:
 * codes 0x00..0x1F map to code + 0x40, all other values pass through.
 * The engine computes the sum in a 16-bit register and returns the low byte. */
uint8_t pr_rec_ecl_char_remap(uint8_t code);

/* GAME.OVR 0x09379 (unit 0x25 + 0x1EA6), the operand-count dispatcher.
 * Returns the number of tagged operands the engine prefetches for an opcode
 * before dispatching its handler: 1, 2, 3, 4, 5, 6, 8 or 14. Returns 0 for
 * every other opcode value, where the engine only advances the PC by one.
 * Menus and ON GOTO/GOSUB (0x15, 0x25, 0x26, 0x2B) return 0 here because
 * their handlers read their own operand lists. */
unsigned pr_rec_ecl_operand_count(uint8_t opcode);

/* GAME.OVR 0x08626 (unit 0x25 + 0x1153), the six-bit text unpacker.
 * Consumes exactly packed_len script bytes and emits one character per
 * non-zero six-bit code (four codes per three bytes, MSB first, each code
 * remapped with pr_rec_ecl_char_remap). Zero codes are skipped, not treated
 * as terminators. The fourth code of a group is emitted only when the final
 * byte completes a three-byte group. At most PR_REC_ECL_TEXT_MAX characters
 * are produced; later characters are dropped as the engine's string[255]
 * would drop them. Writes up to capacity characters into out (no NUL) and
 * returns the number of characters the engine would have produced. */
size_t pr_rec_ecl_unpack_text(const uint8_t *packed, size_t packed_len,
                              char *out, size_t capacity);

#endif
