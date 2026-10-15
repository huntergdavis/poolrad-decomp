#include "poolrad/recovered/slot_a_post_up_page_join.h"

#include <stdio.h>
#include <string.h>

enum {
    TEXT_CAP = 255,
    PLANE_BYTES = 16192,
    CALL_BYTES = 8 + TEXT_CAP,
    INPUT_BYTES = PR_FONT201_BLOCK_SIZE + 2 * CALL_BYTES + 4 * PLANE_BYTES
};

static uint8_t input[INPUT_BYTES];
static uint8_t planes[4][PLANE_BYTES];

static uint16_t read_word(const uint8_t *at) {
    return (uint16_t)((uint16_t)at[0] | ((uint16_t)at[1] << 8));
}

static pr_post_up_page_print_call decode_call(const uint8_t *data,
                                              const pr_font201 *font) {
    pr_post_up_page_print_call call;
    call.font = font;
    call.draw_segment[0] = read_word(data);
    call.draw_segment[1] = read_word(data + 2u);
    call.length = data[4];
    call.column = data[5];
    call.row = data[6];
    call.color = data[7];
    call.text = (const char *)(data + 8u);
    return call;
}

static int write_stage(const char *path) {
    FILE *output = fopen(path, "wb");
    size_t lane;
    if (output == NULL) return 0;
    for (lane = 0u; lane < 4u; ++lane) {
        if (fwrite(planes[lane], 1u, PLANE_BYTES, output) != PLANE_BYTES) {
            fclose(output);
            return 0;
        }
    }
    return fclose(output) == 0;
}

int main(int argc, char **argv) {
    pr_post_up_page_join route;
    pr_post_up_page_print_call first, second;
    pr_post_up_page_print_report first_report, second_report;
    pr_res0709_rect_report clear_report;
    pr_res0709_rect_args clear = {0u, 0u, 15u, 38u, 15u, 17u};
    pr_font201 font;
    uint8_t *plane[4] = {planes[0], planes[1], planes[2], planes[3]};
    const size_t calls_at = PR_FONT201_BLOCK_SIZE;
    const size_t planes_at = calls_at + 2u * CALL_BYTES;
    FILE *source;
    size_t lane;

    if (argc != 5) {
        fprintf(stderr, "usage: probe PRIVATE_INPUT FIRST CLEAR SECOND\n");
        return 2;
    }
    source = fopen(argv[1], "rb");
    if (source == NULL) return 2;
    {
        int read_ok = fread(input, 1u, sizeof input, source) == sizeof input;
        int at_eof = fgetc(source) == EOF;
        int had_error = ferror(source) != 0;
        int close_ok = fclose(source) == 0;
        if (!read_ok || !at_eof || had_error || !close_ok) {
            fprintf(stderr, "private input size/read error\n");
            return 2;
        }
    }
    if (pr_font201_open(input, PR_FONT201_BLOCK_SIZE, &font) != PR_FONT201_OK) {
        fprintf(stderr, "private font boundary\n");
        return 2;
    }
    first = decode_call(input + calls_at, &font);
    second = decode_call(input + calls_at + CALL_BYTES, &font);
    for (lane = 0u; lane < 4u; ++lane) {
        memcpy(planes[lane], input + planes_at + lane * PLANE_BYTES,
               PLANE_BYTES);
    }
    if (pr_post_up_page_join_open(&route, plane, PLANE_BYTES, 0u,
                                  0xA000u) != PR_POST_UP_PAGE_JOIN_OK ||
        pr_post_up_page_join_print(&route, &first, &first_report) !=
            PR_POST_UP_PAGE_JOIN_OK || !write_stage(argv[2]) ||
        pr_post_up_page_join_clear(&route, clear, &clear_report) !=
            PR_POST_UP_PAGE_JOIN_OK || !write_stage(argv[3]) ||
        pr_post_up_page_join_print(&route, &second, &second_report) !=
            PR_POST_UP_PAGE_JOIN_OK || !write_stage(argv[4])) {
        fprintf(stderr, "bounded page route rejected private input\n");
        return 2;
    }
    printf("first pages=%zu cells=%zu stores=%zu changed=%zu\n",
           first_report.distinct_pages, first_report.page[0].cells,
           first_report.page[0].plane_stores,
           first_report.page[0].changed_plane_bytes);
    printf("clear stores=%zu changed=%zu selected=%04X restored=%04X\n",
           clear_report.plane_byte_stores, clear_report.changed_plane_bytes,
           clear_report.selected_segment, clear_report.restored_segment);
    printf("second pages=%zu cells=%zu stores=%zu changed=%zu\n",
           second_report.distinct_pages, second_report.page[0].cells,
           second_report.page[0].plane_stores,
           second_report.page[0].changed_plane_bytes);
    return 0;
}
