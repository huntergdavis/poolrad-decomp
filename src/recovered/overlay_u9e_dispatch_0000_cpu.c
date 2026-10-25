/* Original unit 0x9E +0000, the effect dispatcher, raw CPU, composed with the raw-CPU credited unit-0x38 handlers (docs/re/OVL_009E_0000_CPU.md). */
#include "poolrad/recovered/overlay_u9e_dispatch_0000_cpu.h"
#include "poolrad/recovered/overlay_u38_append_1363_cpu.h"
#include "poolrad/recovered/overlay_u38_character_compare_1276_cpu.h"
#include "poolrad/recovered/overlay_u38_character_loop_11f9_cpu.h"
#include "poolrad/recovered/overlay_u38_clear_cycle_2e08_cpu.h"
#include "poolrad/recovered/overlay_u38_dispatch_nop_cpu.h"
#include "poolrad/recovered/overlay_u38_effect_clear_264b.h"
#include "poolrad/recovered/overlay_u38_effect_scale_clear_2d88.h"
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"
#include "poolrad/recovered/overlay_u38_link_roll_2b20.h"
#include "poolrad/recovered/overlay_u38_lookup_append_2814_cpu.h"
#include "poolrad/recovered/overlay_u38_roll_append_1730_cpu.h"
#include "poolrad/recovered/overlay_u38_roll_clear_2de4_cpu.h"
#include "poolrad/recovered/overlay_u38_roll_effect_27d9_cpu.h"
#include "poolrad/recovered/overlay_u38_roll_gate_2915_cpu.h"
#include "poolrad/recovered/overlay_u38_sidecar_1472_cpu.h"
#include "poolrad/recovered/overlay_u38_effect_match_12b2.h"
#include "poolrad/recovered/overlay_u38_pair_04ba.h"
#include "poolrad/recovered/overlay_u38_record_roll_2d38.h"
#include "poolrad/recovered/overlay_u38_target_mask81_25bc.h"
#include "poolrad/recovered/overlay_u38_sidecar_flag_0918.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "poolrad/recovered/overlay_u38_string_match_cpu.h"
#include "poolrad/recovered/overlay_u38_target_gate_2ae6.h"
#include "poolrad/recovered/overlay_u38_target_index_gate_2a87.h"
#include "poolrad/recovered/overlay_u38_target_reset_133f_cpu.h"
#include "poolrad/recovered/overlay_u38_target_reset_2b7d.h"
#include "poolrad/recovered/overlay_unit38_effects.h"
#include "poolrad/recovered/ecl_helpers.h"
#include "poolrad/recovered/overlay_unit38_effects_next.h"
#include <string.h>

#define MASK (PR_U9E_0000_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
#define DISPATCH_RETURN_IP 0x0022u
#define LCALL_IP 0x001Eu
typedef pr_u9e_0000_cpu cpu_t;

enum { K_REFUSED = 0, K_RAW = 1, K_GAP_SEMANTIC = 2, K_GAP_UNCREDITED = 3, K_GAP_DEFERRED = 4, K_ADAPTED = 5 };   /* K_ADAPTED: a credited SEMANTIC model composed through a hand adapter (phase 2) */
typedef struct { uint8_t kind; uint16_t stub_off, stub_seg, entry; uint32_t id; } slot_t;

/* Generated from the media by tools/overlay/u9e_dispatch_0000_audit.py (the audit re-derives this table from 31A2's operands, the unit stub table and
 * strict_full_c_addresses.csv and asserts it equal): slots 1..139 are 31A2's installs (slot 88 is U86 +37A7's); 0 and 140..255 are not handlers. */
