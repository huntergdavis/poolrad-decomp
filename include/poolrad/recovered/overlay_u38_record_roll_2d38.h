#ifndef POOLRAD_RECOVERED_OVERLAY_U38_RECORD_ROLL_2D38_H
#define POOLRAD_RECOVERED_OVERLAY_U38_RECORD_ROLL_2D38_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_2D38_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x9E entry models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_2d38_cpu;

typedef enum {
    PR_U38_2D38_OK = 0,
    PR_U38_2D38_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U38_2D38_CODE,       /* the far-call site or its stub is not the loaded-state original */
    PR_U38_2D38_CALLEE,     /* the merged entry-9/entry-8 chain stopped (status in *chain_status) */
    PR_U38_2D38_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u38_2d38_status;

/* Complete GAME.OVR unit 0x38 public entry 106 (+2D38..+2D87, 80 bytes,
 * 29 instructions), entered after the far CALL with five caller words it
 * never reads; RETF 0Ah. It reads the active record pointer DS:5D92/5D94,
 * then that record's far pointer +CC/+CE; when that pointer is nonzero
 * and its byte +2E is 56h it calls unit 0x9E public entry 9 (+0E2E, stub
 * 00B0:004D, count 3, range 8) and stores AL at DS:6818. Then, when
 * DS:6819 has bit 0 or bit 3 set, DS:681C (the latch entry 9 wrote) is
 * added to DS:6818. The far call is modelled as the loaded-state transfer:
 * the relocated segment word is read from the call site at cs:(ip+32),
 * the five-byte far jump at that paragraph's offset 4D supplies the
 * callee CS:IP, the merged pr_u9e_entry9_roll_latch_0e2e runs with the
 * merged entry-8 chain (SYSTEM at 0AF8:11C0, seed at DS:4428) as its
 * callee, and each RETF frame is checked. cpu->ip is the body's load
 * offset (0x2D38 in the game). Every operand is reread from RAM. */
pr_u38_2d38_status pr_u38_record_roll_2d38(
    uint8_t *ram, size_t ram_size, pr_u38_2d38_cpu *cpu, int *chain_status);

#endif
