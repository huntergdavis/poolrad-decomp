#ifndef POOLRAD_RECOVERED_OVERLAY_U3B9_ENTRY7_0708_H
#define POOLRAD_RECOVERED_OVERLAY_U3B9_ENTRY7_0708_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3B9 + 0x0708: public entry 7 (run-time stub 0x3D0:0x43),
 * executable [0708,07C0), 184 bytes, 74 instructions, 8 basic blocks, no loop,
 * 7 far call sites; arguments: a far pointer to a 0x42-byte value record at
 * [BP+6] and an index byte at [BP+0xA], consumed by `retf 6`. Its private CS
 * literal pool is [06E9,0708): one 30-character Pascal string, loaded only at
 * +0737. Evidence: docs/re/U3B9_ENTRY7_0708_SPRITE.md. Semantics:
 *   +0708  push bp; mov bp,sp; sub sp,0x46: locals BP-0x46..BP-1 = a far
 *          pointer temporary (BP-0x46 offset, BP-0x44 segment) followed by
 *          the 0x42-byte record copy at BP-0x42;
 *   +070E  0xAF8:0x0A3F copies 0x42 bytes from the far pointer [BP+6] to
 *          SS:BP-0x42 (value-parameter copy);
 *   +0721  when the live index byte [BP+0xA] is below 1 or above 3:
 *          resident 0x464:0x00FF (no arguments), then Write(Output at
 *          DS:0x8608, CS:0x06E9, width 0) and Writeln (0xAF8:0x0919,
 *          0x084A), then resident 0x899:0x0000 (no arguments);
 *   +074E  with di = index * 8: when the words [BP+di-0x44] (offset) and
 *          [BP+di-0x42] (segment) are both zero, nothing is drawn; otherwise
 *          the temporary := that far pointer p, and resident 0x464:0x0D71 is
 *          called with x = p^[4] + 2, y = p^[6] + 2 (16-bit), the words
 *          k = (y & 0xFF00) | 0 and flags = (y & 0xFF00) | 1 (AL set, AH
 *          carried from y), S = the far pointer re-read from [BP+di-0x42]:
 *          [BP+di-0x44], and D = the far pointer DS:[0x4A22]:[0x4A20];
 *          then resident 0x464:0x1B52 (no arguments);
 *   +07BA  mov sp,bp; pop bp; retf 6.
 * The locals are modelled as one 0x46-byte frame image that the callbacks can
 * see, and every original re-read of [BP+0xA] and of the locals is a re-read
 * of that represented image, so a callback that aliases the represented frame
 * changes the model as it would the original. The index range 0..8 stays
 * inside the modelled locals. Index i > 8 selects the words at BP+(8i-0x44)
 * and BP+(8i-0x42): index 9 reads the return CS at BP+4 and the record-offset
 * argument at BP+6; index 10 and above read stack beyond the explicit
 * arguments. No index selects the saved BP (BP+0) or the return IP (BP+2).
 * This model does not carry that upper-stack image and stops with
 * UNMODELED_FRAME whenever a re-read index exceeds 8 (after 0x899:0x0000
 * returns, or after a callback changed the live index). The parameter orders
 * of 0x464:0x0D71 / 0x1B52 follow other agents' unmerged source-only
 * contracts, not verified parity. */

#define PR_U3B9_0708_RECORD_SIZE 0x42u                   /* mov ax,0x42 at +0718 */
#define PR_U3B9_0708_LOCALS_SIZE 0x46u                   /* sub sp,0x46 */
#define PR_U3B9_0708_COPY_AT 4u                          /* BP-0x42 within the locals image */
#define PR_U3B9_0708_INDEX_MIN 1u
#define PR_U3B9_0708_INDEX_MAX 3u
#define PR_U3B9_0708_INDEX_FRAME_MAX 8u                  /* largest index whose slot lies in the locals */
#define PR_U3B9_0708_OUTPUT_DS 0x8608u                   /* the Text record passed to Write/Writeln */
#define PR_U3B9_0708_DEST_OFF_DS 0x4A20u
#define PR_U3B9_0708_DEST_SEG_DS 0x4A22u
#define PR_U3B9_0708_MESSAGE_LENGTH 30u                  /* CS:0x06E9 */
#define PR_U3B9_0708_END_OFFSET 0x07C0u