static const slot_t SLOTS[256] = {
    {K_REFUSED, 0, 0, 0, 0},
    {K_ADAPTED, 0x002A, 0x0041, 0x010D, 0x00380000u | 0x010Du},
    {K_ADAPTED, 0x002F, 0x0041, 0x011D, 0x00380000u | 0x011Du},
    {K_ADAPTED, 0x0034, 0x0041, 0x013B, 0x00380000u | 0x013Bu},
    {K_GAP_UNCREDITED, 0x0039, 0x0041, 0x017F, 0x00380000u | 0x017Fu},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_GAP_SEMANTIC, 0x003E, 0x0041, 0x01C3, 0x00380000u | 0x01C3u},
    {K_GAP_UNCREDITED, 0x0043, 0x0041, 0x0233, 0x00380000u | 0x0233u},
    {K_ADAPTED, 0x0048, 0x0041, 0x0377, 0x00380000u | 0x0377u},
    {K_ADAPTED, 0x004D, 0x0041, 0x03AC, 0x00380000u | 0x03ACu},
    {K_ADAPTED, 0x0052, 0x0041, 0x03E1, 0x00380000u | 0x03E1u},
    {K_GAP_DEFERRED, 0x0057, 0x0041, 0x0400, 0x00380000u | 0x0400u},
    {K_ADAPTED, 0x005C, 0x0041, 0x04BA, 0x00380000u | 0x04BAu},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_ADAPTED, 0x0061, 0x0041, 0x05D6, 0x00380000u | 0x05D6u},
    {K_GAP_UNCREDITED, 0x0066, 0x0041, 0x05EB, 0x00380000u | 0x05EBu},
    {K_RAW, 0x006B, 0x0041, 0x0647, 0x00380000u | 0x0647u},
    {K_ADAPTED, 0x0070, 0x0041, 0x064E, 0x00380000u | 0x064Eu},
    {K_RAW, 0x0075, 0x0041, 0x0679, 0x00380000u | 0x0679u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_ADAPTED, 0x007A, 0x0041, 0x06E4, 0x00380000u | 0x06E4u},
    {K_GAP_UNCREDITED, 0x007F, 0x0041, 0x070F, 0x00380000u | 0x070Fu},
    {K_GAP_UNCREDITED, 0x0084, 0x0041, 0x0775, 0x00380000u | 0x0775u},
    {K_GAP_UNCREDITED, 0x0089, 0x0041, 0x07E0, 0x00380000u | 0x07E0u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_ADAPTED, 0x008E, 0x0041, 0x0918, 0x00380000u | 0x0918u},
    {K_RAW, 0x0093, 0x0041, 0x0947, 0x00380000u | 0x0947u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_GAP_UNCREDITED, 0x0098, 0x0041, 0x09C0, 0x00380000u | 0x09C0u},
    {K_ADAPTED, 0x009D, 0x0041, 0x0A3D, 0x00380000u | 0x0A3Du},
    {K_GAP_UNCREDITED, 0x00A2, 0x0041, 0x0A65, 0x00380000u | 0x0A65u},
    {K_RAW, 0x00F7, 0x0041, 0x133F, 0x00380000u | 0x133Fu},
    {K_GAP_UNCREDITED, 0x00A7, 0x0041, 0x0B1B, 0x00380000u | 0x0B1Bu},
    {K_ADAPTED, 0x00AC, 0x0041, 0x0BAC, 0x00380000u | 0x0BACu},
    {K_GAP_UNCREDITED, 0x00B1, 0x0041, 0x0BCF, 0x00380000u | 0x0BCFu},
    {K_ADAPTED, 0x00B6, 0x0041, 0x0C06, 0x00380000u | 0x0C06u},
    {K_ADAPTED, 0x00BB, 0x0041, 0x0C15, 0x00380000u | 0x0C15u},
    {K_ADAPTED, 0x00C0, 0x0041, 0x0C26, 0x00380000u | 0x0C26u},
    {K_ADAPTED, 0x005C, 0x0041, 0x04BA, 0x00380000u | 0x04BAu},
    {K_GAP_UNCREDITED, 0x00C5, 0x0041, 0x0C4B, 0x00380000u | 0x0C4Bu},
    {K_GAP_UNCREDITED, 0x00CA, 0x0041, 0x0CC3, 0x00380000u | 0x0CC3u},
    {K_GAP_UNCREDITED, 0x00CF, 0x0041, 0x1086, 0x00380000u | 0x1086u},
    {K_ADAPTED, 0x00D4, 0x0041, 0x10C1, 0x00380000u | 0x10C1u},
    {K_GAP_UNCREDITED, 0x00D9, 0x0041, 0x10DE, 0x00380000u | 0x10DEu},
    {K_GAP_UNCREDITED, 0x00DE, 0x0041, 0x1168, 0x00380000u | 0x1168u},
    {K_ADAPTED, 0x0048, 0x0041, 0x0377, 0x00380000u | 0x0377u},
    {K_ADAPTED, 0x004D, 0x0041, 0x03AC, 0x00380000u | 0x03ACu},
    {K_RAW, 0x00E3, 0x0041, 0x11F9, 0x00380000u | 0x11F9u},
    {K_RAW, 0x00E8, 0x0041, 0x1276, 0x00380000u | 0x1276u},
    {K_ADAPTED, 0x00ED, 0x0041, 0x12B2, 0x00380000u | 0x12B2u},
    {K_GAP_UNCREDITED, 0x00F2, 0x0041, 0x12FB, 0x00380000u | 0x12FBu},
    {K_RAW, 0x00F7, 0x0041, 0x133F, 0x00380000u | 0x133Fu},
    {K_RAW, 0x00F7, 0x0041, 0x133F, 0x00380000u | 0x133Fu},
    {K_RAW, 0x00F7, 0x0041, 0x133F, 0x00380000u | 0x133Fu},
    {K_RAW, 0x00FC, 0x0041, 0x1355, 0x00380000u | 0x1355u},
    {K_RAW, 0x0101, 0x0041, 0x135C, 0x00380000u | 0x135Cu},
    {K_RAW, 0x0106, 0x0041, 0x1363, 0x00380000u | 0x1363u},
    {K_GAP_UNCREDITED, 0x010B, 0x0041, 0x1399, 0x00380000u | 0x1399u},
    {K_ADAPTED, 0x0110, 0x0041, 0x1452, 0x00380000u | 0x1452u},
    {K_RAW, 0x0115, 0x0041, 0x1472, 0x00380000u | 0x1472u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_RAW, 0x011A, 0x0041, 0x1490, 0x00380000u | 0x1490u},
    {K_GAP_UNCREDITED, 0x011F, 0x0041, 0x14EB, 0x00380000u | 0x14EBu},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_GAP_UNCREDITED, 0x0124, 0x0041, 0x165D, 0x00380000u | 0x165Du},
    {K_GAP_UNCREDITED, 0x0129, 0x0041, 0x1671, 0x00380000u | 0x1671u},
    {K_GAP_UNCREDITED, 0x012E, 0x0041, 0x1685, 0x00380000u | 0x1685u},
    {K_GAP_UNCREDITED, 0x0133, 0x0041, 0x1699, 0x00380000u | 0x1699u},
    {K_GAP_UNCREDITED, 0x0138, 0x0041, 0x16BB, 0x00380000u | 0x16BBu},
    {K_GAP_UNCREDITED, 0x013D, 0x0041, 0x16E6, 0x00380000u | 0x16E6u},
    {K_GAP_UNCREDITED, 0x0142, 0x0041, 0x170B, 0x00380000u | 0x170Bu},
    {K_ADAPTED, 0x0147, 0x0041, 0x171F, 0x00380000u | 0x171Fu},
    {K_RAW, 0x014C, 0x0041, 0x1730, 0x00380000u | 0x1730u},
    {K_GAP_UNCREDITED, 0x008E, 0x0068, 0x39D2, 0x005E0000u | 0x39D2u},
    {K_GAP_UNCREDITED, 0x0093, 0x0068, 0x3A86, 0x005E0000u | 0x3A86u},
    {K_GAP_UNCREDITED, 0x0151, 0x0041, 0x176F, 0x00380000u | 0x176Fu},
    {K_GAP_UNCREDITED, 0x0156, 0x0041, 0x18B5, 0x00380000u | 0x18B5u},
    {K_GAP_UNCREDITED, 0x015B, 0x0041, 0x1935, 0x00380000u | 0x1935u},
    {K_GAP_UNCREDITED, 0x0160, 0x0041, 0x19D9, 0x00380000u | 0x19D9u},
    {K_GAP_UNCREDITED, 0x0165, 0x0041, 0x1A19, 0x00380000u | 0x1A19u},
    {K_GAP_UNCREDITED, 0x016A, 0x0041, 0x1A4C, 0x00380000u | 0x1A4Cu},
    {K_GAP_UNCREDITED, 0x016F, 0x0041, 0x1AAA, 0x00380000u | 0x1AAAu},
    {K_GAP_UNCREDITED, 0x0174, 0x0041, 0x1BA8, 0x00380000u | 0x1BA8u},
    {K_GAP_UNCREDITED, 0x0179, 0x0041, 0x1CAD, 0x00380000u | 0x1CADu},
    {K_GAP_UNCREDITED, 0x017E, 0x0041, 0x1E77, 0x00380000u | 0x1E77u},
    {K_GAP_UNCREDITED, 0x0183, 0x0041, 0x23A4, 0x00380000u | 0x23A4u},
    {K_GAP_UNCREDITED, 0x0188, 0x0041, 0x23C2, 0x00380000u | 0x23C2u},
    {K_GAP_UNCREDITED, 0x018D, 0x0041, 0x23EC, 0x00380000u | 0x23ECu},
    {K_GAP_UNCREDITED, 0x015B, 0x0096, 0x30D9, 0x00860000u | 0x30D9u},
    {K_ADAPTED, 0x0192, 0x0041, 0x2490, 0x00380000u | 0x2490u},
    {K_GAP_SEMANTIC, 0x0197, 0x0041, 0x24D9, 0x00380000u | 0x24D9u},
    {K_GAP_UNCREDITED, 0x019C, 0x0041, 0x2556, 0x00380000u | 0x2556u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_ADAPTED, 0x01A1, 0x0041, 0x25A2, 0x00380000u | 0x25A2u},
    {K_ADAPTED, 0x01A6, 0x0041, 0x25BC, 0x00380000u | 0x25BCu},
    {K_GAP_UNCREDITED, 0x01AB, 0x0041, 0x2610, 0x00380000u | 0x2610u},
    {K_RAW, 0x01B0, 0x0041, 0x264B, 0x00380000u | 0x264Bu},
    {K_GAP_SEMANTIC, 0x01B5, 0x0041, 0x268A, 0x00380000u | 0x268Au},
    {K_ADAPTED, 0x01BA, 0x0041, 0x2700, 0x00380000u | 0x2700u},
    {K_GAP_UNCREDITED, 0x01BF, 0x0041, 0x2730, 0x00380000u | 0x2730u},
    {K_RAW, 0x01C4, 0x0041, 0x27D9, 0x00380000u | 0x27D9u},
    {K_RAW, 0x01C9, 0x0041, 0x2814, 0x00380000u | 0x2814u},
    {K_GAP_UNCREDITED, 0x01CE, 0x0041, 0x2866, 0x00380000u | 0x2866u},
    {K_GAP_UNCREDITED, 0x01D3, 0x0041, 0x28BF, 0x00380000u | 0x28BFu},
    {K_GAP_UNCREDITED, 0x01D8, 0x0041, 0x2901, 0x00380000u | 0x2901u},
    {K_RAW, 0x01DD, 0x0041, 0x2976, 0x00380000u | 0x2976u},
    {K_RAW, 0x01E2, 0x0041, 0x2984, 0x00380000u | 0x2984u},
    {K_RAW, 0x01E7, 0x0041, 0x2992, 0x00380000u | 0x2992u},
    {K_RAW, 0x01EC, 0x0041, 0x29B6, 0x00380000u | 0x29B6u},
    {K_RAW, 0x01F1, 0x0041, 0x29CB, 0x00380000u | 0x29CBu},
    {K_RAW, 0x01F6, 0x0041, 0x29D9, 0x00380000u | 0x29D9u},
    {K_RAW, 0x01FB, 0x0041, 0x29F0, 0x00380000u | 0x29F0u},
    {K_RAW, 0x0200, 0x0041, 0x2A11, 0x00380000u | 0x2A11u},
    {K_ADAPTED, 0x0205, 0x0041, 0x2A28, 0x00380000u | 0x2A28u},
    {K_ADAPTED, 0x020A, 0x0041, 0x2A6D, 0x00380000u | 0x2A6Du},
    {K_RAW, 0x020F, 0x0041, 0x2A87, 0x00380000u | 0x2A87u},
    {K_RAW, 0x0214, 0x0041, 0x2AE6, 0x00380000u | 0x2AE6u},
    {K_RAW, 0x0219, 0x0041, 0x2B20, 0x00380000u | 0x2B20u},
    {K_ADAPTED, 0x021E, 0x0041, 0x2B63, 0x00380000u | 0x2B63u},
    {K_RAW, 0x0223, 0x0041, 0x2B7D, 0x00380000u | 0x2B7Du},
    {K_GAP_UNCREDITED, 0x0228, 0x0041, 0x2BC8, 0x00380000u | 0x2BC8u},
    {K_GAP_UNCREDITED, 0x022D, 0x0041, 0x2C1C, 0x00380000u | 0x2C1Cu},
    {K_ADAPTED, 0x0232, 0x0041, 0x2D38, 0x00380000u | 0x2D38u},
    {K_RAW, 0x0237, 0x0041, 0x2D88, 0x00380000u | 0x2D88u},
    {K_RAW, 0x023C, 0x0041, 0x2DE4, 0x00380000u | 0x2DE4u},
    {K_RAW, 0x0241, 0x0041, 0x2E08, 0x00380000u | 0x2E08u},
    {K_GAP_SEMANTIC, 0x0246, 0x0041, 0x2E37, 0x00380000u | 0x2E37u},
    {K_RAW, 0x0264, 0x0041, 0x319B, 0x00380000u | 0x319Bu},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x0250, 0x0041, 0x2F57, 0x00380000u | 0x2F57u},
    {K_GAP_UNCREDITED, 0x0255, 0x0041, 0x30A5, 0x00380000u | 0x30A5u},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_DEFERRED, 0x025A, 0x0041, 0x3134, 0x00380000u | 0x3134u},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x025F, 0x0041, 0x317A, 0x00380000u | 0x317Au},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_GAP_UNCREDITED, 0x024B, 0x0041, 0x2EDA, 0x00380000u | 0x2EDAu},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0},
    {K_REFUSED, 0, 0, 0, 0}
};

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t bp16(const uint8_t *ram, const cpu_t *c, unsigned off) { return rd16(ram, c->ss, (uint16_t)(c->bp + off)); }
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
#define ARITH 0x08D5u
/* SHL r16,1 with the PINNED rule: CF = old bit 15, OF = CF xor the new bit 15, SF/ZF/PF of the result, AF set (the pinned core's lazy AF is count & 1Fh). */
static uint16_t shl1(cpu_t *c, uint16_t a)
{
    uint16_t r = (uint16_t)(a << 1u), f = parity((uint8_t)r) | 0x0010u;
    if (a & 0x8000u) f |= 0x0001u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (((a >> 15u) ^ (r >> 15u)) & 1u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
    return r;
}
#define STEP() do { if (*budget == 0u) return PR_U9E_0000_LIMIT; --*budget; } while (0)
#define AT(next_ip) (c->ip = (next_ip))

typedef struct { uint8_t *ram; size_t size; cpu_t *cpu; const pr_u9e_0000_limits *lim; int status_raw; } call_t;

/* One adapter per composed handler: copy the 14 words into the model's own CPU type, call it with the host caps, copy back. Status 0 is OK in every model's enum. */
#define ADAPT(name, type, call) static int name(call_t *m) { type x; int s; COPY(&x, m->cpu); s = (int)(call); COPY(m->cpu, &x); return s; }
#define LIM m->lim
ADAPT(h_nop, pr_u38_nop_cpu, pr_u38_dispatch_nop_cpu(m->ram, m->size, &x))
ADAPT(h_0679, pr_u38_string_match_cpu, pr_u38_string_match_0679_cpu(m->ram, m->size, &x, LIM->loopbacks, &m->status_raw))
ADAPT(h_0947, pr_u38_string_match_cpu, pr_u38_string_match_0947_cpu(m->ram, m->size, &x, LIM->loopbacks, &m->status_raw))
ADAPT(h_11f9, pr_u38_11f9_cpu, pr_u38_character_loop_11f9_cpu(m->ram, m->size, &x, LIM->loopbacks, &m->status_raw))
ADAPT(h_1276, pr_u38_1276_cpu, pr_u38_character_compare_1276_cpu(m->ram, m->size, &x, &m->status_raw))
ADAPT(h_133f, pr_u38_133f_cpu, pr_u38_target_reset_133f_cpu(m->ram, m->size, &x))
ADAPT(h_1490, pr_u38c_cpu, pr_u38_entry50_1490_cpu(m->ram, m->size, &x, LIM->loopbacks))
ADAPT(h_29b6, pr_u38c_cpu, pr_u38_entry92_29b6_cpu(m->ram, m->size, &x))
ADAPT(h_29cb, pr_u38c_cpu, pr_u38_entry93_29cb_cpu(m->ram, m->size, &x))
ADAPT(h_29d9, pr_u38c_cpu, pr_u38_entry94_29d9_cpu(m->ram, m->size, &x))
ADAPT(h_29f0, pr_u38c_cpu, pr_u38_entry95_29f0_cpu(m->ram, m->size, &x))
ADAPT(h_2a11, pr_u38c_cpu, pr_u38_entry96_2a11_cpu(m->ram, m->size, &x))
ADAPT(h_2976, pr_u38r_cpu, pr_u38_entry89_2976_cpu(m->ram, m->size, &x, LIM->system_calls, &m->status_raw))
ADAPT(h_2984, pr_u38r_cpu, pr_u38_entry90_2984_cpu(m->ram, m->size, &x, LIM->system_calls, &m->status_raw))
ADAPT(h_2992, pr_u38r_cpu, pr_u38_entry91_2992_cpu(m->ram, m->size, &x, LIM->system_calls, &m->status_raw))
ADAPT(h_2de4, pr_u38rc_cpu, pr_u38_roll_clear_2de4_cpu(m->ram, m->size, &x, LIM->system_calls, &m->status_raw))
ADAPT(h_2e08, pr_u38_2e08_cpu, pr_u38_clear_cycle_2e08_cpu(m->ram, m->size, &x, &m->status_raw))
#define ADAPT_T(name, shift)                                                                                                                              \
    static int name##_t(call_t *m)                                                                                                                         \
    {                                                                                                                                                      \
        pr_target_cpu x; pr_target_diagnostic dg; int s; memset(&dg, 0, sizeof dg); COPY(&x, m->cpu);                                                       \
        s = (int)(shift); COPY(m->cpu, &x); m->status_raw = dg.selector_status; return s;                                                                  \
    }
ADAPT_T(h_264b, pr_u38_effect_clear_264b_cpu(m->ram, m->size, &x, &dg))
ADAPT_T(h_2a87, pr_u38_target_index_gate_2a87_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))
ADAPT_T(h_2ae6, pr_u38_target_gate_2ae6_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))
ADAPT_T(h_2b7d, pr_u38_target_reset_2b7d_cpu(m->ram, m->size, &x, &dg))
ADAPT_T(h_2d88, pr_u38_effect_scale_clear_2d88_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))

