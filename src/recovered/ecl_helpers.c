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

/* ---- engine windows (issue #22) ------------------------------------------ */

void pr_rec_ecl_windows_clear(pr_rec_ecl_loader_state *state) {
    unsigned i;
    if (state == NULL) {
        return;
    }
    state->call_depth = 0u;                   /* initializer: [0x6F70] starts null */
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) state->window0[i] = 0u;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) state->window1[i] = 0u;
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) state->window2[i] = 0u;
    /* the initializer also stores a null far pointer to [0x5D92]: no active name */
    for (i = 0; i < PR_REC_ECL_NAME_MAX; ++i) state->active_name[i] = 0;
    state->active_name_len = 0u;
    state->active_name_valid = 0u;
    pr_rec_ecl_loader_reset(state);
}

/* Window word for an address of class 0, 1 or 2, or NULL when the address
 * would index past the array the engine allocated. The engine computes
 * base + 2*address + bias with 16-bit wraparound, which is exactly
 * element (address - window_base). */
static uint16_t *window_word(pr_rec_ecl_loader_state *state, uint8_t cls, uint16_t address) {
    switch (cls) {
    case 0u:
        return address >= 0x4900u && address - 0x4900u < PR_REC_ECL_WINDOW0_WORDS ? &state->window0[address - 0x4900u] : NULL;
    case 1u:
        return address >= 0x6B00u && address - 0x6B00u < PR_REC_ECL_WINDOW1_WORDS ? &state->window1[address - 0x6B00u] : NULL;
    case 2u:
        return address >= 0x9700u && address - 0x9700u < PR_REC_ECL_WINDOW2_WORDS ? &state->window2[address - 0x9700u] : NULL;
    default:
        return NULL;
    }
}

pr_rec_ecl_window_status pr_rec_ecl_string_by_address(pr_rec_ecl_loader_state *state,
                                                      unsigned k, uint16_t address) {
    uint8_t cls;
    if (state == NULL || k == 0u || k >= PR_REC_ECL_SLOT_COUNT) {
        return PR_REC_ECL_WINDOW_ARGUMENT;
    }
    cls = pr_rec_ecl_address_class(address);
    state->string_len[k] = 0u;                /* mov byte [0x6FC8 + 256k],0 */
    state->string_address[k] = address;
    state->string_unresolved[k] = 0u;
    if (cls == 1u && address == 0x6B00u) {
        unsigned i;
        if (!state->active_name_valid) {
            state->string_unresolved[k] = 1u;
            return PR_REC_ECL_WINDOW_UNRESOLVED;
        }
        for (i = 0; i < state->active_name_len && i < PR_REC_ECL_NAME_MAX; ++i) {
            loader_append(state, k, state->active_name[i]);
        }
        return PR_REC_ECL_WINDOW_OK;
    }
    if (cls <= 2u) {
        uint16_t a = address;
        for (;;) {
            const uint16_t *w = window_word(state, cls, a);
            if (w == NULL) {
                return PR_REC_ECL_WINDOW_BOUNDS;
            }
            if (*w == 0u) {
                break;
            }
            loader_append(state, k, (char)(*w & 0xFFu));   /* low byte of the word */
            a++;
        }
        return PR_REC_ECL_WINDOW_OK;
    }
    if (cls == 3u) {
        uint16_t a = address;
        for (;;) {
            int b = loader_read(state, a);
            if (b < 0) {
                return PR_REC_ECL_WINDOW_BOUNDS;
            }
            if (b == 0) {
                break;
            }
            loader_append(state, k, (char)b);
            a++;
        }
        return PR_REC_ECL_WINDOW_OK;
    }
    return PR_REC_ECL_WINDOW_OK;              /* class 4: slot stays empty */
}

pr_rec_ecl_window_status pr_rec_ecl_store_string(pr_rec_ecl_loader_state *state,
                                                 uint16_t address, const char *text, size_t len) {
    uint8_t cls;
    size_t i;
    if (state == NULL || (text == NULL && len != 0u)) {
        return PR_REC_ECL_WINDOW_ARGUMENT;
    }
    if (len > PR_REC_ECL_TEXT_MAX) {
        len = PR_REC_ECL_TEXT_MAX;            /* copied into a local string[255] first */
    }
    cls = pr_rec_ecl_address_class(address);
    if (cls == 1u && address == 0x6B00u) {
        if (len > PR_REC_ECL_NAME_MAX) {
            len = PR_REC_ECL_NAME_MAX;        /* store with maximum length 0x0F */
        }
        for (i = 0; i < len; ++i) {
            state->active_name[i] = text[i];
        }
        state->active_name_len = (uint8_t)len;
        state->active_name_valid = 1u;
        return PR_REC_ECL_WINDOW_OK;
    }
    if (cls <= 2u) {
        for (i = 0; i <= len; ++i) {          /* characters, then the zero word */
            uint16_t *w = window_word(state, cls, (uint16_t)(address + i));
            if (w == NULL) {
                return PR_REC_ECL_WINDOW_BOUNDS;
            }
            *w = i < len ? (uint16_t)(unsigned char)text[i] : 0u;
        }
        return PR_REC_ECL_WINDOW_OK;
    }
    if (cls == 3u) {
        if (state->script_rw == NULL) {
            return PR_REC_ECL_WINDOW_UNRESOLVED;
        }
        for (i = 0; i <= len; ++i) {
            uint16_t a = (uint16_t)(address + i);
            if (a < PR_REC_ECL_SCRIPT_BASE || (size_t)(a - PR_REC_ECL_SCRIPT_BASE) >= state->script_size) {
                return PR_REC_ECL_WINDOW_BOUNDS;
            }
            state->script_rw[a - PR_REC_ECL_SCRIPT_BASE] = i < len ? (uint8_t)text[i] : 0u;
        }
        return PR_REC_ECL_WINDOW_OK;
    }
    return PR_REC_ECL_WINDOW_OK;              /* class 4: nothing stored */
}

