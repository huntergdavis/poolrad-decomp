/* Translations of three ECL helper functions from GAME.OVR overlay unit 0x25
 * (DOS v1.3, sha256 872964a3...928614). Each function mirrors the control flow
 * of the disassembled original; engine memory (script buffer, PC, Pascal
 * string buffers) is replaced by explicit arguments. See
 * docs/re/FUNCTION_LEDGER.md for the byte-level evidence and the checks. */

#include "poolrad/recovered/ecl_helpers.h"

uint8_t pr_rec_ecl_char_remap(uint8_t code) {
    /* cmp byte [bp+6],0x1F ; jbe -> add ax,0x40 (ah cleared first) ; return al */
    if (code <= 0x1Fu) {
        return (uint8_t)((unsigned)code + 0x40u);
    }
    return code;
}

unsigned pr_rec_ecl_operand_count(uint8_t opcode) {
    /* Transcribed from the compare chains of the dispatcher in the order the
     * engine tests them. Each chain ends in a call to the operand loader
     * with the count; the final else only increments the PC. */
    switch (opcode) {
    case 0x01: case 0x02: case 0x0A: case 0x0E: case 0x11: case 0x12:
    case 0x1D: case 0x20: case 0x2D: case 0x32: case 0x34: case 0x36:
    case 0x38: case 0x39: case 0x3C:
        return 1u;
    case 0x03: case 0x08: case 0x09: case 0x0C: case 0x0F: case 0x10:
    case 0x1F: case 0x22:
        return 2u;
    case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x0B: case 0x21: case 0x28: case 0x2A: case 0x2F: case 0x30:
    case 0x35: case 0x37: case 0x3B:
        return 3u;
    case 0x14: case 0x23:
        return 4u;
    case 0x2E:
        return 5u;
    case 0x1E: case 0x2C:
        return 6u;
    case 0x27:
        return 8u;
    case 0x29:
        return 14u;
    default:
        return 0u;
    }
}

size_t pr_rec_ecl_unpack_text(const uint8_t *packed, size_t packed_len,
                              char *out, size_t capacity) {
    /* Engine locals: phase (1..4), the current and previous script bytes,
     * the running count of consumed bytes, and the current six-bit code. */
    unsigned phase = 1u;
    uint8_t cur = 0u, prev = 0u, code = 0u;
    size_t consumed = 0u, produced = 0u;

    if (packed_len > 0u && packed == NULL) {
        return 0u;
    }
    while (consumed < packed_len) {
        if (phase < 4u) {
            prev = cur;
            cur = packed[consumed];
            consumed++;
        }
        if (phase == 1u) {
            code = (uint8_t)(cur >> 2);
        } else if (phase == 2u) {
            code = (uint8_t)((((unsigned)prev & 0x03u) << 4) | ((unsigned)cur >> 4));
        } else if (phase == 3u) {
            code = (uint8_t)((((unsigned)prev & 0x0Fu) << 2) | ((unsigned)cur >> 6));
        } else {
            code = (uint8_t)(cur & 0x3Fu);
        }
        if (phase < 4u) {
            phase++;
        } else {
            phase = 1u;
        }
        if (code != 0u) {
            if (produced < PR_REC_ECL_TEXT_MAX) {
                if (out != NULL && produced < capacity) {
                    out[produced] = (char)pr_rec_ecl_char_remap(code);
                }
                produced++;
            }
        }
    }
    if (phase == 4u) {
        /* The loop left the fourth code of the last group unread. */
        code = (uint8_t)(cur & 0x3Fu);
        if (code != 0u && produced < PR_REC_ECL_TEXT_MAX) {
            if (out != NULL && produced < capacity) {
                out[produced] = (char)pr_rec_ecl_char_remap(code);
            }
            produced++;
        }
    }
    return produced;
}