static int h_2b20_t(call_t *m)
{
    pr_roll_chain_cpu x; pr_roll_chain_diagnostic dr; int s;
    memset(&dr, 0, sizeof dr); COPY(&x, m->cpu);
    s = (int)pr_u38_link_roll_2b20_cpu(m->ram, m->size, &x, m->lim->system_calls, &dr); COPY(m->cpu, &x); m->status_raw = dr.latch_status; return s;
}
static int h_1363_t(call_t *m)
{
    pr_u38_1363_cpu x; pr_u38_1363_limits l; pr_u38_1363_diagnostic d; int s;
    memset(&d, 0, sizeof d); l.max_heap_steps = m->lim->heap_steps; l.max_links = m->lim->links; COPY(&x, m->cpu);
    s = (int)pr_u38_append_1363_cpu(m->ram, m->size, &x, &l, &d); COPY(m->cpu, &x); m->status_raw = d.append_status; return s;
}
static int h_1472_t(call_t *m)
{
    pr_u38_1472_cpu x; pr_u38_1472_diagnostic d; int s;
    memset(&d, 0, sizeof d); COPY(&x, m->cpu);
    s = (int)pr_u38_sidecar_1472_cpu(m->ram, m->size, &x, m->lim->heap_steps, m->lim->links, &d); COPY(m->cpu, &x); m->status_raw = d.append_status; return s;
}
static int h_1730_t(call_t *m)
{
    pr_u38_1730_cpu x; pr_u38_1730_limits l; pr_u38_1730_diagnostic d; int s;
    memset(&d, 0, sizeof d); l.max_system_calls = m->lim->system_calls; l.max_heap_steps = m->lim->heap_steps; l.max_links = m->lim->links; COPY(&x, m->cpu);
    s = (int)pr_u38_roll_append_1730_cpu(m->ram, m->size, &x, &l, &d); COPY(m->cpu, &x); m->status_raw = d.status; return s;
}
static int h_27d9_t(call_t *m)
{
    pr_u38_27d9_cpu x; pr_u38_27d9_limits l; pr_u38_27d9_diagnostic d; int s;
    memset(&d, 0, sizeof d); l.max_system_calls = m->lim->system_calls; l.max_heap_steps = m->lim->heap_steps; l.max_links = m->lim->links; COPY(&x, m->cpu);
    s = (int)pr_u38_roll_effect_27d9_cpu(m->ram, m->size, &x, &l, &d); COPY(m->cpu, &x); m->status_raw = d.status; return s;
}
static int h_2814_t(call_t *m)
{
    pr_u38_2814_cpu x; pr_u38_2814_limits l; pr_u38_2814_diagnostic d; int s;
    memset(&d, 0, sizeof d); l.max_lookup_loopbacks = m->lim->loopbacks; l.max_heap_steps = m->lim->heap_steps; l.max_links = m->lim->links; COPY(&x, m->cpu);
    s = (int)pr_u38_lookup_append_2814_cpu(m->ram, m->size, &x, &l, &d); COPY(m->cpu, &x); m->status_raw = d.status; return s;
}