pr_rec_ecl_window_status pr_rec_ecl_load_word(pr_rec_ecl_loader_state *state,
                                              uint16_t address, uint16_t *value) {
    uint8_t cls;
    if (state == NULL || value == NULL) {
        return PR_REC_ECL_WINDOW_ARGUMENT;
    }
    cls = pr_rec_ecl_address_class(address);
    if (cls <= 2u) {
        const uint16_t *w = window_word(state, cls, address);
        if (w == NULL) {
            return PR_REC_ECL_WINDOW_BOUNDS;
        }
        *value = *w;
        return cls == 1u ? PR_REC_ECL_WINDOW_HOOK : PR_REC_ECL_WINDOW_OK;
    }
    if (cls == 3u) {
        int b = loader_read(state, address);
        if (b < 0) {
            return PR_REC_ECL_WINDOW_BOUNDS;
        }
        *value = (uint16_t)b;                 /* zero-extended script byte */
        return PR_REC_ECL_WINDOW_OK;
    }
    return PR_REC_ECL_WINDOW_UNRESOLVED;      /* class 4 pseudo-addresses */
}

pr_rec_ecl_window_status pr_rec_ecl_store_word(pr_rec_ecl_loader_state *state,
                                               uint16_t address, uint16_t value) {
    uint8_t cls;
    if (state == NULL) {
        return PR_REC_ECL_WINDOW_ARGUMENT;
    }
    cls = pr_rec_ecl_address_class(address);
    if (cls <= 2u) {
        uint16_t *w = window_word(state, cls, address);
        if (w == NULL) {
            return PR_REC_ECL_WINDOW_BOUNDS;
        }
        *w = value;
        return cls == 1u ? PR_REC_ECL_WINDOW_HOOK : PR_REC_ECL_WINDOW_OK;
    }
    if (cls == 3u) {
        if (state->script_rw == NULL) {
            return PR_REC_ECL_WINDOW_UNRESOLVED;
        }
        if (address < PR_REC_ECL_SCRIPT_BASE || (size_t)(address - PR_REC_ECL_SCRIPT_BASE) >= state->script_size) {
            return PR_REC_ECL_WINDOW_BOUNDS;
        }
        state->script_rw[address - PR_REC_ECL_SCRIPT_BASE] = (uint8_t)value;   /* low byte only */
        return PR_REC_ECL_WINDOW_OK;
    }
    return PR_REC_ECL_WINDOW_UNRESOLVED;
}

/* Allocates the next string slot of this call (engine: string index += 1) and
 * clears everything recorded about it, so state reused across instructions
 * never carries a previous call's address or unresolved flag. */
static unsigned loader_alloc_string(pr_rec_ecl_loader_state *state) {
    unsigned k = ++state->string_count;
    state->string_len[k] = 0u;
    state->string_address[k] = 0u;
    state->string_unresolved[k] = 0u;
    return k;
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
            unsigned k = loader_alloc_string(state);
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
            unsigned k = loader_alloc_string(state);
            int hb;
            state->pc++;
            hb = loader_read(state, state->pc);
            if (hb < 0) {
                return PR_REC_ECL_LOADER_BOUNDS;
            }
            state->high[i] = (uint8_t)hb;
            {
                pr_rec_ecl_window_status ws = pr_rec_ecl_string_by_address(
                    state, k, pr_rec_ecl_operand_word(state->low[i], state->high[i]));
                if (ws == PR_REC_ECL_WINDOW_BOUNDS) {
                    return PR_REC_ECL_LOADER_BOUNDS;
                }
                /* UNRESOLVED (active name unknown) is recorded in string_unresolved[k] */
            }
        }
        /* any other tag (including 0x00): nothing more */
    }
    state->pc++;                              /* inc word [0x49ED] */
    return PR_REC_ECL_LOADER_OK;
}

/* ---- Short ECL handlers (issue #34) --------------------------------------- */

