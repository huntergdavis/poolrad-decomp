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

/* ---- Character-effect dispatch entry 10 (issue #46) ----------------------- */

pr_rec_effect_status pr_rec_u38_entry10(pr_rec_modifier_state *mods, unsigned *applied) {
    if (applied != NULL) {
        *applied = 0u;
    }
    if (mods == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    /* mov al,[0x6819] ; and al,2 ; or al,al ; je skip */
    if ((mods->b6819 & 0x02u) != 0u) {
        /* mov al,[0x6818] ; xor ah,ah ; shr ax,1 ; mov [0x6818],al */
        mods->b6818 = (uint8_t)(mods->b6818 >> 1);
        /* add byte [0x6816],3 */
        mods->b6816 = (uint8_t)(mods->b6816 + 3u);
        if (applied != NULL) {
            *applied = 1u;
        }
    }
    return PR_REC_EFFECT_OK;                           /* pop bp ; retf 0xA */
}

/* ---- Character-effect dispatch entry 31 (issue #49) ----------------------- */

pr_rec_effect_status pr_rec_u38_entry31(pr_rec_modifier_state *mods) {
    if (mods == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    mods->b6822 = (uint8_t)(mods->b6822 - 4u);         /* sub byte [0x6822],4 */
    mods->b6816 = (uint8_t)(mods->b6816 - 4u);         /* sub byte [0x6816],4 */
    return PR_REC_EFFECT_OK;                           /* pop bp ; retf 0xA */
}

/* ---- Character-effect dispatch entry 30 (issue #51) ----------------------- */

pr_rec_effect_status pr_rec_u38_entry30(pr_rec_modifier_state *mods) {
    if (mods == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    mods->b6816 = (uint8_t)(mods->b6816 + 1u);         /* inc byte [0x6816] */
    mods->b6822 = (uint8_t)(mods->b6822 + 1u);         /* inc byte [0x6822] */
    return PR_REC_EFFECT_OK;                           /* pop bp ; retf 0xA */
}

/* ---- Issue #56: view-parameter setter, unit 0x3bd + 0x0155 ---------------- */

pr_rec_effect_status pr_rec_u3bd_entry1(pr_rec_view_state *view, uint8_t first,
                                        uint8_t second, uint8_t third, uint8_t fourth) {
    if (view == NULL) {
        return PR_REC_EFFECT_ARGUMENT;
    }
    view->b6a60 = first;        /* mov al,[bp+0xC]; mov [0x6A60],al */
    view->b6a61 = second;       /* mov al,[bp+0xA]; mov [0x6A61],al */
    view->b6a62 = third;        /* mov al,[bp+8];   mov [0x6A62],al */
    view->b6a63 = fourth;       /* mov al,[bp+6];   mov [0x6A63],al */
    return PR_REC_EFFECT_OK;    /* pop bp; retf 8 */
}

/* ---- Issue #56: script-state initializer, unit 0x25 + 0x01B6 (model) ------ */

pr_rec_ecl_handler_status pr_rec_ecl_script_init(pr_rec_ecl_loader_state *state) {
    unsigned k;
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    state->b84e0 = 0u;                                   /* +0x01BC */
    state->b84e2 = 0u;                                   /* +0x01C1 */
    state->b84d2 = 0u;                                   /* +0x01C6 */
    state->b84d3 = 0u;                                   /* +0x01CB */
    state->b6f68 = 0x41u;                                /* +0x01D0 */
    state->b6f69 = 9u;                                   /* +0x01D5 */
    state->b84e5 = 1u;                                   /* +0x01DA */
    state->b84c8 = 0u;                                   /* +0x01DF..+0x01EB: fill [0x84C8], 2, 0 */
    state->b84c9 = 0u;
    state->b6f83 = 8u;                                   /* +0x01F0 */
    state->pc = PR_REC_ECL_SCRIPT_BASE;                  /* +0x01F5: [0x49ED] = 0x9900 */
    state->b70c6 = 0u;                                   /* +0x01FB */
    state->call_depth = 0u;                              /* +0x0200..+0x0205: list head [0x6F70] = nil */
    state->w6f74 = 0u;                                   /* +0x0208..+0x020D */
    state->w6f76 = 0u;
    for (k = 0u; k < 6u; ++k) {                          /* +0x0210..+0x021C: fill [0x6F78], 6, 0 */
        state->cmp_flags[k] = 0u;
    }
    state->window1[0x2E1] = 0xFFu;                       /* +0x0221: script address 0x6DE1 */
    state->window1[0x2D2] = 0u;                          /* +0x022C: 0x6DD2 */
    state->window1[0x2D3] = 0u;                          /* +0x0237: 0x6DD3 */
    state->window0[0xE5] = 0u;                           /* +0x0242: 0x49E5 */
    for (k = 0u; k < 5u; ++k) {                          /* +0x024D..+0x02B8, unrolled five times */
        pr_rec_ecl_loader_status st = pr_rec_ecl_load_operands(state, 1u);   /* mov al,1; call +0x0000 */
        if (st != PR_REC_ECL_LOADER_OK) {
            return map_loader_status(st);
        }
        /* push [0x7086]; push [0x7046]; call +0x06AF; mov [0x49E3 + 2k], ax */
        state->entry_addr[k] = pr_rec_ecl_operand_word(state->low[1], state->high[1]);
    }
    state->window0[0xE6] = 1u;                           /* +0x02BB: 0x49E6 */
    if (state->restored == 0u) {                         /* +0x02C6 */
        /* +0x02CD..+0x02F0: for a = 0x4A00 to 0x4A1F do window0[a] = 0
         * (emitted as (i + 0x49FF) * 2 + 0x6E00 with 16-bit wrap, i = 1..32) */
        for (k = 0u; k < PR_REC_ECL_INIT_CLEAR0_COUNT; ++k) {
            state->window0[PR_REC_ECL_INIT_CLEAR0_FIRST + k] = 0u;
        }
        /* +0x02F2..+0x0315: for a = 0x6E79 to 0x6E82 do window1[a] = 0
         * (emitted as (i + 0x6E79) * 2 + 0x2A00 with 16-bit wrap, i = 0..9) */
        for (k = 0u; k < PR_REC_ECL_INIT_CLEAR1_COUNT; ++k) {
            state->window1[PR_REC_ECL_INIT_CLEAR1_FIRST + k] = 0u;
        }
    } else {
        state->restored = 0u;                            /* +0x0319 */
    }
    return PR_REC_ECL_HANDLER_OK;                        /* +0x031E: mov sp,bp; pop bp; retf */
}

/* ---- Issue #68: ECL block load, unit 0x25 + 0x0341 (model) ----------------- */

void pr_rec_ecl_block_name(uint8_t file_number, char name[PR_REC_ECL_BLOCK_NAME_MAX]) {
    char digits[4];
    unsigned n = 0, v = file_number, i = 0;
    if (name == NULL) {
        return;
    }
    name[i++] = 'E'; name[i++] = 'C'; name[i++] = 'L';          /* the 3-character constant at +0x0338 */
    do {                                                        /* Str(v : 0): no padding */
        digits[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u);
    while (n > 0u) {
        name[i++] = digits[--n];
    }
    name[i] = '\0';
}

pr_rec_ecl_block_status pr_rec_ecl_load_block(pr_rec_ecl_loader_state *state, uint8_t *buffer,
                                              uint8_t block_id, const pr_rec_ecl_block_io *io,
                                              unsigned max_attempts, unsigned *attempts) {
    char name[PR_REC_ECL_BLOCK_NAME_MAX];
    const uint8_t *block = NULL;
    uint16_t size = 0;
    unsigned made = 0u;                                         /* load_block calls so far (model bound) */
    size_t k;
    if (attempts != NULL) {
        *attempts = 0u;
    }
    if (state == NULL || buffer == NULL || io == NULL || io->load_block == NULL || io->release == NULL) {
        return PR_REC_ECL_BLOCK_ARGUMENT;
    }
    for (k = 0; k < PR_REC_ECL_SCRIPT_BUFFER_SIZE; ++k) {      /* +0x0348..+0x0355: fill [0x49DE]^, 0x1E00, 0 */
        buffer[k] = 0u;
    }
    state->b557c = 0u;                                          /* +0x035A */
    for (;;) {
        if (io->clear_row != NULL) {                            /* +0x035F: unit 0x3b0 entry 3 */
            io->clear_row(io->ctx);
        }
        if (io->show_wait != NULL) {                            /* +0x0364..+0x037C */
            io->show_wait(io->ctx);
        }
        pr_rec_ecl_block_name(state->file_number, name);        /* +0x0381..+0x03B0: Str([0x5376]), "ECL" + digits */
        ++made;
        if (attempts != NULL) {
            *attempts = made;
        }
        if (made > max_attempts) {
            return PR_REC_ECL_BLOCK_RETRY_LIMIT;                /* model bound; the engine has none */
        }
        block = NULL;
        size = 0u;
        /* +0x03B5..+0x03D3: extension constant, block id, &size, &block -> 0x802:0x0716 */
        if (io->load_block(io->ctx, name, PR_REC_ECL_BLOCK_EXT, block_id, &size, &block) != 0) {
            size = 0u;
            block = NULL;
        }
        if (size >= 2u) {                                       /* +0x03D8: cmp [bp-8],2 ; jb +0x035F */
            break;
        }
        /* a short result loops without releasing anything, as the engine does */
    }
    if (block == NULL || (size_t)(size - 2u) > PR_REC_ECL_SCRIPT_BUFFER_SIZE) {
        /* The engine would run Move with size - 2 past the buffer (or from a null
         * pointer); the model refuses the copy but keeps the call sequence. */
        io->release(io->ctx, block, size);                      /* +0x03F8..+0x0401 */
        if (io->clear_row != NULL) {
            io->clear_row(io->ctx);                             /* +0x0406 */
        }
        return PR_REC_ECL_BLOCK_OVERFLOW;
    }
    for (k = 0; k < (size_t)(size - 2u); ++k) {                 /* +0x03DE..+0x03F3: Move(block + 2, buffer, size - 2) */
        buffer[k] = block[2u + k];
    }
    io->release(io->ctx, block, size);                          /* +0x03F8..+0x0401: FreeMem(block, size) */
    if (io->clear_row != NULL) {                                /* +0x0406 */
        io->clear_row(io->ctx);
    }
    state->script = buffer;                                     /* the engine's [0x49DE] already addresses it */
    state->script_rw = buffer;
    state->script_size = PR_REC_ECL_SCRIPT_BUFFER_SIZE;
    return PR_REC_ECL_BLOCK_OK;                                 /* +0x040B..+0x040E: retf 2 */
}

/* ---- Issue #75: ECL run loop and opcode dispatcher (model) ------------------ */

const uint16_t pr_rec_ecl_handler_offset[PR_REC_ECL_OPCODE_COUNT] = {
    0x0001u, /* 0x00 */
    0x00F4u, /* 0x01 */
    0x0111u, /* 0x02 */
    0x0126u, /* 0x03 */
    0x01A3u, /* 0x04 */
    0x01A3u, /* 0x05 */
    0x01A3u, /* 0x06 */
    0x01A3u, /* 0x07 */
    0x0234u, /* 0x08 */
    0x0292u, /* 0x09 */
    0x02E0u, /* 0x0A */
    0x0450u, /* 0x0B */
    0x03B4u, /* 0x0C */
    0x0817u, /* 0x0D */
    0x0856u, /* 0x0E */
    0x0951u, /* 0x0F */
    0x0994u, /* 0x10 */
    0x0A09u, /* 0x11 */
    0x0A09u, /* 0x12 */
    0x0A9Fu, /* 0x13 */
    0x0AEDu, /* 0x14 */
    0x0F08u, /* 0x15 */
    0x0B52u, /* 0x16 */
    0x0B52u, /* 0x17 */
    0x0B52u, /* 0x18 */
    0x0B52u, /* 0x19 */
    0x0B52u, /* 0x1A */
    0x0B52u, /* 0x1B */
    0x125Du, /* 0x1C */
    0x12C6u, /* 0x1D */
    0x1435u, /* 0x1E */
    0x0000u, /* 0x1F: no case */
    0x0BD1u, /* 0x20 */
    0x0C74u, /* 0x21 */
    0x1629u, /* 0x22 */
    0x16C9u, /* 0x23 */
    0x179Bu, /* 0x24 */
    0x18F8u, /* 0x25 */
    0x18F8u, /* 0x26 */
    0x19B0u, /* 0x27 */
    0x1DB1u, /* 0x28 */
    0x1EF8u, /* 0x29 */
    0x0E5Eu, /* 0x2A */
    0x10D1u, /* 0x2B */
    0x25CBu, /* 0x2C */
    0x2CCFu, /* 0x2D */
    0x2775u, /* 0x2E */
    0x0DEFu, /* 0x2F */
    0x0DEFu, /* 0x30 */
    0x2AA2u, /* 0x31 */
    0x266Au, /* 0x32 */
    0x2AEBu, /* 0x33 */
    0x2AC1u, /* 0x34 */
    0x0EBCu, /* 0x35 */
    0x2B7Eu, /* 0x36 */
    0x0C74u, /* 0x37 */
    0x2E10u, /* 0x38 */
    0x2B33u, /* 0x39 */
    0x271Fu, /* 0x3A */
    0x2C03u, /* 0x3B */
    0x2E7Du, /* 0x3C */
    0x2B14u, /* 0x3D */
};

pr_rec_ecl_run_status pr_rec_ecl_dispatch(pr_rec_ecl_loader_state *state,
                                          const pr_rec_ecl_dispatch_table *table,
                                          pr_rec_ecl_handler_status *handler_status) {
    uint8_t op;
    pr_rec_ecl_handler_status hs;
    if (handler_status != NULL) {
        *handler_status = PR_REC_ECL_HANDLER_OK;
    }
    if (state == NULL || table == NULL) {
        return PR_REC_ECL_RUN_ARGUMENT;
    }
    op = state->latch;                                          /* +0x2F30: mov al,[0x6F7F] */
    /* +0x2F33..+0x3169: cmp al,imm / jne chain; each matching case is `push cs;
     * call handler; jmp +0x316F`. Opcodes with no case reach +0x316F directly. */
    if (op >= PR_REC_ECL_OPCODE_COUNT || pr_rec_ecl_handler_offset[op] == 0u) {
        return PR_REC_ECL_RUN_NO_CASE;                          /* +0x316F: pop bp; retf */
    }
    if (table->handler[op] == NULL) {
        return PR_REC_ECL_RUN_UNMODELLED;
    }
    hs = table->handler[op](state, table->ctx);
    if (handler_status != NULL) {
        *handler_status = hs;
    }
    return hs == PR_REC_ECL_HANDLER_OK ? PR_REC_ECL_RUN_OK : PR_REC_ECL_RUN_HANDLER;
}

pr_rec_ecl_run_status pr_rec_ecl_run(pr_rec_ecl_loader_state *state,
                                     const pr_rec_ecl_dispatch_table *table, uint16_t entry,
                                     unsigned max_steps, unsigned *steps,
                                     pr_rec_ecl_handler_status *handler_status) {
    unsigned made = 0u;
    if (steps != NULL) {
        *steps = 0u;
    }
    if (handler_status != NULL) {
        *handler_status = PR_REC_ECL_HANDLER_OK;
    }
    if (state == NULL || table == NULL) {
        return PR_REC_ECL_RUN_ARGUMENT;
    }
    state->pc = entry;                                          /* +0x3174..+0x3177: [0x49ED] = [bp+6] */
    state->b442e = 0u;                                          /* +0x317A */
    for (;;) {
        int b;
        pr_rec_ecl_run_status rs;
        if (state->b442e != 0u || state->b49ff != 0u) {         /* +0x317F..+0x318B */
            break;
        }
        state->prev_opcode = state->latch;                      /* +0x318D..+0x3190: [0x6F7E] = [0x6F7F] */
        b = loader_read(state, state->pc);                      /* +0x3193..+0x319C: es:[buffer + PC + 0x6700] */
        if (b < 0) {
            return PR_REC_ECL_RUN_FETCH_BOUNDS;
        }
        state->latch = (uint8_t)b;                              /* +0x31A1: [0x6F7F] = al */
        if (made >= max_steps) {
            return PR_REC_ECL_RUN_STEP_LIMIT;                   /* model bound */
        }
        ++made;
        if (steps != NULL) {
            *steps = made;
        }
        rs = pr_rec_ecl_dispatch(state, table, handler_status); /* +0x31A4..+0x31A5: call +0x2F2D */
        if (rs != PR_REC_ECL_RUN_OK) {
            return rs;
        }
    }                                                           /* +0x31A8: jmp +0x317F */
    state->b442e = 0u;                                          /* +0x31AA */
    return PR_REC_ECL_RUN_OK;                                   /* +0x31AF..+0x31B0: pop bp; retf 2 */
}

/* ---- Issue #84: slot value (unit 0x25 + 0x0136) and SAVE (unit 0x1c + 0x0292) */

pr_rec_ecl_window_status pr_rec_ecl_slot_value(pr_rec_ecl_loader_state *state, uint8_t slot,
                                               uint16_t *value) {
    uint8_t tag, low, high;
    if (value != NULL) {
        *value = 0u;
    }
    if (state == NULL || value == NULL || slot == 0u || slot >= PR_REC_ECL_SLOT_COUNT) {
        return PR_REC_ECL_WINDOW_ARGUMENT;
    }
    low = state->low[slot];                                     /* +0x013C..+0x0146: [0x7085+slot] */
    high = state->high[slot];                                   /* +0x0149..+0x0153: [0x7045+slot] */
    tag = state->tag[slot];                                     /* +0x0156..+0x015C: [0x7005+slot] */
    if (tag == 0x00u) {                                         /* +0x0160..+0x016C */
        *value = low;
        return PR_REC_ECL_WINDOW_OK;
    }
    if (tag == 0x01u || tag == 0x03u || tag == 0x80u) {         /* +0x016E..+0x0194 */
        return pr_rec_ecl_load_word(state, (uint16_t)(((unsigned)high << 8) + low), value);
    }
    if (tag == 0x02u || tag == 0x81u) {                         /* +0x0196..+0x01AA */
        *value = pr_rec_ecl_operand_word(low, high);
        return PR_REC_ECL_WINDOW_OK;
    }
    return PR_REC_ECL_WINDOW_UNRESOLVED;                        /* +0x01AD: [bp-2] never written */
}

static pr_rec_ecl_handler_status map_window_status(pr_rec_ecl_window_status ws) {
    switch (ws) {
    case PR_REC_ECL_WINDOW_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_WINDOW_HOOK: return PR_REC_ECL_HANDLER_HOOK;
    case PR_REC_ECL_WINDOW_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    case PR_REC_ECL_WINDOW_UNRESOLVED: return PR_REC_ECL_HANDLER_UNRESOLVED;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
}

pr_rec_ecl_handler_status pr_rec_ecl_op_save(pr_rec_ecl_loader_state *state,
                                             pr_rec_ecl_window_status *store_status) {
    pr_rec_ecl_loader_status st;
    pr_rec_ecl_window_status ws, vs = PR_REC_ECL_WINDOW_OK;
    uint16_t dest, value = 0u;
    if (store_status != NULL) {
        *store_status = PR_REC_ECL_WINDOW_OK;
    }
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    st = pr_rec_ecl_load_operands(state, 2u);                   /* +0x0298..+0x029B: mov al,2; lcall 0x2B:0x2A */
    if (st != PR_REC_ECL_LOADER_OK) {
        return map_loader_status(st);
    }
    dest = pr_rec_ecl_operand_word(state->low[2], state->high[2]); /* +0x02A0..+0x02AD: [0x7087], [0x7047] -> 0x2B:0x4D */
    if (state->tag[1] < 0x80u) {                                /* +0x02B0: cmp [0x7006],0x80 ; jae */
        vs = pr_rec_ecl_slot_value(state, 1u, &value);          /* +0x02B7..+0x02BF: mov al,1; lcall 0x2B:0x25 */
        if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED ||
            vs == PR_REC_ECL_WINDOW_ARGUMENT) {
            return map_window_status(vs);                       /* model: the store is not made */
        }
        ws = pr_rec_ecl_store_word(state, dest, value);         /* +0x02C2..+0x02C8: lcall 0x2B:0x52 */
    } else {
        /* +0x02CF..+0x02D7: push dest; push DS:0x70C8 (string buffer 1); lcall 0x2B:0x5C */
        ws = pr_rec_ecl_store_string(state, dest, state->string_text[1], state->string_len[1]);
    }
    if (store_status != NULL) {
        *store_status = ws;
    }
    if (ws == PR_REC_ECL_WINDOW_OK && vs == PR_REC_ECL_WINDOW_HOOK) {
        return PR_REC_ECL_HANDLER_HOOK;                         /* the value read ran the load hook */
    }
    return map_window_status(ws);                               /* +0x02DC..+0x02DF: retf */
}

/* ---- Issue #92: LOAD FILES / LOAD PIECES, unit 0x1c + 0x0C74 (model) ------- */

pr_rec_ecl_handler_status pr_rec_ecl_op_load_files(pr_rec_ecl_loader_state *state,
                                                   const pr_rec_ecl_load_files_io *io) {
    pr_rec_ecl_loader_status st;
    pr_rec_ecl_handler_status result = PR_REC_ECL_HANDLER_OK;
    uint8_t v[4];
    unsigned i;
    if (state == NULL || io == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    st = pr_rec_ecl_load_operands(state, 3u);                   /* +0x0C7A..+0x0C7D: mov al,3; lcall 0x2B:0x2A */
    if (st != PR_REC_ECL_LOADER_OK) {
        return map_loader_status(st);
    }
    state->b4431 = 1u;                                          /* +0x0C82 */
    for (i = 1u; i <= 3u; ++i) {                                /* +0x0C87..+0x0CA9: [bp-4+i] = low byte of slot value i */
        uint16_t w = 0u;
        pr_rec_ecl_window_status ws = pr_rec_ecl_slot_value(state, (uint8_t)i, &w);
        if (ws == PR_REC_ECL_WINDOW_HOOK) {
            result = PR_REC_ECL_HANDLER_HOOK;
        } else if (ws != PR_REC_ECL_WINDOW_OK) {
            return map_window_status(ws);                       /* model: before the branch-specific effects */
        }
        v[i] = (uint8_t)w;
    }
    if (state->latch == 0x21u) {                                /* +0x0CAB: cmp [0x6F7F],0x21 */
        state->b4434 = 1u;                                      /* +0x0CB2 */
        if (v[1] != 0xFFu && v[1] != 0x7Fu && state->window0[0xE6] != 0u) {   /* +0x0CB7..+0x0CCD */
            state->window0[0xC5] = v[1];                        /* +0x0CCF..+0x0CD8: word 0x18A/2 */
            if (io->load_wallset == NULL) {
                return PR_REC_ECL_HANDLER_UNRESOLVED;           /* boundary reached without a callback */
            }
            io->load_wallset(io->ctx, v[1]);                    /* +0x0CDD..+0x0CE1: lcall 0x3D5:0x43 */
            state->window1[0x2C9] = 0u;                         /* +0x0CE6..+0x0CEC: word 0x592/2 */
        }
        if (v[3] != 0xFFu && state->window0[0xE6] == 0u) {      /* +0x0CF1..+0x0D01 */
            if (io->show_text == NULL) {
                return PR_REC_ECL_HANDLER_UNRESOLVED;
            }
            io->show_text(io->ctx);                             /* +0x0D03..+0x0D18: constant cs:0x0C6D, 0, 0x28 -> 0x3E9:0x25 */
        }
    } else {                                                    /* +0x0D20 (opcode 0x37) */
        state->b4433 = 1u;
        if (v[1] == 0x7Fu) {                                    /* +0x0D25 */
            if (io->load_pieces == NULL) {
                return PR_REC_ECL_HANDLER_UNRESOLVED;
            }
            io->load_pieces(io->ctx, 0u, 1u);                   /* +0x0D2B..+0x0D31 */
        } else if (state->window0[0xE7] != 0u && state->window0[0xE8] != 0u) {   /* +0x0D39..+0x0D4F */
            if (io->load_pieces == NULL) {
                return PR_REC_ECL_HANDLER_UNRESOLVED;
            }
            io->load_pieces(io->ctx, v[1], 1u);                 /* +0x0D51..+0x0D58 */
            io->load_pieces(io->ctx, v[3], 3u);                 /* +0x0D5D..+0x0D64 */
        } else {
            for (i = 1u; i <= 3u; ++i) {                        /* +0x0D6B..+0x0DBD */
                if (v[i] != 0xFFu) {
                    if (io->load_pieces == NULL) {
                        return PR_REC_ECL_HANDLER_UNRESOLVED;   /* earlier slots' effects stand */
                    }
                    io->load_pieces(io->ctx, v[i], (uint8_t)i); /* +0x0D81..+0x0D90 */
                } else {
                    state->piece_words[i - 1u][0] = 0xFFFFu;    /* +0x0D97..+0x0DA2: [0x6AAE + 4i] */
                    state->piece_words[i - 1u][1] = 0xFFFFu;    /* +0x0DA8..+0x0DB3: [0x6AB0 + 4i] */
                }
            }
        }
    }
    if (state->b4432 != 0u && state->b4433 != 0u && state->b4434 != 0u) {   /* +0x0DBF..+0x0DD2 */
        if (io->resident_27d9 == NULL) {
            return PR_REC_ECL_HANDLER_UNRESOLVED;               /* [0x4432] stays set */
        }
        io->resident_27d9(io->ctx);                             /* +0x0DD4 */
        if (io->resident_0767 == NULL) {
            return PR_REC_ECL_HANDLER_UNRESOLVED;
        }
        io->resident_0767(io->ctx);                             /* +0x0DD9..+0x0DE1 with [0x5D92] */
        state->b4432 = 0u;                                      /* +0x0DE6 */
    }
    return result;                                              /* +0x0DEB..+0x0DEE */
}

/* ---- Issue #102: compare helpers (unit 0x25 + 0x18EB, + 0x1815) ------------- */

static void clear_cmp_flags(pr_rec_ecl_loader_state *state) {
    unsigned k;
    for (k = 0u; k < 6u; ++k) {                                 /* 0xAF8:0x16A8 with (DS:0x6F78, 6, 0) */
        state->cmp_flags[k] = 0u;
    }
}

pr_rec_ecl_handler_status pr_rec_ecl_compare_words(pr_rec_ecl_loader_state *state, uint16_t a, uint16_t b) {
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    clear_cmp_flags(state);                                     /* +0x18EE..+0x18FA */
    if (a == b) { state->cmp_flags[0] = 1u; }                   /* +0x18FF..+0x1907: jne skips */
    if (a != b) { state->cmp_flags[1] = 1u; }                   /* +0x190C..+0x1914: je skips */
    if (a < b)  { state->cmp_flags[2] = 1u; }                   /* +0x1919..+0x1921: jae skips */
    if (a > b)  { state->cmp_flags[3] = 1u; }                   /* +0x1926..+0x192E: jbe skips */
    if (a <= b) { state->cmp_flags[4] = 1u; }                   /* +0x1933..+0x193B: ja skips */
    if (a >= b) { state->cmp_flags[5] = 1u; }                   /* +0x1940..+0x1948: jb skips */
    return PR_REC_ECL_HANDLER_OK;                               /* +0x194D: pop bp; retf 4 */
}

/* Turbo Pascal string ordering: unsigned bytes, first difference decides, a
 * shorter string that is a prefix of the other is less. */
static int pascal_str_cmp(const char *a, size_t a_len, const char *b, size_t b_len) {
    size_t n = a_len < b_len ? a_len : b_len, i;
    for (i = 0; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i], cb = (unsigned char)b[i];
        if (ca != cb) {
            return ca < cb ? -1 : 1;
        }
    }
    if (a_len == b_len) {
        return 0;
    }
    return a_len < b_len ? -1 : 1;
}

pr_rec_ecl_handler_status pr_rec_ecl_compare_strings(pr_rec_ecl_loader_state *state,
                                                     const char *a, size_t a_len,
                                                     const char *b, size_t b_len) {
    char la[256], lb[256];                                      /* [bp-0x100], [bp-0x200] */
    size_t i;
    int r;
    if (state == NULL || (a == NULL && a_len != 0u) || (b == NULL && b_len != 0u)) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    /* +0x181C..+0x183F: both inputs copied into the locals with limit 255
     * (0xAF8:0x0B25) BEFORE the flags are cleared, so an input that aliases
     * the flag bytes is compared as it was on entry. */
    if (a_len > 255u) { a_len = 255u; }
    if (b_len > 255u) { b_len = 255u; }
    for (i = 0; i < a_len; ++i) { la[i] = a[i]; }
    for (i = 0; i < b_len; ++i) { lb[i] = b[i]; }
    clear_cmp_flags(state);                                     /* +0x1844..+0x1850 */
    r = pascal_str_cmp(la, a_len, lb, b_len);                   /* 0xAF8:0x0C10 six times with (a, b) */
    if (r == 0) { state->cmp_flags[0] = 1u; }                   /* +0x1866 jne */
    if (r != 0) { state->cmp_flags[1] = 1u; }                   /* +0x187E je */
    if (r < 0)  { state->cmp_flags[2] = 1u; }                   /* +0x1896 jae */
    if (r > 0)  { state->cmp_flags[3] = 1u; }                   /* +0x18AE jbe */
    if (r <= 0) { state->cmp_flags[4] = 1u; }                   /* +0x18C6 ja */
    if (r >= 0) { state->cmp_flags[5] = 1u; }                   /* +0x18DE jb */
    return PR_REC_ECL_HANDLER_OK;                               /* +0x18E5: retf 8 */
}

/* ---- Issue #102: COMPARE (unit 0x1c + 0x0126) and the IF family (+ 0x0B52) -- */

pr_rec_ecl_handler_status pr_rec_ecl_op_compare(pr_rec_ecl_loader_state *state) {
    pr_rec_ecl_loader_status st;
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    st = pr_rec_ecl_load_operands(state, 2u);                   /* +0x012D..+0x0130: mov al,2; lcall 0x2B:0x2A */
    if (st != PR_REC_ECL_LOADER_OK) {
        return map_loader_status(st);
    }
    if (state->tag[1] >= 0x80u || state->tag[2] >= 0x80u) {     /* +0x0135..+0x0141 */
        /* +0x0143..+0x0177: buffers DS:0x70C8 and DS:0x71C8 -> 0x2B:0x6B */
        return pr_rec_ecl_compare_strings(state, state->string_text[1], state->string_len[1],
                                          state->string_text[2], state->string_len[2]);
    } else {
        uint16_t a = 0u, b = 0u;
        pr_rec_ecl_window_status wa, wb;
        wa = pr_rec_ecl_slot_value(state, 1u, &a);              /* +0x017E..+0x0186: mov al,1; lcall 0x2B:0x25 */
        if (wa != PR_REC_ECL_WINDOW_OK) {
            return map_window_status(wa);                       /* reached-boundary rule: a class-1 load hook (HOOK),
                                                                 * an unresolved or out-of-bounds source stops here;
                                                                 * slot 2 is not read and the flags are untouched */
        }
        wb = pr_rec_ecl_slot_value(state, 2u, &b);              /* +0x0189..+0x0191 */
        if (wb != PR_REC_ECL_WINDOW_OK) {
            return map_window_status(wb);                       /* same rule before the compare */
        }
        return pr_rec_ecl_compare_words(state, a, b);           /* +0x0194..+0x019A: push a; push b; lcall 0x2B:0x70; +0x019F retf */
    }
}

pr_rec_ecl_handler_status pr_rec_ecl_op_if(pr_rec_ecl_loader_state *state) {
    uint8_t op;
    if (state == NULL) {
        return PR_REC_ECL_HANDLER_ARGUMENT;
    }
    state->pc++;                                                /* +0x0B55: inc word [0x49ED] */
    op = state->latch;                                          /* +0x0B59: mov al,[0x6F7F] */
    if (op < 0x16u || op > 0x1Bu) {
        return PR_REC_ECL_HANDLER_OK;                           /* no case matches: +0x0BC6 */
    }
    if (state->cmp_flags[op - 0x16u] != 0u) {                   /* +0x0B60 .. +0x0BBA: cmp byte [0x6F78 + (op - 0x16)],0 */
        return PR_REC_ECL_HANDLER_OK;
    }
    /* skip the next statement: unit 0x25 entry 22 (+0x1EA6): fetch the opcode
     * at the PC into the latch, then the loader with its prefetch count, or
     * PC += 1 when the count is 0 */
    {
        int b = loader_read(state, state->pc);                  /* +0x1EA9..+0x1EB2 */
        unsigned n;
        if (b < 0) {
            return PR_REC_ECL_HANDLER_BOUNDS;
        }
        state->latch = (uint8_t)b;                              /* +0x1EB7: [0x6F7F] = al */
        n = pr_rec_ecl_operand_count(state->latch);             /* the compare chain of function 2 */
        if (n == 0u) {
            state->pc++;                                        /* +0x1FAB: inc word [0x49ED] */
            return PR_REC_ECL_HANDLER_OK;
        }
        return map_loader_status(pr_rec_ecl_load_operands(state, (uint8_t)n));
    }
}
