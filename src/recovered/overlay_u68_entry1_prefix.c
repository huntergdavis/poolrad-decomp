#include "poolrad/recovered/overlay_u68_entry1_prefix.h"
#include "poolrad/recovered/overlay_u3bd_field.h"

pr_u68_e1_prefix_stop pr_u68_e1_prefix(const pr_u68_e1_prefix_input *input,
                                       pr_u68_e1_prefix_report *report)
{
    pr_u3bd_field_state field;
    uint8_t result = 0;
    if (!report) return PR_U68_PREFIX_ARGUMENT;
    *report = (pr_u68_e1_prefix_report){PR_U68_PREFIX_ARGUMENT, 0, 0};
    if (!input) return report->stop;
    if (input->selector != 1) return report->stop = PR_U68_PREFIX_SELECTOR_PATH;
    if (input->window_word >= 0xffu) return report->stop = PR_U68_PREFIX_WINDOW_CLEAR;
    if (!input->field_buffer || input->field_buffer_size < PR_U3BD_FIELD_BUFFER_SIZE)
        return report->stop = PR_U68_PREFIX_MISSING_INPUT;
    field = (pr_u3bd_field_state){input->map_id, input->field_buffer, input->frame_residue};
    if (pr_u3bd_entry3(&field, input->x, input->y, input->facing, &result, NULL) != PR_U3BD_FIELD_OK)
        return report->stop = PR_U68_PREFIX_MISSING_INPUT;
    report->entry3_result = result;
    report->entry3_valid = 1;
    if (result == 1) return report->stop = PR_U68_PREFIX_BEFORE_073B;
    if (result == 2 || result == 3) return report->stop = PR_U68_PREFIX_MESSAGE_PATH;
    return report->stop = PR_U68_PREFIX_BEFORE_8AE;
}
