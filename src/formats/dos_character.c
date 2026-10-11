#include "poolrad/formats/dos_character.h"

static int valid_record(const pr_dos_character_record *record) {
    return record && record->data && record->size == PR_DOS_CHARACTER_SIZE;
}

pr_dos_character_status pr_dos_character_open(const uint8_t *data, size_t size,
                                                pr_dos_character_record *out) {
    pr_dos_character_record result;
    if (!data || !out) return PR_DOS_CHARACTER_ARGUMENT;
    if (size != PR_DOS_CHARACTER_SIZE) return PR_DOS_CHARACTER_BAD_SIZE;
    result.data = data;
    result.size = size;
    *out = result;
    return PR_DOS_CHARACTER_OK;
}

pr_dos_character_status pr_dos_character_raw_span(
    const pr_dos_character_record *record, size_t source_offset,
    size_t length, pr_dos_character_span *out) {
    pr_dos_character_span result;
    if (!valid_record(record) || !out) return PR_DOS_CHARACTER_ARGUMENT;
    if (source_offset > record->size ||
        length > record->size - source_offset) return PR_DOS_CHARACTER_RANGE;
    result.source_offset = source_offset;
    result.end_offset = source_offset + length;
    *out = result;
    return PR_DOS_CHARACTER_OK;
}

pr_dos_character_status pr_dos_character_name_span(
    const pr_dos_character_record *record, pr_dos_character_span *out) {
    pr_dos_character_span result;
    if (!valid_record(record) || !out) return PR_DOS_CHARACTER_ARGUMENT;
    if (record->data[0] > PR_DOS_CHARACTER_NAME_CAPACITY)
        return PR_DOS_CHARACTER_BAD_NAME_LENGTH;
    result.source_offset = 1;
    result.end_offset = 1u + record->data[0];
    *out = result;
    return PR_DOS_CHARACTER_OK;
}

const char *pr_dos_character_status_string(pr_dos_character_status status) {
    switch (status) {
    case PR_DOS_CHARACTER_OK: return "ok";
    case PR_DOS_CHARACTER_ARGUMENT: return "invalid argument or record";
    case PR_DOS_CHARACTER_BAD_SIZE: return "character record must be 285 bytes";
    case PR_DOS_CHARACTER_RANGE: return "raw span outside character record";
    case PR_DOS_CHARACTER_BAD_NAME_LENGTH: return "character name length exceeds 15 bytes";
    default: return "unknown error";
    }
}
