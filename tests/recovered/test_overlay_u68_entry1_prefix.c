#include "poolrad/recovered/overlay_u68_entry1_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    uint8_t bytes[1024] = {0}, copy[1024];
    pr_u68_e1_prefix_input in = {1, 0, 0, 6, 1, 0, 0, bytes, sizeof bytes};
    pr_u68_e1_prefix_input saved;
    pr_u68_e1_prefix_report r;
    unsigned v;
    CHECK(pr_u68_e1_prefix(NULL, &r) == PR_U68_PREFIX_ARGUMENT && !r.entry3_valid);
    CHECK(pr_u68_e1_prefix(&in, NULL) == PR_U68_PREFIX_ARGUMENT);
    in.selector = 2; CHECK(pr_u68_e1_prefix(&in, &r) == PR_U68_PREFIX_SELECTOR_PATH && !r.entry3_valid);
    in.selector = 1; in.window_word = 255; CHECK(pr_u68_e1_prefix(&in, &r) == PR_U68_PREFIX_WINDOW_CLEAR && !r.entry3_valid);
    in.window_word = 0; in.field_buffer_size = 1023; CHECK(pr_u68_e1_prefix(&in, &r) == PR_U68_PREFIX_MISSING_INPUT && !r.entry3_valid);
    in.field_buffer_size = sizeof bytes; in.field_buffer = NULL; CHECK(pr_u68_e1_prefix(&in, &r) == PR_U68_PREFIX_MISSING_INPUT);
    in.field_buffer = bytes; bytes[0x100] = 1;
    for (v = 0; v < 4; ++v) {
        bytes[0x300] = (uint8_t)(v << 6); memcpy(copy, bytes, sizeof bytes); saved = in;
        CHECK(pr_u68_e1_prefix(&in, &r) == (v == 0 ? PR_U68_PREFIX_BEFORE_8AE : v == 1 ? PR_U68_PREFIX_BEFORE_073B : PR_U68_PREFIX_MESSAGE_PATH));
        CHECK(r.entry3_valid && r.entry3_result == v);
        CHECK(memcmp(bytes, copy, sizeof bytes) == 0 && memcmp(&in, &saved, sizeof in) == 0);
    }
    return 0;
}
