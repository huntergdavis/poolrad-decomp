/* Private planar-page probe for the bounded RES_0709:00B7 model.
 * Input and output files are caller supplied; no game data is embedded. */
#include "poolrad/recovered/resident_0709_rect_fill_00b7.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int parse_byte(const char *s, uint8_t *out) {
    char *end;
    unsigned long value;
    errno = 0;
    value = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || value > 255u) {
        return 0;
    }
    *out = (uint8_t)value;
    return 1;
}

int main(int argc, char **argv) {
    FILE *input = NULL, *output = NULL;
    uint8_t values[7], *bytes = NULL;
    pr_res0709_rect_state state = {0};
    pr_res0709_rect_args args;
    pr_res0709_rect_report report;
    long file_length;
    size_t length, lane;
    int result = 1;

    if (argc != 10) {
        fputs("usage: probe INPUT OUTPUT COLOR PAGE BOTTOM RIGHT TOP LEFT ACTIVE_PAGE\n", stderr);
        return 2;
    }
    for (lane = 0; lane < 7; ++lane) {
        if (!parse_byte(argv[lane + 3], &values[lane])) {
            fputs("numeric argument outside byte range\n", stderr);
            return 2;
        }
    }
    input = fopen(argv[1], "rb");
    if (input == NULL || fseek(input, 0, SEEK_END) != 0 ||
        (file_length = ftell(input)) <= 0 || fseek(input, 0, SEEK_SET) != 0) {
        fputs("cannot size private input\n", stderr);
        goto done;
    }
    length = (size_t)file_length;
    if (length % 4u != 0u || length / 4u < 8000u) {
        fputs("invalid planar input size\n", stderr);
        goto done;
    }
    bytes = malloc(length);
    if (bytes == NULL || fread(bytes, 1, length, input) != length) {
        fputs("cannot read private input\n", stderr);
        goto done;
    }
    state.plane_bytes = length / 4u;
    for (lane = 0; lane < 4u; ++lane) {
        state.plane[lane] = bytes + lane * state.plane_bytes;
    }
    state.active_page = values[6];
    args = (pr_res0709_rect_args){values[0], values[1], values[2],
                                  values[3], values[4], values[5]};
    if (pr_res0709_rect_fill_00b7(&state, args, &report) != PR_RES0709_RECT_OK) {
        fputs("rectangle model rejected private input\n", stderr);
        goto done;
    }
    output = fopen(argv[2], "wb");
    if (output == NULL || fwrite(bytes, 1, length, output) != length) {
        fputs("cannot write private output\n", stderr);
        goto done;
    }
    printf("stores=%zu changed=%zu selected=%04X restored=%04X\n",
           report.plane_byte_stores, report.changed_plane_bytes,
           report.selected_segment, report.restored_segment);
    result = 0;
done:
    if (input != NULL) fclose(input);
    if (output != NULL && fclose(output) != 0) result = 1;
    free(bytes);
    return result;
}
