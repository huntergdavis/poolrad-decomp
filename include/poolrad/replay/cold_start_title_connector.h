#ifndef POOLRAD_REPLAY_COLD_START_TITLE_CONNECTOR_H
#define POOLRAD_REPLAY_COLD_START_TITLE_CONNECTOR_H

typedef enum {
    PR_COLD_START_TITLE_CONNECT_OK = 0,
    PR_COLD_START_TITLE_CONNECT_ARGUMENT,
    PR_COLD_START_TITLE_CONNECT_BLOCKED_ASSET,
    PR_COLD_START_TITLE_CONNECT_BLOCKED_FRAME,
    PR_COLD_START_TITLE_CONNECT_OUTPUT,
    PR_COLD_START_TITLE_CONNECT_MEMORY
} pr_cold_start_title_connect_status;

/* Title-only file boundary. Read asset_root/TITLE.DAX, invoke the portable
 * title renderer once, then write exactly 640x400x3 top-down RGB24 bytes to
 * output_path. No output file is opened before a complete frame is ready;
 * output_path must not already exist.
 * The caller owns both paths; this does not run native game startup, input,
 * menus, save restoration, or a display window. */
pr_cold_start_title_connect_status pr_cold_start_title_connect_file(
    const char *asset_root, const char *output_path);

const char *pr_cold_start_title_connect_status_string(
    pr_cold_start_title_connect_status status);

#endif
