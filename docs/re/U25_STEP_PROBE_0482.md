# Unit 0x25 +0x0482: provisional full-state public entry 6

`pr_u25_step_probe_0482` translates the complete U25 public entry 6 (far
stub `2B:003E`) over caller-owned A20-off 1 MiB RAM and a fourteen-word CPU
frame. Its single callee, unit 3BD entry 2 (`3D5:002A`), is an explicit
fail-closed call-site callback. The name describes the observed control
flow only; **game identity is not asserted, and no strict full-C, identity
or count credit is requested.** Whole credit remains for PM review.

## Original boundary

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The routine is exactly file
`[0x7955,0x79E4)`, unit `+0482..+0510`, **143 bytes / 52 instructions**,
SHA-256 `bcfe56989d4733d260e0f85a7146d4cb56de3041cbf20b7de27851f0f513df2b`.
Its only return is `+050E RETF 6`. It follows `+0411`'s `RETF 0xE`, and
`+0511`'s prologue follows directly. All jumps are internal.

The callers are three far calls through `2B:003E`, all in unit `0x1C`, at
`+03F5`, `+1803` and `+1FF5`. A decode-aligned scan of every ledger function
found no other far call and no near caller.

Its only call is `+04C9 CALL FAR 3D5:002A`, unit 3BD entry 2: 255 bytes,
`RETF 6`, three byte-word arguments (x, y, selector), with the result in AL
([U3BD_NIBBLE_ENTRY2.md](U3BD_NIBBLE_ENTRY2.md)). Its merged C translation is
not a full-state API, so it stays a callback here.

## Behavior

Arguments are the byte words direction `[BP+6]`, y `[BP+8]` and x
`[BP+0xA]`. Locals are `SUB SP,4`: count `[BP-3]`, stop `[BP-2]` and result
`[BP-1]`, all cleared.

- **Window-0 word `+1CC` is zero:** set result to 2 and store word 2 at
  window-1 `+0582`.
- **Otherwise,** loop while count < 2 and stop is zero:
  1. Push x, y and direction as `MOV AL; PUSH AX`, so AH carries through, and
     call entry 2.
  2. A nonzero AL from `OR AL,AL` sets stop.
  3. A zero AL increments count and copies it to result. The *caller's*
     argument bytes are then stepped in place by direction: 0 decrements y,
     2 increments x, 4 increments y, 6 decrements x, and any other value
     changes nothing.
- **Return** AL = result with `RETF 6`.

Every local, argument, window pointer and window word is re-read where the
original reads it. Callee or SS alias writes, such as to the arguments, the
count or the return words, therefore take effect in source order.

Flags are exact wherever the hook or the return observes them:

- **Hook entry:** the falling-through `CMP [BP-2],0`.
- **Return:** the window `CMP word,0`, or the loop-exit `CMP [BP-3],2` or
  `CMP [BP-2],0`.

`OR AL,AL` is computed with the reference emulator's cleared AF, which is
architecturally undefined. The INC and DEC flags are dead before the next
`CMP` and omitted.

## Verification

`test_overlay_u25_step_probe_0482.c` checks:

- the zero-window path and its final flags
- all four directions plus two no-step values, with the pushed arguments, AH
  carry and in-place argument updates
- the stop and immediate-stop exits with their final flags
- an abort at the second call, with the first step retained
- argument rejection

It passes strict C11 `-Wall -Wextra -Wpedantic -Werror` (also clean with
`-Wconversion`) and ASan+UBSan.

`check_overlay_u25_step_probe_0482.py` pins the file and body hashes, the
single call, internal jumps, the neighbouring bytes and the sole `RETF 6`.
Its source pins have run statically. Its Unicorn differential has **not** run
and waits for a PM serial slot. When run, it covers 157 cases:

- 3 window words × 7 directions × 5 result scripts
- callee rewrites of the direction, count or return words under normal,
  SS=DS, low and high stack offsets
- 40 seeded cases

For each case it compares the C against original-byte execution over all
1 MiB of RAM, the CPU words and every call trace. It requires all 52
instruction starts and all 16 conditional outcomes to be hit.

No reached native `+0482` frame was searched. Callee and caller native
parity remain open.