typedef struct {
    uint16_t record_off, record_seg;   /* [BP+6], [BP+8] */
    uint16_t index_word;               /* [BP+0xA]; only the low byte is read */
    uint8_t locals[PR_U3B9_0708_LOCALS_SIZE];  /* SS:BP-0x46..BP-1; the host supplies the
                                                  initial (uninitialized-stack) contents */
} pr_u3b9_0708_frame;

typedef struct {
    uint16_t dest_off, dest_seg;       /* DS:0x4A20, DS:0x4A22 (read at +07A4/+07A8) */
} pr_u3b9_0708_state;

typedef struct {
    void *context;
    /* 0xAF8:0x0A3F: copy `count` bytes from far src_seg:src_off to dst */
    int (*copy_record)(void *context, pr_u3b9_0708_frame *frame, uint16_t src_seg, uint16_t src_off,
                       uint8_t *dst, uint16_t count);
    int (*resident_464_00ff)(void *context, pr_u3b9_0708_frame *frame);
    /* 0xAF8:0x0919: Write(text record at DS:file, pascal string, width) */
    int (*write_string)(void *context, pr_u3b9_0708_frame *frame, uint16_t file_ds,
                        const uint8_t *pascal_string, size_t span, uint16_t width);
    /* 0xAF8:0x084A: Writeln(text record at DS:file) */
    int (*write_line_end)(void *context, pr_u3b9_0708_frame *frame, uint16_t file_ds);
    int (*resident_899_0000)(void *context, pr_u3b9_0708_frame *frame);
    /* a word read through a far pointer (es:[di+4], es:[di+6]) */
    int (*read_far_word)(void *context, pr_u3b9_0708_frame *frame, uint16_t seg, uint16_t off,
                         uint16_t *value);
    /* resident 0x464:0x0D71, words in push order */
    int (*resident_464_0d71)(void *context, pr_u3b9_0708_frame *frame, uint16_t x, uint16_t y,
                             uint16_t k_word, uint16_t flags_word, uint16_t s_seg, uint16_t s_off,
                             uint16_t d_seg, uint16_t d_off);
    int (*resident_464_1b52)(void *context, pr_u3b9_0708_frame *frame);
} pr_u3b9_0708_hooks;

typedef struct {
    uint8_t copied;          /* 0x0A3F returned */
    uint8_t error_path;      /* the index test failed and the message path ran */
    uint8_t drawn;           /* 0x464:0x0D71 returned */
    uint8_t flushed;         /* 0x464:0x1B52 returned */
    uint16_t x, y;           /* the 0x0D71 position words, when computed */
    uint16_t next_offset;    /* source offset the model stopped at, or 0x07C0 after retf */
} pr_u3b9_0708_result;

typedef enum {
    PR_U3B9_0708_OK = 0,
    PR_U3B9_0708_ARGUMENT,         /* null frame/state/hooks/out, or a message literal that is not 30 characters */
    PR_U3B9_0708_NEED_HOOK,        /* a reached boundary has no callback */
    PR_U3B9_0708_HOST_FAILURE,     /* a callback returned non-zero */
    PR_U3B9_0708_UNMODELED_FRAME   /* a re-read index > 8 (after 0x899:0x0000 returned or a callback
                                      changed it): the original reads upper-stack words at
                                      BP+(8i-0x44)/BP+(8i-0x42), e.g. return CS and the record-offset
                                      argument for index 9, which this model does not carry */
} pr_u3b9_0708_status;

/* message: the CS:0x06E9 Pascal string (length byte first, 31-byte span),
 * supplied by the caller and not reproduced here. On a non-OK status the
 * frame, state and result hold exactly the effects reached before the stop. */
pr_u3b9_0708_status pr_u3b9_0708_show(
    pr_u3b9_0708_frame *frame, const pr_u3b9_0708_state *state,
    const uint8_t *message, size_t message_size,
    const pr_u3b9_0708_hooks *hooks, pr_u3b9_0708_result *out);

#endif
