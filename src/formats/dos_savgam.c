#include "poolrad/formats/dos_savgam.h"

static const pr_dos_savgam_span blocks[PR_DOS_SAVGAM_BLOCK_COUNT] = {
    {0x0000u, 0x0001u}, /* initial ECL file byte */
    {0x0001u, 0x0801u}, /* window 0 */
    {0x0801u, 0x1001u}, /* window 1 */
    {0x1001u, 0x1401u}, /* window 2 */
    {0x1401u, 0x3201u}, /* ECL script buffer */
    {0x3201u, 0x3206u}, /* party position and two unclassified bytes */
    {0x3206u, 0x3207u}, /* post-load selector */
    {0x3207u, 0x3208u}, /* mode byte */
    {0x3208u, 0x3209u}, /* character count */
    {0x3209u, 0x3351u}  /* eight 41-byte name slots */
};

static int valid_record(const pr_dos_savgam_record *record) {
    return record && record->data && record->size == PR_DOS_SAVGAM_SIZE;
}

pr_dos_savgam_status pr_dos_savgam_open(const uint8_t *data, size_t size,
                                        pr_dos_savgam_record *out) {
    pr_dos_savgam_record result;
    if (!data || !out) return PR_DOS_SAVGAM_ARGUMENT;
    if (size != PR_DOS_SAVGAM_SIZE) return PR_DOS_SAVGAM_BAD_SIZE;
    result.data = data;
    result.size = size;
    *out = result;
    return PR_DOS_SAVGAM_OK;
}

pr_dos_savgam_status pr_dos_savgam_block(const pr_dos_savgam_record *record,
                                         size_t index, pr_dos_savgam_span *out) {
    if (!valid_record(record) || !out) return PR_DOS_SAVGAM_ARGUMENT;
    if (index >= PR_DOS_SAVGAM_BLOCK_COUNT) return PR_DOS_SAVGAM_RANGE;
    *out = blocks[index];
    return PR_DOS_SAVGAM_OK;
}

pr_dos_savgam_status pr_dos_savgam_read_core(const pr_dos_savgam_record *record,
                                             pr_dos_savgam_core *out) {
    pr_dos_savgam_core result;
    const uint8_t *data;
    if (!valid_record(record) || !out) return PR_DOS_SAVGAM_ARGUMENT;
    data = record->data;
    result.saved_ecl_file = data[0x0000u];
    result.restored_ecl_file = data[0x0E25u];
    result.map_id = data[0x01E5u];
    result.x = data[0x3201u];
    result.y = data[0x3202u];
    result.facing = data[0x3203u];
    result.mode = data[0x3207u];
    result.character_count = data[0x3208u];
    *out = result;
    return PR_DOS_SAVGAM_OK;
}

const char *pr_dos_savgam_status_string(pr_dos_savgam_status status) {
    switch (status) {
    case PR_DOS_SAVGAM_OK: return "ok";
    case PR_DOS_SAVGAM_ARGUMENT: return "invalid argument or record";
    case PR_DOS_SAVGAM_BAD_SIZE: return "world save must be 13137 bytes";
    case PR_DOS_SAVGAM_RANGE: return "block index outside world save";
    default: return "unknown error";
    }
}
