#include "poolrad/replay/slot_a_ecl_live.h"

#include "poolrad/formats/dos_character.h"

#include <string.h>

#define DS_LAST_REQUIRED 0x84E8u

static uint16_t word(const uint8_t *at)
{
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[1] << 8));
}

static const uint8_t *far_span(const uint8_t *ram, size_t ram_size,
                               const uint8_t *ds_bytes, uint16_t slot,
                               size_t width)
{
    uint16_t off = word(ds_bytes + slot);
    uint16_t seg = word(ds_bytes + slot + 2u);
    size_t start = ((size_t)seg << 4) + off;
    if (width > 0x10000u || (size_t)off > 0x10000u - width ||
        start > ram_size || width > ram_size - start)
        return NULL;
    return ram + start;
}

static void copy_words(uint16_t *dst, const uint8_t *src, size_t count)
{
    size_t i;
    for (i = 0u; i < count; ++i) dst[i] = word(src + 2u * i);
}

pr_slot_a_ecl_bind_status pr_slot_a_ecl_bind_ram(
    const uint8_t *ram, size_t ram_size, uint16_t ds, uint16_t entry,
    pr_rec_ecl_loader_state *out)
{
    size_t ds_start = (size_t)ds << 4;
    const uint8_t *d, *w0, *w1, *w2, *script, *record = NULL;
    pr_rec_ecl_loader_state result;
    unsigned i;
    if (!ram || !out) return PR_SLOT_A_ECL_BIND_ARGUMENT;
    if (ds_start > ram_size || DS_LAST_REQUIRED + 1u > ram_size - ds_start)
        return PR_SLOT_A_ECL_BIND_BOUNDS;
    d = ram + ds_start;
    w0 = far_span(ram, ram_size, d, 0x49D2u, 0x800u);
    w1 = far_span(ram, ram_size, d, 0x49D6u, 0x800u);
    w2 = far_span(ram, ram_size, d, 0x49DAu, 0x400u);
    script = far_span(ram, ram_size, d, 0x49DEu, 0x1E00u);
    if (!w0 || !w1 || !w2 || !script) return PR_SLOT_A_ECL_BIND_BOUNDS;
    if (word(d + 0x6F70u) != 0u || word(d + 0x6F72u) != 0u)
        return PR_SLOT_A_ECL_BIND_CALL_LIST;
    if (word(d + 0x5D92u) != 0u || word(d + 0x5D94u) != 0u) {
        record = far_span(ram, ram_size, d, 0x5D92u, PR_DOS_CHARACTER_SIZE);
        if (!record) return PR_SLOT_A_ECL_BIND_BOUNDS;
        if (record[0] > PR_DOS_CHARACTER_NAME_CAPACITY)
            return PR_SLOT_A_ECL_BIND_BAD_NAME;
    }

    memset(&result, 0, sizeof result);
    result.script = script;
    result.script_size = 0x1E00u;
    result.pc = entry;
    copy_words(result.window0, w0, PR_REC_ECL_WINDOW0_WORDS);
    copy_words(result.window1, w1, PR_REC_ECL_WINDOW1_WORDS);
    copy_words(result.window2, w2, PR_REC_ECL_WINDOW2_WORDS);
    result.active_record = record;
    if (record) {
        result.active_record_size = PR_DOS_CHARACTER_SIZE;
        result.active_name_len = record[0];
        memcpy(result.active_name, record + 1u, record[0]);
        result.active_name_valid = 1u;
    }
    for (i = 0u; i < 5u; ++i)
        result.entry_addr[i] = word(d + 0x49E3u + 2u * i);
    result.restored = d[0x49F8u];
    memcpy(result.cmp_flags, d + 0x6F78u, sizeof result.cmp_flags);
    result.w6f74 = word(d + 0x6F74u);
    result.w6f76 = word(d + 0x6F76u);
    result.b84e0 = d[0x84E0u];
    result.b84e2 = d[0x84E2u];
    result.b84d2 = d[0x84D2u];
    result.b84d3 = d[0x84D3u];
    result.b84e5 = d[0x84E5u];
    result.b6f68 = d[0x6F68u];
    result.b6f69 = d[0x6F69u];
    result.b84c8 = d[0x84C8u];
    result.b84c9 = d[0x84C9u];
    result.b6f83 = d[0x6F83u];
    result.b70c6 = d[0x70C6u];
    result.file_number = d[0x5376u];
    result.b557c = d[0x557Cu];
    result.latch = d[0x6F7Fu];
    result.prev_opcode = d[0x6F7Eu];
    result.b442e = d[0x442Eu];
    result.b49ff = d[0x49FFu];
    result.b4431 = d[0x4431u];
    result.b4432 = d[0x4432u];
    result.b4433 = d[0x4433u];
    result.b4434 = d[0x4434u];
    for (i = 0u; i < 3u; ++i) {
        result.piece_words[i][0] = word(d + 0x6AB2u + 4u * i);
        result.piece_words[i][1] = word(d + 0x6AB4u + 4u * i);
    }
    result.w6f6e = word(d + 0x6F6Eu);
    result.w6f6a = word(d + 0x6F6Au);
    result.w6f6c = word(d + 0x6F6Cu);
    result.b6aad = d[0x6AADu];
    result.b6aae = d[0x6AAEu];
    result.b6aaf = d[0x6AAFu];
    result.b6ab0 = d[0x6AB0u];
    result.b6ab1 = d[0x6AB1u];
    result.b84e8 = d[0x84E8u];
    result.b84df = d[0x84DFu];
    result.b5e24 = d[0x5E24u];
    result.b5e25 = d[0x5E25u];
    *out = result;
    return PR_SLOT_A_ECL_BIND_OK;
}