pr_rec_ecl_handler_status pr_rec_ecl_push_call_frame(pr_rec_ecl_loader_state *state,
                                                     unsigned slot) {
    if (state == NULL || slot == 0u || slot >= PR_REC_ECL_SLOT_COUNT) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    /* Both original branches allocate a 6-byte node, make it the list head,
     * link it to the previous head (null when the list was empty), and store
     * the current PC in its first word. The array models that list. */
    if (state->call_depth >= PR_REC_ECL_CALL_STACK_MAX) {
        return PR_REC_ECL_HANDLER_STACK_FULL;
    }
    state->call_stack[state->call_depth] = state->pc;   /* mov es:[di], [0x49ED] */
    state->call_depth++;
    /* push [0x7085+slot]; push [0x7045+slot]; call +0x06AF; mov [0x49ED], ax */
    state->pc = pr_rec_ecl_operand_word(state->low[slot], state->high[slot]);
    return PR_REC_ECL_HANDLER_OK;
}

static pr_rec_ecl_handler_status map_loader_status(pr_rec_ecl_loader_status st) {
    switch (st) {
    case PR_REC_ECL_LOADER_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_LOADER_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
}

pr_rec_ecl_handler_status pr_rec_ecl_op_goto(pr_rec_ecl_loader_state *state) {
    pr_rec_ecl_loader_status st;
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    st = pr_rec_ecl_load_operands(state, 1u);             /* mov al,1; lcall 0x2B:0x2A */
    if (st != PR_REC_ECL_LOADER_OK) {
        return map_loader_status(st);
    }
    /* push [0x7086]; push [0x7046]; lcall 0x2B:0x4D; mov [0x49ED], ax */
    state->pc = pr_rec_ecl_operand_word(state->low[1], state->high[1]);
    return PR_REC_ECL_HANDLER_OK;
}

pr_rec_ecl_handler_status pr_rec_ecl_op_gosub(pr_rec_ecl_loader_state *state) {
    pr_rec_ecl_loader_status st;
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    st = pr_rec_ecl_load_operands(state, 1u);             /* mov al,1; lcall 0x2B:0x2A */
    if (st != PR_REC_ECL_LOADER_OK) {
        return map_loader_status(st);
    }
    return pr_rec_ecl_push_call_frame(state, 1u);         /* mov al,1; lcall 0x2B:0x75 */
}

pr_rec_ecl_handler_status pr_rec_ecl_op_print_return(pr_rec_ecl_loader_state *state) {
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    state->pc++;                                          /* inc word [0x49ED] */
    if (state->b84df != 0u) {                             /* cmp byte [0x84DF],0 ; je */
        state->b5e24 = 1u;
        state->b5e25++;
        state->b84df = 0u;
    } else {
        state->b5e24 = 1u;
        state->b5e25++;
    }
    return PR_REC_ECL_HANDLER_OK;
}

/* ---- Character-effect dispatch entry 8 (issue #39) ------------------------ */

pr_rec_effect_status pr_rec_u38_entry8(pr_rec_modifier_state *mods,
                                       const uint8_t *record, unsigned *applied) {
    uint8_t v;
    if (applied != NULL) {
        *applied = 0u;
    }
    if (mods == NULL || record == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    /* les di,[0x5D92] ; cmp byte es:[di+0xA0],2 ; je ; ... ,5 ; je ; ... ,8 ; jne skip */
    v = record[PR_REC_CHAR_OFFSET_A0];
    if (v == 2u || v == 5u || v == 8u) {
        mods->b6816 = (uint8_t)(mods->b6816 + 2u);     /* add byte [0x6816],2 */
        mods->b6822 = (uint8_t)(mods->b6822 - 2u);     /* sub byte [0x6822],2 */
        if (applied != NULL) {
            *applied = 1u;
        }
    }
    return PR_REC_EFFECT_OK;                           /* pop bp ; retf 0xA */
}

/* ---- Character-effect dispatch entry 9 (issue #43) ------------------------ */

pr_rec_effect_status pr_rec_u38_entry9(pr_rec_modifier_state *mods,
                                       const uint8_t *record, unsigned *applied) {
    uint8_t v;
    if (applied != NULL) {
        *applied = 0u;
    }
    if (mods == NULL || record == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    /* les di,[0x5D92] ; cmp byte es:[di+0xA0],0 ; je ; ... ,3 ; je ; ... ,6 ; jne skip */
    v = record[PR_REC_CHAR_OFFSET_A0];
    if (v == 0u || v == 3u || v == 6u) {
        mods->b6816 = (uint8_t)(mods->b6816 + 2u);     /* add byte [0x6816],2 */
        mods->b6822 = (uint8_t)(mods->b6822 - 2u);     /* sub byte [0x6822],2 */
        if (applied != NULL) {
            *applied = 1u;
        }
    }
    return PR_REC_EFFECT_OK;                           /* pop bp ; retf 0xA */
}