/* ---- phase 2: adapters for credited SEMANTIC models (the PM's bar: full CPU14, FLAGS and RAM equal at the handler's return into 009E) ----
 * The semantic model supplies the engine-state effect (pr_u38_state); the adapter supplies what the model's contract leaves out: the frame (PUSH BP / POP BP, RETF 0Ah), the registers and the FLAGS of the
 * handler's last writer, computed by the pinned stored rule (byte ADD/INC here). Every one of those is compared against the original bytes by the composed differential. */
static uint8_t ds8(const uint8_t *ram, const cpu_t *c, uint16_t off) { return ram[physical(c->ds, off)]; }
static void ds_set8(uint8_t *ram, const cpu_t *c, uint16_t off, uint8_t v) { ram[physical(c->ds, off)] = v; }
static void load_state(const uint8_t *ram, const cpu_t *c, pr_u38_state *st)
{
    st->b6816 = ds8(ram, c, 0x6816); st->b6818 = ds8(ram, c, 0x6818); st->b6819 = ds8(ram, c, 0x6819); st->b681a = ds8(ram, c, 0x681A); st->b681b = ds8(ram, c, 0x681B);
    st->b681d = ds8(ram, c, 0x681D); st->b681e = ds8(ram, c, 0x681E); st->b6822 = ds8(ram, c, 0x6822); st->b6825 = ds8(ram, c, 0x6825); st->b6de3 = ds8(ram, c, 0x6DE3);
    st->scratch_6826.offset = rd16(ram, c->ds, 0x6826u); st->scratch_6826.segment = rd16(ram, c->ds, 0x6828u);
}
static void store_state(uint8_t *ram, const cpu_t *c, const pr_u38_state *st)
{
    ds_set8(ram, c, 0x6816, st->b6816); ds_set8(ram, c, 0x6818, st->b6818); ds_set8(ram, c, 0x6819, st->b6819); ds_set8(ram, c, 0x681A, st->b681a); ds_set8(ram, c, 0x681B, st->b681b);
    ds_set8(ram, c, 0x681D, st->b681d); ds_set8(ram, c, 0x681E, st->b681e); ds_set8(ram, c, 0x6822, st->b6822); ds_set8(ram, c, 0x6825, st->b6825); ds_set8(ram, c, 0x6DE3, st->b6de3);
    wr16(ram, c->ds, 0x6826u, st->scratch_6826.offset); wr16(ram, c->ds, 0x6828u, st->scratch_6826.segment);
}
/* byte ADD r/m8,imm8 and INC r/m8 with the pinned stored FillFlags rule: CF carry (INC keeps CF), AF=(a^b^r)&10h, ZF, SF at bit 7, OF, PF = low-byte parity */
static void add8_flags(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a + b); uint16_t f = parity(r);
    if (r < a) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((uint8_t)(~(a ^ b) & (a ^ r)) & 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void inc8_flags(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a + 1u); uint16_t f = parity(r);
    if ((a ^ 1u ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (a == 0x7Fu) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x0001u)) | f);   /* CF is preserved */
}
static void frame_enter(call_t *m) { push(m->ram, m->cpu, m->cpu->bp); m->cpu->bp = m->cpu->sp; }
static void frame_leave(call_t *m, uint16_t pops)
{
    cpu_t *c = m->cpu;
    c->bp = pop(m->ram, c); c->ip = pop(m->ram, c); c->cs = pop(m->ram, c); c->sp = (uint16_t)(c->sp + pops);
}
/* +010D: PUSH BP; MOV BP,SP; ADD byte [6825],5; INC byte [6822]; POP BP; RETF 0Ah (pr_u38_entry2) */
static int h_010d(call_t *m)
{
    pr_u38_state st; uint8_t a6825, a6822;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a6825 = st.b6825; a6822 = st.b6822;
    if (pr_u38_entry2(&st) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    add8_flags(m->cpu, a6825, 5u); inc8_flags(m->cpu, a6822);
    frame_leave(m, 0x0Au);
    return 0;
}


static void sub8_flags(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b); uint16_t f = parity(r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((uint8_t)((a ^ b) & (a ^ r)) & 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void dec8_flags(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a - 1u); uint16_t f = parity(r);
    if ((a ^ 1u ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (a == 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x0001u)) | f);   /* CF is preserved */
}
static void logic8_flags(cpu_t *c, uint8_t r)                          /* AND/OR/XOR byte: CF=OF=AF=0, ZF, SF at bit 7, PF */
{
    uint16_t f = parity(r);
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void shr16_flags(cpu_t *c, uint16_t a)                          /* SHR r16,1 with the pinned rule: CF = old bit 0, OF = old bit 15, AF set, SF/ZF/PF of the result */
{
    uint16_t r = (uint16_t)(a >> 1u), f = (uint16_t)(parity((uint8_t)r) | 0x0010u);
    if (a & 1u) f |= 0x0001u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (a & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void load_mods(const uint8_t *ram, const cpu_t *c, pr_rec_modifier_state *m)
{ m->b6816 = ds8(ram, c, 0x6816); m->b6822 = ds8(ram, c, 0x6822); m->b6818 = ds8(ram, c, 0x6818); m->b6819 = ds8(ram, c, 0x6819); }
static void store_mods(uint8_t *ram, const cpu_t *c, const pr_rec_modifier_state *m)
{ ds_set8(ram, c, 0x6816, m->b6816); ds_set8(ram, c, 0x6822, m->b6822); ds_set8(ram, c, 0x6818, m->b6818); ds_set8(ram, c, 0x6819, m->b6819); }
static void set_al_(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }

/* +011D: PUSH BP; MOV BP,SP; CMP byte [6825],5; JAE; (MOV byte [6825],0 | SUB byte [6825],5); DEC byte [6822]; POP BP; RETF 0Ah (pr_u38_entry3).
 * The CF that survives the DEC is the CMP's (the SUB on the taken path computes the same CF = 0 because the path needs [6825] >= 5). */
static int h_011d(call_t *m)
{
    pr_u38_state st; unsigned paths = 0; uint8_t a6825, a6822;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a6825 = st.b6825; a6822 = st.b6822;
    if (pr_u38_entry3(&st, &paths) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    m->cpu->flags = (uint16_t)((m->cpu->flags & ~0x0001u) | (a6825 < 5u ? 0x0001u : 0u));
    dec8_flags(m->cpu, a6822);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +0C06: INC byte [6816]; INC byte [6822] (pr_rec_u38_entry30) */
static int h_0c06(call_t *m)
{
    pr_rec_modifier_state mods; uint8_t a6816, a6822;
    frame_enter(m);
    load_mods(m->ram, m->cpu, &mods); a6816 = mods.b6816; a6822 = mods.b6822;
    if (pr_rec_u38_entry30(&mods) != PR_REC_EFFECT_OK) return 1;
    store_mods(m->ram, m->cpu, &mods);
    inc8_flags(m->cpu, a6816); inc8_flags(m->cpu, a6822);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +0C15: SUB byte [6822],4; SUB byte [6816],4 (pr_rec_u38_entry31) */
static int h_0c15(call_t *m)
{
    pr_rec_modifier_state mods; uint8_t a6816, a6822;
    frame_enter(m);
    load_mods(m->ram, m->cpu, &mods); a6816 = mods.b6816; a6822 = mods.b6822;
    if (pr_rec_u38_entry31(&mods) != PR_REC_EFFECT_OK) return 1;
    store_mods(m->ram, m->cpu, &mods);
    sub8_flags(m->cpu, a6822, 4u); sub8_flags(m->cpu, a6816, 4u);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +171F: MOV byte [681E],1; SUB byte [6822],4 (pr_u38_entry59) */
static int h_171f(call_t *m)
{
    pr_u38_state st; uint8_t a6822;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a6822 = st.b6822;
    if (pr_u38_entry59(&st) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    sub8_flags(m->cpu, a6822, 4u);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +25A2 / +2A6D / +2B63: MOV AL,[6819]; AND AL,mask; OR AL,AL; JE skip; MOV AL,[6818]; XOR AH,AH; SHR AX,1; MOV [6818],AL (pr_u38_entry77 / 98 / 102).
 * Not taken the mask test: AL = the masked bit, FLAGS = the OR's (zero result). Taken: AX = [6818] >> 1 (AH cleared), FLAGS = the SHR's. */
static int halve_adapter(call_t *m, uint8_t mask, pr_u38_status (*model)(pr_u38_state *, unsigned *))
{
    pr_u38_state st; unsigned paths = 0; uint8_t masked, a6818;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a6818 = st.b6818; masked = (uint8_t)(st.b6819 & mask);
    if (model(&st, &paths) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    logic8_flags(m->cpu, masked); set_al_(m->cpu, masked);
    if (masked) { m->cpu->ax = (uint16_t)(a6818 >> 1); shr16_flags(m->cpu, a6818); }
    frame_leave(m, 0x0Au);
    return 0;
}
static int h_25a2(call_t *m) { return halve_adapter(m, 0x01u, pr_u38_entry77); }
static int h_2a6d(call_t *m) { return halve_adapter(m, 0x04u, pr_u38_entry98); }
static int h_2b63(call_t *m) { return halve_adapter(m, 0x02u, pr_u38_entry102); }
/* +10C1: MOV AL,[681A]; XOR AH,AH; SHR AX,1; MOV [681A],AL (pr_u38_entry36) */
static int h_10c1(call_t *m)
{
    pr_u38_state st; uint8_t a681a;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a681a = st.b681a;
    if (pr_u38_entry36(&st) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    m->cpu->ax = (uint16_t)(a681a >> 1); shr16_flags(m->cpu, a681a);
    frame_leave(m, 0x0Au);
    return 0;
}


static void sub16_flags(cpu_t *c, uint16_t a, uint16_t b)               /* SUB AX,DX with the pinned stored rule: CF borrow, AF=(a^b^r)&10h, ZF, SF at bit 15, OF, PF = low byte */
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((uint16_t)((a ^ b) & (a ^ r)) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void load_n(const uint8_t *ram, const cpu_t *c, pr_u38n_state *st)
{
    st->b6816 = ds8(ram, c, 0x6816); st->b6818 = ds8(ram, c, 0x6818); st->b6819 = ds8(ram, c, 0x6819); st->b681c = ds8(ram, c, 0x681C);
    st->b6822 = ds8(ram, c, 0x6822); st->b682a = ds8(ram, c, 0x682A);
    st->scratch_6826.offset = rd16(ram, c->ds, 0x6826u); st->scratch_6826.segment = rd16(ram, c->ds, 0x6828u);
}
static void store_n(uint8_t *ram, const cpu_t *c, const pr_u38n_state *st)
{
    ds_set8(ram, c, 0x6816, st->b6816); ds_set8(ram, c, 0x6818, st->b6818); ds_set8(ram, c, 0x6819, st->b6819); ds_set8(ram, c, 0x681C, st->b681c);
    ds_set8(ram, c, 0x6822, st->b6822); ds_set8(ram, c, 0x682A, st->b682a);
    wr16(ram, c->ds, 0x6826u, st->scratch_6826.offset); wr16(ram, c->ds, 0x6828u, st->scratch_6826.segment);
}
/* the registers and flags of the gated "halve [6818], add 3 to [6816]" shape (+03E1 mask 02h, +06E4 mask 01h): MOV AL,[6819]; AND AL,mask; OR AL,AL; JE skip; MOV AL,[6818]; XOR AH,AH; SHR AX,1; MOV [6818],AL; ADD byte [6816],3.
 * Gate clear: AL = 0, FLAGS = the OR's. Gate set: AX = [6818] >> 1 and the ADD is the last flag writer (it overwrites the SHR's). */
static void gated_halve_add3_regs(cpu_t *c, uint8_t mask, uint8_t a6819, uint8_t a6818, uint8_t a6816)
{
    uint8_t masked = (uint8_t)(a6819 & mask);
    logic8_flags(c, masked); set_al_(c, masked);
    if (masked) { c->ax = (uint16_t)(a6818 >> 1); add8_flags(c, a6816, 3u); }
}
/* +03E1 (pr_rec_u38_entry10) */
static int h_03e1(call_t *m)
{
    pr_rec_modifier_state mods; unsigned applied = 0; uint8_t a6819, a6818, a6816;
    frame_enter(m);
    load_mods(m->ram, m->cpu, &mods); a6819 = mods.b6819; a6818 = mods.b6818; a6816 = mods.b6816;
    if (pr_rec_u38_entry10(&mods, &applied) != PR_REC_EFFECT_OK) return 1;
    store_mods(m->ram, m->cpu, &mods);
    gated_halve_add3_regs(m->cpu, 0x02u, a6819, a6818, a6816);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +06E4 (pr_u38_entry18) */
static int h_06e4(call_t *m)
{
    pr_u38_state st; unsigned paths = 0; uint8_t a6819, a6818, a6816;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a6819 = st.b6819; a6818 = st.b6818; a6816 = st.b6816;
    if (pr_u38_entry18(&st, &paths) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    gated_halve_add3_regs(m->cpu, 0x01u, a6819, a6818, a6816);
    frame_leave(m, 0x0Au);
    return 0;
}
/* +0A3D: MOV AL,[6818]; XOR AH,AH; SHR AX,1; SHR AX,1; MOV DX,AX; MOV AL,[6818]; XOR AH,AH; SUB AX,DX; MOV [6818],AL (pr_u38_entry25): DX = a >> 2, AX = a - DX, FLAGS = the 16-bit SUB's */
static int h_0a3d(call_t *m)
{
    pr_u38_state st; uint8_t a;
    frame_enter(m);
    load_state(m->ram, m->cpu, &st); a = st.b6818;
    if (pr_u38_entry25(&st) != PR_U38_OK) return 1;
    store_state(m->ram, m->cpu, &st);
    m->cpu->dx = (uint16_t)(a >> 2); m->cpu->ax = (uint16_t)(a - (a >> 2));
    sub16_flags(m->cpu, a, (uint16_t)(a >> 2));
    frame_leave(m, 0x0Au);
    return 0;
}
/* +2A28 (pr_u38_entry97): SUB SP,2 makes the locals [BP-2] (the limit [681C]) and [BP-1] (the loop counter). Gate clear: AL = 0, the OR's flags, no local written. Limit 0: [BP-2] = 0, AL = 1,
 * FLAGS = CMP 1,0 (JA taken). Otherwise the counter runs 1..limit: [BP-1] = limit and AL = limit at the end, FLAGS = CMP limit,limit (equal), [BP-2] = limit. */
static int h_2a28(call_t *m)
{
    pr_u38n_state st; unsigned iterations = 0; uint8_t a6819, limit;
    cpu_t *c = m->cpu;
    frame_enter(m);
    load_n(m->ram, c, &st); a6819 = st.b6819; limit = st.b681c;
    if (pr_u38_entry97(&st, &iterations) != PR_U38_OK) return 1;
    store_n(m->ram, c, &st);
    logic8_flags(c, (uint8_t)(a6819 & 1u)); set_al_(c, (uint8_t)(a6819 & 1u));
    if (a6819 & 1u) {
        m->ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = limit;
        set_al_(c, 1u); sub8_flags(c, 1u, limit);
        if (limit >= 1u) {
            m->ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = limit;
            set_al_(c, limit); sub8_flags(c, limit, limit);
        }
    }
    frame_leave(m, 0x0Au);
    return 0;
}


/* ---- batch P: handlers that read or write far-pointer records. The semantic models take FLAT host pointers, so each far pointer is copied in with the original's 16-bit offset wrap
 * (ES:[DI+k] wraps at 64 KiB inside the segment), the model runs on the copies, and only the bytes the model changed are written back (the models write at most one record, so aliasing between
 * two pointers cannot make the copies disagree with the original). Registers (ES:DI = the last LES) and FLAGS (the last writer) are derived per handler. */
#define XB_BYTES 0x120u
typedef struct { call_t *m; unsigned n; uint16_t seg[4], off[4]; size_t len[4]; uint8_t orig[4][XB_BYTES], cur[4][XB_BYTES]; } xb_t;
static uint8_t xb_byte(const call_t *m, uint16_t seg, uint16_t off, unsigned k) { return m->ram[physical(seg, (uint16_t)(off + k))]; }
static uint8_t *xb_get(xb_t *x, uint16_t seg, uint16_t off, size_t need)
{
    unsigned k, i = x->n++;
    if (i >= 4u || need > XB_BYTES) { x->n--; return NULL; }
    x->seg[i] = seg; x->off[i] = off; x->len[i] = need;
    for (k = 0; k < XB_BYTES; ++k) x->orig[i][k] = x->cur[i][k] = xb_byte(x->m, seg, off, k);
    return x->cur[i];
}
static void xb_flush(xb_t *x)
{
    unsigned i, k;
    for (i = 0; i < x->n; ++i)
        for (k = 0; k < XB_BYTES; ++k)
            if (x->cur[i][k] != x->orig[i][k]) x->m->ram[physical(x->seg[i], (uint16_t)(x->off[i] + k))] = x->cur[i][k];
}
static uint8_t *xb_resolve(void *ctx, pr_u38_far_ptr p, size_t need) { return xb_get((xb_t *)ctx, p.segment, p.offset, need); }
static void far_arg(const call_t *m, unsigned bp_off, uint16_t *seg, uint16_t *off) { *off = bp16(m->ram, m->cpu, bp_off); *seg = bp16(m->ram, m->cpu, bp_off + 2u); }
static void set_es_di(cpu_t *c, uint16_t seg, uint16_t off) { c->es = seg; c->di = off; }

/* +05D6 (pr_u38_entry13): LES DI,[BP+8]; MOV AL,ES:[DI+3]; LES DI,[BP+0C]; MOV ES:[DI+15h],AL. No flag writer: FLAGS stay. */
static int h_05d6(call_t *m)
{
    xb_t xs, *x = &xs; uint16_t rs, ro, ss_, so; uint8_t *rec, *src; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m);
    far_arg(m, 0x0Cu, &rs, &ro); far_arg(m, 0x08u, &ss_, &so);
    rec = xb_get(x, rs, ro, 0x16u); src = xb_get(x, ss_, so, 4u);
    if (rec && src && pr_u38_entry13(rec, src) == PR_U38_OK) {
        xb_flush(x); set_al_(m->cpu, src[3]); set_es_di(m->cpu, rs, ro);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}
/* +064E (pr_u38_entry16): record[111h] = max(record[111h], 39h); INC byte [6816]; CMP byte [681B],0Fh (the last flag writer); [681B]==0Fh clears [6818]. */
static int h_064e(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; unsigned paths = 0; uint16_t rs, ro; uint8_t *rec, a681b; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    load_state(m->ram, m->cpu, &st); a681b = st.b681b;
    rec = xb_get(x, rs, ro, 0x112u);
    if (rec && pr_u38_entry16(&st, rec, &paths) == PR_U38_OK) {
        xb_flush(x); store_state(m->ram, m->cpu, &st);
        sub8_flags(m->cpu, a681b, 0x0Fu); set_es_di(m->cpu, rs, ro);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}
/* +0BAC (pr_u38_entry28): SUB [6822],4; SUB record[111h],4; SUB record[112h],4; SUB byte [6816],4 (the last flag writer) */
static int h_0bac(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; uint16_t rs, ro; uint8_t *rec, a6816; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    load_state(m->ram, m->cpu, &st); a6816 = st.b6816;
    rec = xb_get(x, rs, ro, 0x113u);
    if (rec && pr_u38_entry28(&st, rec) == PR_U38_OK) {
        xb_flush(x); store_state(m->ram, m->cpu, &st);
        sub8_flags(m->cpu, a6816, 4u); set_es_di(m->cpu, rs, ro);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}
/* +0C26 (pr_u38_entry32): LES DI,[BP+0C]; LES DI,ES:[DI+108h] (P); CMP byte ES:[DI+3],0 (the flag writer); JLE skip; [681E]=1; [6822]=FFh */
static int h_0c26(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; unsigned paths = 0; uint16_t rs, ro, ps, po; uint8_t *rec; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    load_state(m->ram, m->cpu, &st);
    rec = xb_get(x, rs, ro, 0x10Cu);
    if (rec && pr_u38_entry32(&st, rec, xb_resolve, x, &paths) == PR_U38_OK) {
        po = (uint16_t)(rec[0x108] | rec[0x109] << 8); ps = (uint16_t)(rec[0x10A] | rec[0x10B] << 8);
        store_state(m->ram, m->cpu, &st); xb_flush(x);
        sub8_flags(m->cpu, xb_byte(m, ps, po, 3u), 0u); set_es_di(m->cpu, ps, po);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}
/* +1452 (pr_u38_entry48): P = far pointer at record[108h]; P[6] = 0; CMP byte [681D],0 (the flag writer); [681D] != 0 clears [681A] */
static int h_1452(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; unsigned paths = 0; uint16_t rs, ro, ps, po; uint8_t *rec, a681d; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    load_state(m->ram, m->cpu, &st); a681d = st.b681d;
    rec = xb_get(x, rs, ro, 0x10Cu);
    if (rec && pr_u38_entry48(&st, rec, xb_resolve, x, &paths) == PR_U38_OK) {
        po = (uint16_t)(rec[0x108] | rec[0x109] << 8); ps = (uint16_t)(rec[0x10A] | rec[0x10B] << 8);
        store_state(m->ram, m->cpu, &st); xb_flush(x);
        sub8_flags(m->cpu, a681d, 0u); set_es_di(m->cpu, ps, po);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}
/* +2490 (pr_u38_entry74): path A ([6DE3]==0 && [6822]==0): AL = source[3] & 0Fh stored back, FLAGS = the AND's; path B1 (bit 4 set): AL = source[3] & 10h, FLAGS = OR AL,AL;
 * path B2 (bit 4 clear): [6822]=FFh, AL = source[3] | 10h stored back, FLAGS = the OR's. ES:DI = the source pointer. */
static int h_2490(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; unsigned paths = 0; uint16_t ss_, so; uint8_t *src, s3, al; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x08u, &ss_, &so);
    load_state(m->ram, m->cpu, &st);
    src = xb_get(x, ss_, so, 4u);
    if (src) {
        s3 = src[3];
        if (pr_u38_entry74(&st, src, &paths) == PR_U38_OK) {
            xb_flush(x); store_state(m->ram, m->cpu, &st);
            if (paths == PR_U38_PATH_EFFECT) al = (uint8_t)(s3 & 0x0Fu);
            else if (paths == PR_U38_PATH_SECOND) al = (uint8_t)(s3 | 0x10u);
            else al = (uint8_t)(s3 & 0x10u);
            logic8_flags(m->cpu, al); set_al_(m->cpu, al); set_es_di(m->cpu, ss_, so);
            frame_leave(m, 0x0Au); rc = 0;
        }
    }
    return rc;
}
/* +2700 (pr_u38_entry82): record[11Bh] += 3; AL = record[11Bh]; CMP AL,record[32h] (the flag writer); JBE keeps, else record[11Bh] = AL = record[32h] */
static int h_2700(call_t *m)
{
    xb_t xs, *x = &xs; unsigned paths = 0; uint16_t rs, ro; uint8_t *rec, v, lim; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    rec = xb_get(x, rs, ro, 0x11Cu);
    if (rec) {
        v = (uint8_t)(rec[0x11B] + 3u); lim = rec[0x32];
        if (pr_u38_entry82(rec, &paths) == PR_U38_OK) {
            xb_flush(x); sub8_flags(m->cpu, v, lim); set_al_(m->cpu, v > lim ? lim : v); set_es_di(m->cpu, rs, ro);
            frame_leave(m, 0x0Au); rc = 0;
        }
    }
    return rc;
}
/* +013B (pr_u38_entry4): P = far pointer at record[108h]; AX = P[0Ah], DX = P[0Ch]; [6826:6828] = Q (those two words); CMP byte ES:[DI+9Fh],4 on Q; JNE end; ADD [6822],2; ADD byte [6818],2 (the last flag writer on that path) */
static int h_013b(call_t *m)
{
    xb_t xs, *x = &xs; pr_u38_state st; unsigned paths = 0; uint16_t rs, ro, ps, po; uint8_t *rec, a6818, q9f; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m); far_arg(m, 0x0Cu, &rs, &ro);
    load_state(m->ram, m->cpu, &st); a6818 = st.b6818;
    rec = xb_get(x, rs, ro, 0x10Cu);
    if (rec && pr_u38_entry4(&st, rec, xb_resolve, x, &paths) == PR_U38_OK) {
        po = (uint16_t)(rec[0x108] | rec[0x109] << 8); ps = (uint16_t)(rec[0x10A] | rec[0x10B] << 8);
        store_state(m->ram, m->cpu, &st); xb_flush(x);
        m->cpu->ax = (uint16_t)(xb_byte(m, ps, po, 0xAu) | xb_byte(m, ps, po, 0xBu) << 8); m->cpu->dx = (uint16_t)(xb_byte(m, ps, po, 0xCu) | xb_byte(m, ps, po, 0xDu) << 8);
        q9f = xb_byte(m, st.scratch_6826.segment, st.scratch_6826.offset, 0x9Fu);
        if (q9f == 4u) add8_flags(m->cpu, a6818, 2u); else sub8_flags(m->cpu, q9f, 4u);
        set_es_di(m->cpu, st.scratch_6826.segment, st.scratch_6826.offset);
        frame_leave(m, 0x0Au); rc = 0;
    }
    return rc;
}


/* ---- batch P2: +0377 and +03AC read the GLOBAL active-character pointer DS:[5D92] (offset), DS:[5D94] (segment) and compare the record byte +0A0h with three constants (+0377: 2, 5, 8; +03AC: 0, 3, 6):
 * LES DI,[5D92]; CMP byte ES:[DI+0A0h],k1; JE body; LES DI,[5D92]; CMP ...,k2; JE body; LES DI,[5D92]; CMP ...,k3; JNE end; body: ADD byte [6816],2; SUB byte [6822],2.
 * A match leaves the SUB as the last flag writer; no match leaves the third CMP (against k3) and no data change. ES:DI is the record pointer on every path; AX is untouched. The models take the full 285-byte record. */
static int gated_alignment(call_t *m, int second, uint8_t k3)
{
    xb_t xs, *x = &xs; pr_rec_modifier_state mods; unsigned applied = 0; uint16_t rs, ro; uint8_t *rec, a6822, b;
    cpu_t *c = m->cpu; int rc = 1;
    memset(&xs, 0, sizeof xs); x->m = m; frame_enter(m);
    ro = rd16(m->ram, c->ds, 0x5D92u); rs = rd16(m->ram, c->ds, 0x5D94u);
    load_mods(m->ram, c, &mods); a6822 = mods.b6822;
    rec = xb_get(x, rs, ro, PR_REC_CHAR_RECORD_SIZE);
    if (rec) {
        b = rec[0xA0];
        if ((second ? pr_rec_u38_entry9(&mods, rec, &applied) : pr_rec_u38_entry8(&mods, rec, &applied)) == PR_REC_EFFECT_OK) {
            store_mods(m->ram, c, &mods); xb_flush(x);
            if (applied) sub8_flags(c, a6822, 2u); else sub8_flags(c, b, k3);
            set_es_di(c, rs, ro);
            frame_leave(m, 0x0Au); rc = 0;
        }
    }
    return rc;
}
static int h_0377(call_t *m) { return gated_alignment(m, 0, 8u); }
static int h_03ac(call_t *m) { return gated_alignment(m, 1, 6u); }


/* ---- +0918 (slot 25): a credited CPU-level model with a CALLBACK for the resident sidecar lookup BA:2199 (SUB SP,4; PUSH [5D94]; PUSH [5D92]; MOV AL,18h; PUSH AX; LEA DI,[BP-4]; PUSH SS; PUSH DI; LCALL BA:2199;
 * OR AL,AL; JNE; MOV [681E],1; SUB byte [6822],4). The callback is the credited raw-CPU model of the resident routine (pr_res_00ba_2199_cpu), so the composition is handler + resident callee, both
 * credited, both compared against the ORIGINAL bytes (the original really runs the resident code in the composed world). A lookup that hits the host loop-back cap aborts the handler (CALLEE). */
static int res2199_lookup(void *ctx, uint8_t *ram, size_t size, pr_u38_0918_cpu *cpu)
{
    call_t *m = (call_t *)ctx; pr_res_2199_cpu c; pr_res_2199_status st;
    COPY(&c, cpu); st = pr_res_00ba_2199_cpu(ram, size, &c, m->lim->loopbacks); COPY(cpu, &c);
    return st == PR_RES_2199_OK;
}
static int h_0918(call_t *m)
{
    pr_u38_0918_cpu x; int s;
    COPY(&x, m->cpu); s = (int)pr_u38_sidecar_flag_0918(m->ram, m->size, &x, res2199_lookup, m); COPY(m->cpu, &x);
    return s;
}


/* ---- raw tier: the four credited CPU-level models that already compose their callees (their evidence documents are not named *_CPU.md, so the slots were listed as semantic) ----
 * +12B2 composes +0C06; +25BC composes +101D and the resident far call; +2D38 composes U9E entry 9 and the SYSTEM roll. */
ADAPT(h_12b2, pr_u38_101d_cpu, pr_u38_effect_match_12b2(m->ram, m->size, &x))
ADAPT_T(h_25bc, pr_u38_target_mask81_25bc_cpu(m->ram, m->size, &x, PR_TARGET_SHIFT_DOSBOX386, &dg))
ADAPT(h_2d38, pr_u38_2d38_cpu, pr_u38_record_roll_2d38(m->ram, m->size, &x, &m->status_raw))


/* +04BA (slots 12 and 38): a credited CPU-level model that composes the two U9E callees (+110D, +10E4). u9e_cs is the dispatcher's own code segment (the return CS of the far call that entered the handler),
 * where the loaded U9E callees are; the loop-back budget is the host cap on the candidate-list walk. */
static int h_04ba(call_t *m)
{
    pr_u38_04ba_cpu x; int s; uint16_t u9e_cs = rd16(m->ram, m->cpu->ss, (uint16_t)(m->cpu->sp + 2u));
    COPY(&x, m->cpu); s = (int)pr_u38_pair_04ba(m->ram, m->size, &x, u9e_cs, m->lim->loopbacks); COPY(m->cpu, &x);
    return s;
}

typedef int (*handler_fn)(call_t *);
typedef struct { uint16_t entry; handler_fn fn; } handler_t;
/* the loaded stub's far JMP offset (the unit-relative code offset) -> its composed model */
static const handler_t HANDLERS[] = {
    {0x010D, h_010d}, {0x011D, h_011d}, {0x0C06, h_0c06}, {0x0C15, h_0c15}, {0x171F, h_171f}, {0x25A2, h_25a2}, {0x2A6D, h_2a6d}, {0x2B63, h_2b63}, {0x10C1, h_10c1}, {0x03E1, h_03e1}, {0x06E4, h_06e4}, {0x0A3D, h_0a3d}, {0x2A28, h_2a28}, {0x05D6, h_05d6}, {0x064E, h_064e}, {0x0BAC, h_0bac}, {0x0C26, h_0c26}, {0x1452, h_1452}, {0x2490, h_2490}, {0x2700, h_2700}, {0x013B, h_013b}, {0x0377, h_0377}, {0x03AC, h_03ac}, {0x0918, h_0918}, {0x12B2, h_12b2}, {0x25BC, h_25bc_t}, {0x2D38, h_2d38}, {0x04BA, h_04ba}, {0x0647, h_nop}, {0x1355, h_nop}, {0x135C, h_nop}, {0x319B, h_nop},
    {0x0679, h_0679}, {0x0947, h_0947}, {0x11F9, h_11f9}, {0x1276, h_1276}, {0x133F, h_133f}, {0x1490, h_1490},
    {0x29B6, h_29b6}, {0x29CB, h_29cb}, {0x29D9, h_29d9}, {0x29F0, h_29f0}, {0x2A11, h_2a11},
    {0x2976, h_2976}, {0x2984, h_2984}, {0x2992, h_2992}, {0x2DE4, h_2de4}, {0x2E08, h_2e08},
    {0x264B, h_264b_t}, {0x2A87, h_2a87_t}, {0x2AE6, h_2ae6_t}, {0x2B7D, h_2b7d_t}, {0x2D88, h_2d88_t},
    {0x2B20, h_2b20_t}, {0x1363, h_1363_t}, {0x1472, h_1472_t}, {0x1730, h_1730_t}, {0x27D9, h_27d9_t}, {0x2814, h_2814_t}
};
static handler_fn handler_for(uint16_t entry)
{
    size_t k;
    for (k = 0; k < sizeof HANDLERS / sizeof HANDLERS[0]; ++k) if (HANDLERS[k].entry == entry) return HANDLERS[k].fn;
    return NULL;
}

int pr_u9e_dispatch_0000_slot_kind(unsigned slot) { return slot < 256u ? SLOTS[slot].kind : -1; }
uint16_t pr_u9e_dispatch_0000_slot_stub_off(unsigned slot) { return slot < 256u ? SLOTS[slot].stub_off : 0u; }
uint16_t pr_u9e_dispatch_0000_slot_stub_seg(unsigned slot) { return slot < 256u ? SLOTS[slot].stub_seg : 0u; }
uint16_t pr_u9e_dispatch_0000_slot_entry(unsigned slot) { return slot < 256u ? SLOTS[slot].entry : 0u; }
uint32_t pr_u9e_dispatch_0000_slot_id(unsigned slot) { return slot < 256u ? SLOTS[slot].id : 0u; }

pr_u9e_0000_status pr_u9e_dispatch_0000_cpu(uint8_t *ram, size_t ram_size, pr_u9e_0000_cpu *c, size_t *budget,
                                            const pr_u9e_0000_limits *limits, pr_u9e_0000_diagnostic *diagnostic)
{
    pr_u9e_0000_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t home_cs, ret_ip, ret_cs, off, seg, stub_off, stub_seg;
    unsigned idx;
    const slot_t *sl;
    if (ram == NULL || c == NULL || budget == NULL || limits == NULL || ram_size != PR_U9E_0000_RAM_SIZE || c->ip != 0x0000u || c->sp < 0x0200u || c->sp > 0xFFEEu)
        return PR_U9E_0000_ARGUMENT;
    memset(d, 0, sizeof *d); d->slot = -1; d->callee_status = -1;
    idx = rd8(ram, c->ss, (uint16_t)(c->sp + 0x0Eu));
    d->slot = (int)idx;
    if (idx == 0u || idx >= 140u) return PR_U9E_0000_REFUSED;          /* the declared domain: 1..139 only; nothing has been touched */
    sl = &SLOTS[idx]; d->handler_id = sl->id;
    home_cs = c->cs; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); AT(0x0001u);                                                         /* 0000 PUSH BP */
    STEP(); c->bp = c->sp; AT(0x0003u);                                                               /* 0001 MOV BP,SP */
    STEP(); push(ram, c, bp16(ram, c, 0x0Eu)); AT(0x0006u);                                           /* 0003 PUSH [BP+0E] */
    STEP(); push(ram, c, bp16(ram, c, 0x0Cu)); AT(0x0009u);                                           /* 0006 PUSH [BP+0C] */
    STEP(); push(ram, c, bp16(ram, c, 0x0Au)); AT(0x000Cu);                                           /* 0009 PUSH [BP+0A] */
    STEP(); push(ram, c, bp16(ram, c, 0x08u)); AT(0x000Fu);                                           /* 000C PUSH [BP+8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x0012u); /* 000F MOV AL,[BP+6] */
    STEP(); push(ram, c, c->ax); AT(0x0013u);                                                         /* 0012 PUSH AX */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp + 0x10u))); AT(0x0016u); /* 0013 MOV AL,[BP+10] */
    STEP(); c->ax = (uint16_t)(c->ax & 0x00FFu);                                                      /* 0016 XOR AH,AH (CF=OF=0, ZF=1, SF=0, PF=1, AF=0) */
    c->flags = (uint16_t)((c->flags & ~ARITH) | 0x0044u); AT(0x0018u);
    STEP(); c->di = c->ax; AT(0x001Au);                                                               /* 0018 MOV DI,AX */
    STEP(); c->di = shl1(c, c->di); AT(0x001Cu);                                                      /* 001A SHL DI,1 */
    STEP(); c->di = shl1(c, c->di); AT(LCALL_IP);                                                     /* 001C SHL DI,1 */
    if (*budget == 0u) return PR_U9E_0000_LIMIT;                                                      /* 001E LCALL [DI+6828h]: the budget is read before the slot's verdict */
    off = rd16(ram, c->ds, (uint16_t)(c->di + 0x6828u)); seg = rd16(ram, c->ds, (uint16_t)(c->di + 0x682Au));
    d->stub_off = off; d->stub_seg = seg;
    if (sl->kind != K_RAW && sl->kind != K_ADAPTED) {                                                 /* a NAMED gap: stop at the LCALL, nothing pushed, nothing called */
        d->handler_id = sl->id;
        d->gap = sl->kind == K_GAP_SEMANTIC ? PR_U9E_0000_GAP_SEMANTIC : sl->kind == K_GAP_DEFERRED ? PR_U9E_0000_GAP_DEFERRED : PR_U9E_0000_GAP_UNCREDITED;
        return PR_U9E_0000_GAP;
    }
    stub_off = sl->stub_off; stub_seg = sl->stub_seg;
    if (off != stub_off || seg != stub_seg) return PR_U9E_0000_CODE;                                  /* the table word is not 31A2's */
    if (rd8(ram, stub_seg, stub_off) != 0xEAu) return PR_U9E_0000_CODE;                               /* the stub is not loaded (not a far JMP) */
    {
        uint16_t jo = rd16(ram, stub_seg, (uint16_t)(stub_off + 1u)), js = rd16(ram, stub_seg, (uint16_t)(stub_off + 3u));
        handler_fn fn = handler_for(sl->entry);
        call_t m; int s;
        if (jo != sl->entry || fn == NULL) return PR_U9E_0000_CODE;                                   /* the loaded stub does not enter the original handler offset */
        --*budget; push(ram, c, home_cs); push(ram, c, DISPATCH_RETURN_IP);                           /* the LCALL; the stub's own JMP is uncounted */
        c->cs = js; c->ip = jo;
        m.ram = ram; m.size = ram_size; m.cpu = c; m.lim = limits; m.status_raw = -1;
        s = fn(&m);
        if (s != 0) { d->callee_status = s; return PR_U9E_0000_CALLEE; }
        if (c->cs != home_cs || c->ip != DISPATCH_RETURN_IP) return PR_U9E_0000_TRANSFER;            /* the handler returned somewhere else; CPU left at the popped CS:IP */
    }
    STEP(); c->bp = pop(ram, c); AT(0x0023u);                                                         /* 0022 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 0x000Cu);            /* 0023 RETF 000Ch */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U9E_0000_TRANSFER;
    return PR_U9E_0000_RETURNED;
}
