#include "poolrad/replay/slot_a_entry1_to_0742.h"

#include "poolrad/recovered/overlay_u3bd_field.h"

#define RAM_LIMIT ((size_t)1u << 20)

static size_t physical(unsigned segment, unsigned offset)
{
    return ((size_t)segment << 4) + (offset & 0xFFFFu);
}

static int fits(size_t ram_size, size_t at, size_t length)
{
    return at <= ram_size && length <= ram_size - at;
}

static unsigned word_at(const uint8_t *ram, size_t at)
{
    return (unsigned)ram[at] | ((unsigned)ram[at + 1u] << 8);
}

static void put_word(uint8_t *ram, size_t at, unsigned value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

pr_slot_a_e1_status pr_slot_a_entry1_to_0742(uint8_t *ram, size_t ram_size,
                                             const pr_slot_a_e1_regs *regs,
                                             pr_slot_a_e1_report *report)
{
    pr_u68_e1_prefix_input in;
    pr_u68_e1_prefix_report pre;
    pr_u68_e1_prefix_stop stop;
    size_t ds_at, window_at = 0, buffer_at = 0;
    unsigned bp1, sp1, word_2628;
    size_t at_flag, at_saved, at_cs, at_ret, at_bp, at_arg;

    if (!report) return PR_SLOT_A_E1_ARGUMENT;
    *report = (pr_slot_a_e1_report){PR_U68_PREFIX_ARGUMENT, 0, 0, 0, 0, 0, 0, 0};
    if (!ram || !regs || ram_size == 0 || ram_size > RAM_LIMIT) return PR_SLOT_A_E1_ARGUMENT;

    /* DS inputs. Offsets are DS-relative, so they wrap inside the segment like the
     * original's 16-bit addressing; a read past the image is MISSING_INPUT. */
    ds_at = physical(regs->ds, 0);
    if (!fits(ram_size, physical(regs->ds, 0x49FAu), 1u) ||
        !fits(ram_size, physical(regs->ds, 0x6AADu), 3u) ||
        !fits(ram_size, physical(regs->ds, 0x84DCu), 1u) ||
        !fits(ram_size, physical(regs->ds, 0x2628u), 2u) ||
        !fits(ram_size, physical(regs->ds, 0x49D6u), 4u) ||
        !fits(ram_size, physical(regs->ds, 0x6A5Cu), 4u))
        return PR_SLOT_A_E1_MISSING_INPUT;
    (void)ds_at;

    in = (pr_u68_e1_prefix_input){0};
    in.selector = ram[physical(regs->ds, 0x49FAu)];
    in.x = ram[physical(regs->ds, 0x6AADu)];
    in.y = ram[physical(regs->ds, 0x6AAEu)];
    in.facing = ram[physical(regs->ds, 0x6AAFu)];
    in.map_id = ram[physical(regs->ds, 0x84DCu)];
    in.frame_residue = 0; /* proven not to change entry 3's result (U3BD_FIELD_ENTRY3.md) */
    in.window_word = 0;

    /* The window word is read only on the selector-1 path (+0x0D82). */
    if (in.selector == 1u) {
        window_at = physical(word_at(ram, physical(regs->ds, 0x49D8u)),
                             word_at(ram, physical(regs->ds, 0x49D6u)));
        if (!fits(ram_size, window_at + 0x592u, 2u)) return PR_SLOT_A_E1_MISSING_INPUT;
        in.window_word = (uint16_t)word_at(ram, window_at + 0x592u);
        buffer_at = physical(word_at(ram, physical(regs->ds, 0x6A5Eu)),
                             word_at(ram, physical(regs->ds, 0x6A5Cu)));
        if (fits(ram_size, buffer_at, PR_U3BD_FIELD_BUFFER_SIZE)) {
            in.field_buffer = ram + buffer_at;
            in.field_buffer_size = PR_U3BD_FIELD_BUFFER_SIZE;
        }
    }

    stop = pr_u68_e1_prefix(&in, &pre);
    report->prefix_stop = stop;
    report->entry3_result = pre.entry3_result;
    report->entry3_valid = pre.entry3_valid;
    if (stop == PR_U68_PREFIX_ARGUMENT) return PR_SLOT_A_E1_ARGUMENT;
    if (stop == PR_U68_PREFIX_MISSING_INPUT) return PR_SLOT_A_E1_MISSING_INPUT;
    if (stop != PR_U68_PREFIX_BEFORE_073B) return PR_SLOT_A_E1_NOT_REACHED;

    /* Frame arithmetic, all 16-bit like SP and BP. */
    bp1 = (unsigned)(regs->sp - 2u) & 0xFFFFu;                /* push bp; mov bp,sp */
    sp1 = (bp1 - PR_SLOT_A_E1_LOCALS) & 0xFFFFu;              /* sub sp,0x12C */
    /* A word store at segment offset 0xFFFF would wrap inside the segment; that
     * case is not modeled, so it is refused instead of guessed. */
    if (bp1 == 0xFFFFu || ((sp1 - 2u) & 0xFFFFu) == 0xFFFFu ||
        ((sp1 - 4u) & 0xFFFFu) == 0xFFFFu || ((sp1 - 6u) & 0xFFFFu) == 0xFFFFu ||
        ((sp1 - 8u) & 0xFFFFu) == 0xFFFFu)
        return PR_SLOT_A_E1_MISSING_INPUT;
    word_2628 = word_at(ram, physical(regs->ds, 0x2628u));
    at_flag = physical(regs->ss, (bp1 - 1u) & 0xFFFFu);       /* [bp-1] := 1 (+0x0DA7) */
    at_saved = physical(regs->ss, bp1);                       /* saved caller BP */
    at_cs = physical(regs->ss, (sp1 - 2u) & 0xFFFFu);         /* push cs (+0x102D) */
    at_ret = physical(regs->ss, (sp1 - 4u) & 0xFFFFu);        /* call near +0x073B (+0x102E) */
    at_bp = physical(regs->ss, (sp1 - 6u) & 0xFFFFu);         /* +0x073B: push bp */
    at_arg = physical(regs->ss, (sp1 - 8u) & 0xFFFFu);        /* +0x073E: push word [0x2628] */
    /* A word store never straddles the segment end here, but check the image first. */
    if (!fits(ram_size, at_flag, 1u) || !fits(ram_size, at_saved, 2u) ||
        !fits(ram_size, at_cs, 2u) || !fits(ram_size, at_ret, 2u) ||
        !fits(ram_size, at_bp, 2u) || !fits(ram_size, at_arg, 2u))
        return PR_SLOT_A_E1_MISSING_INPUT;

    ram[at_flag] = 1u;
    put_word(ram, at_saved, regs->bp);
    put_word(ram, at_cs, regs->cs);
    put_word(ram, at_ret, PR_SLOT_A_E1_RETURN_IP_073B);
    put_word(ram, at_bp, bp1);
    put_word(ram, at_arg, word_2628);

    report->sp = (uint16_t)((sp1 - 8u) & 0xFFFFu);
    report->bp = (uint16_t)((sp1 - 6u) & 0xFFFFu);
    report->ip = PR_SLOT_A_E1_STOP_IP;
    report->cs = regs->cs;
    report->frame_bytes_written = 11u;
    return PR_SLOT_A_E1_STOP_0742;
}
