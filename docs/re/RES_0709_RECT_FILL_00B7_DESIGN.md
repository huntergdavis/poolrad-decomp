# 0709:00B7 over `pr_ega`: step-2 design (not yet built)

This is the next step of [the RES 00BA:16EC port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan),
written on 2026-10-18 as a handoff. Its pieces are merged or open:

- the port environment `pr_port_env` (#1038);
- `0709:0000` (#1038) and `0709:00A0` (#1039);
- the EGA model `pr_ega` (#1045).

## The routine (`0709:00B7..0170`, 186 bytes, 89 instructions, `RETF 0Ch`)

**Frame.** `[BP+6]` colour, `[BP+8]` page, `[BP+0Ah]` bottom, `[BP+0Ch]`
right, `[BP+0Eh]` top and `[BP+10h]` left. All are bytes that are
sign-extended with `CBW`.

1. **Save and compute.**
   - `[BP-1] := DS:[4A1Ah]`.
   - `[BP-3] := top × 320 + left`, computed as `CBW; MUL 28h; SHL 3; XOR
     DX,DX; ADD; ADC`. `MUL`'s and `SHL`'s undefined flags die at `XOR`.
   - `[BP-5] := (bottom − top + 1) × 8 − 1`. `SHL`'s defined CF survives
     `DEC`.
2. **Set up.** `PUSH page; PUSH CS; CALL 00A0` selects the page, `PUSH CS;
   CALL 0000` resets the EGA, then `OUT 3CEh/3CFh` sets GC5 to 2 (write
   mode 2).
3. **Loop.** `for i := 0 to [BP-5]`, with the word in `[BP-9]` and i in
   `[BP-7]`. `XOR AX,AX; CMP AX,[BP-9]; JA 0151` can never be taken, because
   0 is never above an unsigned word. So the body always runs, and this
   outcome is unreachable for coverage. Each pass:
   - pushes `ES := DS:[4A18h]` and `DI := [BP-3]`, then `right − left +
     1` and the colour;
   - calls `LCALL 0AF8:16A8`, SYSTEM FillChar, whose `REP STOSB` writes
     **video memory**;
   - does `ADD WORD [BP-3],28h`, then `CMP [BP-7],[BP-9]; JNE 0121` (`INC
     [BP-7]`).
4. **Restore.** GC5 goes back to 0, then `DS:[4A1Ah] := [BP-1]` and `CALL
   00A0` with it. `MOV SP,BP; POP BP; RETF 0Ch`.

A reversed rectangle makes the FillChar count or the row word wrap: up to
65,535 bytes per row, and up to 65,536 rows. So the model needs a cap on
the taken `JNE 0121`.

## The pieces to build

1. **`pr_bus`.** A small memory and port bus: `{ uint8_t *ram; pr_ega
   *ega; const pr_port_env *ports; }`.
   - `pr_bus_read8` and `pr_bus_write8` send physical `A0000h..AFFFFh` to
     `pr_ega_read`/`pr_ega_write` when an EGA is attached, and everything
     else to RAM.
   - `pr_bus_out` sends `OUT`s to `pr_ega_out` and, when present, to
     `ports->out`, which records the trace or refuses.
   - An optional hook logs every `A000` write as (offset, value) for the
     Unicorn half.
2. **A bus-aware FillChar.** It has the same CPU effects as the covered
   SYSTEM FillChar at `0AF8:16A8`:
   - `MOV BX,SP; LES DI,[BX+8]; MOV CX,[BX+6]; MOV AL,[BX+4]; CLD; REP
     STOSB; RETF 8`;
   - every store goes through `pr_bus_write8`;
   - after it, BX is the entry SP, CX is 0, ES:DI has advanced, AL is the
     colour and DF is clear; FLAGS are otherwise untouched.
3. **Compose the callees.**
   - `0709:0000` runs through an adapter `pr_port_env` whose `out` forwards
     to `pr_bus_out`.
   - `0709:00A0` composes as is, since it is pure.
   - The `LCALL 0AF8:16A8` follows the U7D `+0565` convention: the opcode
     and offset are checked at the site, and the segment is read from RAM.
4. **Statuses.** `OK`, `ARGUMENT`, `CODE` (the `LCALL` bytes), `CALLEE` (a
   `PORT` refusal inside `0000`), `PORT` (one of `00B7`'s own four `OUT`s
   refused), `TRANSFER` and `LIMIT`.

## Evidence plan

- **Unicorn.** Hook `OUT` with `UC_HOOK_INSN`, and log `A000` writes with
  `UC_HOOK_MEM_WRITE` on `A0000h..AFFFFh`.
  - The 1 MiB RAM outside the `A000` window, all 14 words, the ordered
    `OUT` events and the ordered `A000` write log must equal the C
    model's, with no mask.
  - Unicorn has no planar memory, so plane effects are checked only
    natively.
  - Coverage claims all starts and every conditional outcome except the
    unreachable `JA 0151`.
- **DOSBox-X** (`machine=ega`, mode `0Dh`).
  - The harness copies `0709:0000..0170` to its own offsets. It places
    FillChar at absolute `0AF8:16A8`, COM offset `44E8h` with the COM at
    `0814`.
  - It clears the planes, runs one rectangle per case on page 0 or 1, and
    dumps the touched planes, for example 16 KiB per plane.
  - The plain run compares CPU, stack, data segment and planes. The traced
    run's `OUT` events must equal the C model's.
- **Focused test.** Hand-predicted plane bytes for small rectangles in write
  mode 2, plus the restored page and the restored write mode.

Once `00B7` is ready, `03F1:0137` (188 instructions) and `0709:0DD3` (190
instructions) build on the same bus with their own `00B7` calls. The
`01DF` blitter (1,036 bytes) follows.
