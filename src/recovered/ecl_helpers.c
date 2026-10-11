/* One full translation and two extracted behavior cores from GAME.OVR overlay unit 0x25
 * (DOS v1.3, sha256 872964a3...928614). The remap mirrors its original
 * instruction for instruction; the other two reproduce only the verified
 * decision table or decoding loop, with engine memory (script buffer, PC,
 * Pascal string buffers, runtime calls) replaced by explicit arguments and
 * their side effects omitted. See docs/re/FUNCTION_LEDGER.md. */

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

uint16_t pr_rec_ecl_operand_word(uint8_t low, uint8_t high) {
    /* mov al,[bp+8]; xor ah,ah; mov dx,ax ; mov al,[bp+6]; xor ah,ah ;
     * shl ax,8 ; add ax,dx ; return ax                                 */
    return (uint16_t)(((unsigned)high << 8) + (unsigned)low);
}

uint8_t pr_rec_ecl_address_class(uint16_t address) {
    /* Local starts at 4; each range test rewrites it when it matches. */
    uint8_t result = 4u;
    if (address >= 0x4900u && address <= 0x4CFFu) {
        result = 0u;
    }
    if (address >= 0x6B00u && address <= 0x6EFFu) {
        result = 1u;
    }
    if (address >= 0x9700u && address <= 0x98FFu) {
        result = 2u;
    }
    if (address >= 0x9900u && address <= 0xB6FFu) {
        result = 3u;
    }
    return result;
}

/* ---- Operand loader (unit 0x25 + 0x0000) ---------------------------------- */

void pr_rec_ecl_loader_reset(pr_rec_ecl_loader_state *state) {
    unsigned i;
    if (state == NULL) {
        return;
    }
    for (i = 0; i < PR_REC_ECL_SLOT_COUNT; ++i) {
        state->tag[i] = 0u;
        state->low[i] = 0u;
        state->high[i] = 0u;
        state->string_len[i] = 0u;
        state->string_address[i] = 0u;
        state->string_unresolved[i] = 0u;
    }
    state->string_count = 0u;
}

/* Reads the script byte at virtual address `address`; returns -1 when the
 * address is outside the supplied buffer (the engine has no such check). */
static int loader_read(const pr_rec_ecl_loader_state *state, uint16_t address) {
    size_t index;
    if (address < PR_REC_ECL_SCRIPT_BASE) {
        return -1;
    }
    index = (size_t)(address - PR_REC_ECL_SCRIPT_BASE);
    if (index >= state->script_size) {
        return -1;
    }
    return state->script[index];
}

/* Appends one character to string buffer k as the runtime concat/store
 * sequence would, including the provisional 255-character cap. */
static void loader_append(pr_rec_ecl_loader_state *state, unsigned k, char ch) {
    if (state->string_len[k] < PR_REC_ECL_TEXT_MAX) {
        state->string_text[k][state->string_len[k]] = ch;
        state->string_len[k]++;
    }
}

/* Unit 0x25 + 0x12FB, string by address, class 3 branch only: raw bytes from
 * the script window up to a zero byte. Other classes are recorded as
 * unresolved (engine word arrays at [0x49D2], [0x49D6], [0x49DA] and the
 * [0x5D92] string are not modelled). */
static pr_rec_ecl_loader_status loader_string_by_address(pr_rec_ecl_loader_state *state,
                                                         unsigned k, uint16_t address) {
    uint8_t cls = pr_rec_ecl_address_class(address);
    state->string_len[k] = 0u;
    state->string_address[k] = address;
    if (cls == 3u) {
        uint16_t a = address;
        for (;;) {
            int b = loader_read(state, a);
            if (b < 0) {
                return PR_REC_ECL_LOADER_BOUNDS;
            }
            if (b == 0) {
                break;
            }
            loader_append(state, k, (char)b);
            a++;
        }
    } else if (cls != 4u) {
        state->string_unresolved[k] = 1u;
    }
    return PR_REC_ECL_LOADER_OK;
}

pr_rec_ecl_loader_status pr_rec_ecl_load_operands(pr_rec_ecl_loader_state *state,
                                                  uint8_t count) {
    unsigned i;
    if (state == NULL || (state->script == NULL && state->script_size != 0u)) {
        return PR_REC_ECL_LOADER_ARGUMENT;
    }
    if (count >= PR_REC_ECL_SLOT_COUNT) {
        return PR_REC_ECL_LOADER_COUNT;
    }
    state->string_count = 0u;                 /* mov byte [bp-2],0 */
    /* mov al,1 ; cmp al,count ; jbe loop ; jmp end */
    for (i = 1u; i <= count; ++i) {
        int t = loader_read(state, (uint16_t)(state->pc + 1u));
        int v = loader_read(state, (uint16_t)(state->pc + 2u));
        if (t < 0 || v < 0) {
            return PR_REC_ECL_LOADER_BOUNDS;
        }
        state->tag[i] = (uint8_t)t;           /* [0x7005+i] */
        state->low[i] = (uint8_t)v;           /* [0x7085+i] */
        state->pc = (uint16_t)(state->pc + 2u);
        if (t == 1 || t == 2 || t == 3) {
            int hb;
            state->pc++;
            hb = loader_read(state, state->pc);
            if (hb < 0) {
                return PR_REC_ECL_LOADER_BOUNDS;
            }
            state->high[i] = (uint8_t)hb;     /* [0x7045+i] */
        } else if (t == 0x80) {
            unsigned k = ++state->string_count;
            unsigned len = state->low[i];
            if (len > 0u) {
                /* the unpacker consumes len bytes, advancing the PC before each read */
                size_t start = (size_t)(state->pc + 1u);
                if (state->pc + len < PR_REC_ECL_SCRIPT_BASE ||
                    (size_t)(state->pc + len - PR_REC_ECL_SCRIPT_BASE) >= state->script_size) {
                    return PR_REC_ECL_LOADER_BOUNDS;
                }
                state->string_len[k] = (uint8_t)pr_rec_ecl_unpack_text(
                    state->script + (start - PR_REC_ECL_SCRIPT_BASE), len,
                    state->string_text[k], PR_REC_ECL_TEXT_MAX);
                state->pc = (uint16_t)(state->pc + len);
            } else {
                state->string_len[k] = 0u;    /* mov byte [0x6FC8 + 256k],0 */
            }
        } else if (t == 0x81) {
            unsigned k = ++state->string_count;
            int hb;
            pr_rec_ecl_loader_status st;
            state->pc++;
            hb = loader_read(state, state->pc);
            if (hb < 0) {
                return PR_REC_ECL_LOADER_BOUNDS;
            }
            state->high[i] = (uint8_t)hb;
            st = loader_string_by_address(state, k,
                                          pr_rec_ecl_operand_word(state->low[i], state->high[i]));
            if (st != PR_REC_ECL_LOADER_OK) {
                return st;
            }
        }
        /* any other tag (including 0x00): nothing more */
    }
    state->pc++;                              /* inc word [0x49ED] */
    return PR_REC_ECL_LOADER_OK;
}
