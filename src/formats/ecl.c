#include "poolrad/formats/ecl.h"

/* Header and six-bit text layout adapted from Gold Box Explorer,
 * Copyright (c) 2018 Bil Simser; MIT notice:
 * docs/formats/GoldBoxExplorer-MIT.txt. */

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int valid_record(const pr_ecl_record *record) {
    return record && record->data && record->size >= PR_ECL_HEADER_SIZE &&
           record->size <= PR_ECL_MAX_RECORD;
}

pr_ecl_status pr_ecl_address_to_offset(const pr_ecl_record *record,
                                       uint16_t address, size_t *offset) {
    size_t result;
    if (!valid_record(record) || !offset) return PR_ECL_ARGUMENT;
    if (address < PR_ECL_ADDRESS_BASE) return PR_ECL_ADDRESS;
    result = (size_t)(address - PR_ECL_ADDRESS_BASE) + 2;
    if (result >= record->size) return PR_ECL_ADDRESS;
    *offset = result;
    return PR_ECL_OK;
}

pr_ecl_status pr_ecl_offset_to_address(const pr_ecl_record *record,
                                       size_t offset, uint16_t *address) {
    if (!valid_record(record) || !address) return PR_ECL_ARGUMENT;
    if (offset < 2 || offset >= record->size ||
        offset - 2 > UINT16_MAX - PR_ECL_ADDRESS_BASE) return PR_ECL_ADDRESS;
    *address = (uint16_t)(PR_ECL_ADDRESS_BASE + offset - 2);
    return PR_ECL_OK;
}

pr_ecl_status pr_ecl_open(const uint8_t *data, size_t size, pr_ecl_record *out) {
    pr_ecl_record result = {0};
    size_t i;
    if (!data || !out) return PR_ECL_ARGUMENT;
    if (size < PR_ECL_HEADER_SIZE || size > PR_ECL_MAX_RECORD) return PR_ECL_SIZE;
    if (word(data) != PR_ECL_DOS_PREFIX) return PR_ECL_HEADER;
    result.data = data;
    result.size = size;
    result.prefix = word(data);
    for (i = 0; i < PR_ECL_ENTRY_COUNT; ++i) {
        size_t at = 2 + i * 4;
        if (data[at] != 0x01 || data[at + 1] != 0x01) return PR_ECL_HEADER;
        result.entry_address[i] = word(data + at + 2);
        if (pr_ecl_address_to_offset(&result, result.entry_address[i],
                                     &result.entry_offset[i]) != PR_ECL_OK)
            return PR_ECL_ADDRESS;
    }
    *out = result;
    return PR_ECL_OK;
}

pr_ecl_status pr_ecl_text_at(const pr_ecl_record *record, size_t source_offset,
                             pr_ecl_text *out) {
    pr_ecl_text result;
    if (!valid_record(record) || !out) return PR_ECL_ARGUMENT;
    if (source_offset > record->size || record->size - source_offset < 2 ||
        record->data[source_offset] != 0x80) return PR_ECL_TEXT_FRAME;
    result.source_offset = source_offset;
    result.payload_offset = source_offset + 2;
    result.packed_size = record->data[source_offset + 1];
    if (result.packed_size > record->size - result.payload_offset)
        return PR_ECL_TEXT_FRAME;
    result.end_offset = result.payload_offset + result.packed_size;
    *out = result;
    return PR_ECL_OK;
}

/* Decode complete six-bit codes; the declared byte length bounds all reads.
 * The first zero code terminates text, with any remaining codes ignored. */
static size_t decode(const uint8_t *packed, size_t packed_size, char *out) {
    unsigned bits = 0, available = 0;
    size_t i, count = 0;
    for (i = 0; i < packed_size; ++i) {
        bits = (bits << 8) | packed[i];
        available += 8;
        while (available >= 6) {
            unsigned code;
            available -= 6;
            code = (bits >> available) & 63u;
            bits &= (1u << available) - 1u;
            if (!code) return count;
            if (out) out[count] = (char)(code <= 0x1f ? code + 0x40 : code);
            ++count;
        }
    }
    return count;
}

pr_ecl_status pr_ecl_text_decode(const pr_ecl_record *record,
                                 const pr_ecl_text *text, char *out,
                                 size_t capacity, size_t *written) {
    pr_ecl_text parsed;
    const uint8_t *packed;
    size_t count;
    if (!text || !written || !valid_record(record)) return PR_ECL_ARGUMENT;
    if (pr_ecl_text_at(record, text->source_offset, &parsed) != PR_ECL_OK ||
        parsed.payload_offset != text->payload_offset ||
        parsed.packed_size != text->packed_size ||
        parsed.end_offset != text->end_offset) return PR_ECL_TEXT_FRAME;
    packed = record->data + parsed.payload_offset;
    count = decode(packed, parsed.packed_size, NULL);
    *written = count;
    if (!out || capacity <= count) return PR_ECL_OUTPUT_SIZE;
    decode(packed, parsed.packed_size, out);
    out[count] = '\0';
    return PR_ECL_OK;
}

const char *pr_ecl_status_string(pr_ecl_status status) {
    switch (status) {
    case PR_ECL_OK: return "ok";
    case PR_ECL_ARGUMENT: return "invalid argument";
    case PR_ECL_SIZE: return "record size out of range";
    case PR_ECL_HEADER: return "invalid ECL header";
    case PR_ECL_ADDRESS: return "virtual address outside record";
    case PR_ECL_TEXT_FRAME: return "invalid text frame";
    case PR_ECL_OUTPUT_SIZE: return "output buffer too small";
    default: return "unknown error";
    }
}
