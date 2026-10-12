#include "poolrad/replay/slot_a_native_save.h"

#include "poolrad/formats/dos_savgam.h"

#include <stdio.h>

static uint8_t file_data[PR_DOS_SAVGAM_SIZE];
static pr_slot_a_native_state state;

int main(int argc, char **argv) {
    FILE *input;
    pr_slot_a_native_snapshot before, after;
    pr_slot_a_native_status status;
    if (argc != 2) {
        fputs("usage: slot_a_native_smoke PRIVATE_SAVGAMA.DAT\n", stderr);
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (!input) {
        fputs("could not open private world save\n", stderr);
        return 2;
    }
    if (fread(file_data, 1u, sizeof file_data, input) != sizeof file_data ||
        fgetc(input) != EOF || ferror(input)) {
        fclose(input);
        fputs("private world save has wrong size or could not be read\n", stderr);
        return 2;
    }
    if (fclose(input) != 0) {
        fputs("could not close private world save\n", stderr);
        return 2;
    }
    status = pr_slot_a_native_restore(file_data, sizeof file_data, &state);
    if (status != PR_SLOT_A_NATIVE_OK) {
        fprintf(stderr, "restore: %s\n", pr_slot_a_native_status_string(status));
        return 1;
    }
    status = pr_slot_a_native_read_snapshot(&state, &before);
    if (status != PR_SLOT_A_NATIVE_OK) return 1;
    printf("initial party=(%u,%u,%u) clock=(%u,%u,%u)\n",
           (unsigned)before.x, (unsigned)before.y, (unsigned)before.facing,
           (unsigned)before.minute, (unsigned)before.tens,
           (unsigned)before.hour);
    status = pr_slot_a_native_one_up(&state);
    printf("one Up: %s\n", pr_slot_a_native_status_string(status));
    if (status != PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH) return 1;
    if (pr_slot_a_native_read_snapshot(&state, &after) != PR_SLOT_A_NATIVE_OK)
        return 1;
    if (before.x != after.x || before.y != after.y ||
        before.facing != after.facing || before.minute != after.minute ||
        before.tens != after.tens || before.hour != after.hour) {
        fputs("blocked Up changed the numeric snapshot\n", stderr);
        return 1;
    }
    return 0;
}
