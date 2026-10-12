/* Synthetic parity test for the unit 0x68 +0x0000 translation. The buffer
 * is fabricated. An independent restatement with wide integers and an
 * explicit modulo is compared for every x, y and selector byte; the
 * 16-bit wrap of the far offset is covered on a 64 KiB view. */
#include "poolrad/recovered/overlay_u68_field_clear_0000.h"
#include "poolrad/recovered/overlay_u3bd_field.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define VIEW 0x800u

static int sb(uint8_t b) { return b < 0x80u ? b : (int)b - 256; }
static uint8_t seg64[0x10000], ref64[0x10000];

int main(void)
{
    static uint8_t buf[VIEW], ref[VIEW];
    pr_u68_0000_state st;
    pr_u68_0000_report rep;
    unsigned x, y, sel, i;

    /* every x, y (signed), the four live selectors plus representative others, four offsets; the address is
       checked with wide integers: ((offset + sy * 16 + sx) mod 65536 + 0x300) mod 65536. The buffer is rebuilt
       once per (offset, selector); each case restores its one byte, and a full comparison runs on a sample. */
    for (i = 0; i < 4; i++) {
        static const unsigned offsets[4] = { 0, 0x100, 0x2F0, 0x1 };
        static const uint8_t sels[11] = { 0, 2, 4, 6, 1, 3, 5, 7, 8, 0x80, 0xFF };
        unsigned si;
        for (si = 0; si < 11; si++) {
            uint8_t mask = 0xFF;
            unsigned k;
            sel = sels[si];
            if (sel == 6) mask = 0x3F; else if (sel == 4) mask = 0xCF; else if (sel == 2) mask = 0xF3; else if (sel == 0) mask = 0xFC;
            for (k = 0; k < VIEW; k++) buf[k] = (uint8_t)(k * 29u + 3u + i);
            memcpy(ref, buf, VIEW);
            for (x = 0; x < 256; x++) {
                for (y = 0; y < 256; y++) {
                    long a = ((long)offsets[i] + (long)sb((uint8_t)y) * 16 + sb((uint8_t)x)) % 65536;
                    unsigned addr;
                    uint8_t orig;
                    if (a < 0) a += 65536;
                    addr = (unsigned)((a + 0x300) % 65536);
                    st.segment = buf; st.segment_size = VIEW; st.offset = (uint16_t)offsets[i];
                    if (mask != 0xFF && addr >= VIEW) {
                        CHECK(pr_u68_0000(&st, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &rep) == PR_U68_0000_ARGUMENT);
                        CHECK(!rep.touched);
                        continue;
                    }
                    orig = addr < VIEW ? buf[addr] : 0;
                    CHECK(pr_u68_0000(&st, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &rep) == PR_U68_0000_OK);
                    CHECK(rep.touched == (mask != 0xFF));
                    if (rep.touched) {
                        CHECK(rep.address == addr && rep.before == orig && rep.after == (uint8_t)(orig & mask) && buf[addr] == rep.after);
                        buf[addr] = orig;                                    /* restore the one byte */
                    }
                    if (((x * 7u + y * 13u) % 509u) == 0) CHECK(memcmp(buf, ref, VIEW) == 0);   /* nothing else changed */
                }
            }
        }
    }

    /* 16-bit wrap on a 64 KiB view: offset 0xFF00 + 16 * 5 + 3 + 0x300 wraps to 0x0253; offset 0xFFFF with x = -1 */
    memset(seg64, 0xFF, sizeof seg64); memcpy(ref64, seg64, sizeof seg64);
    st.segment = seg64; st.segment_size = sizeof seg64; st.offset = 0xFF00u;
    CHECK(pr_u68_0000(&st, 3, 5, 6, &rep) == PR_U68_0000_OK && rep.address == (uint16_t)(0xFF00u + 80u + 3u + 0x300u) && rep.address == 0x0253);
    ref64[0x0253] = 0x3F;
    CHECK(memcmp(seg64, ref64, sizeof seg64) == 0);
    st.offset = 0xFFFFu;                                               /* x = 0xFF = -1: di = 0xFFFE, + 0x300 = 0x02FE */
    CHECK(pr_u68_0000(&st, 0xFF, 0, 0, &rep) == PR_U68_0000_OK && rep.address == 0x02FE && seg64[0x02FE] == 0xFC);
    st.offset = 0x0000u;                                               /* y = 0x80 = -128: -2048 + 0x300 wraps to 0xFB00 */
    CHECK(pr_u68_0000(&st, 0, 0x80, 2, &rep) == PR_U68_0000_OK && rep.address == 0xFB00 && seg64[0xFB00] == 0xF3);

    /* it clears exactly the bit pair entry 3 reads: a byte of 0xFF becomes the 3-bit-mask complement, and the
       merged entry 3 then sees field 0 for that facing and unchanged fields for the other three */
    {
        static uint8_t q[0x400];
        pr_u3bd_field_state fs;
        uint8_t res;
        unsigned f;
        for (f = 0; f < 8; f += 2) {
            unsigned g;
            memset(q, 0, sizeof q);
            q[0x00 + 16 * 4 + 15] = 0x11; q[0x100 + 16 * 4 + 15] = 0x11;   /* nonzero nibbles for every selector */
            q[0x300 + 16 * 4 + 15] = 0xFF;                                 /* every field 3 */
            fs.map_id = 3; fs.buffer = q; fs.entry2_frame_residue = 0;
            st.segment = q; st.segment_size = sizeof q; st.offset = 0;
            CHECK(pr_u68_0000(&st, 15, 4, (uint8_t)f, &rep) == PR_U68_0000_OK);
            for (g = 0; g < 8; g += 2) {
                CHECK(pr_u3bd_entry3(&fs, 15, 4, (uint8_t)g, &res, NULL) == PR_U3BD_FIELD_OK);
                CHECK(res == (g == f ? 0u : 3u));
            }
        }
    }

    /* guards: nothing written, selector with no case touches nothing even with a bad pointer */
    memset(buf, 0xFF, VIEW); memcpy(ref, buf, VIEW);
    st.segment = buf; st.segment_size = 0x300; st.offset = 0;
    CHECK(pr_u68_0000(&st, 0, 0, 6, &rep) == PR_U68_0000_ARGUMENT && memcmp(buf, ref, VIEW) == 0 && !rep.touched);
    CHECK(pr_u68_0000(&st, 0, 0, 5, &rep) == PR_U68_0000_OK && !rep.touched);
    st.segment_size = 0x10001u;
    CHECK(pr_u68_0000(&st, 0, 0, 6, NULL) == PR_U68_0000_ARGUMENT);
    st.segment = NULL;
    CHECK(pr_u68_0000(&st, 0, 0, 6, NULL) == PR_U68_0000_ARGUMENT && pr_u68_0000(NULL, 0, 0, 6, NULL) == PR_U68_0000_ARGUMENT);

    puts("overlay_u68_field_clear_0000: ok");
    return 0;
}
