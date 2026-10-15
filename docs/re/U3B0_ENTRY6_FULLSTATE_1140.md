# Unit 0x3B0 entry 6 +0x1140: VERTICAL MENU node allocator

The VERTICAL MENU call at unit `0x1C:+0x0F73` targets `3C5:003E`. That
segment is an **overlay stub**, not resident code: the
[overlay inventory](overlay_inventory.md) maps stub `0x3C5` to unit `0x3B0`,
and the [function ledger](overlay_function_ledger.csv) maps public entry 6 to
`+0x1140`. A bounded pre-implementation `deja` query timed out without a
result. Repository leads reused and independently checked against bytes were
[RESIDENT_SIDECAR_APPEND.md](RESIDENT_SIDECAR_APPEND.md) and
[U25_ENTRY5_0411.md](U25_ENTRY5_0411.md) for GetMem's DX:AX and runtime
error boundary, and [U1C_VERTICAL_MENU_PREFIX_0F08.md](U1C_VERTICAL_MENU_PREFIX_0F08.md)
for the exact caller stack at `+0x0F73`.

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete entry-6 body is file `[0x2DA0F,0x2DAC2)`, unit
`+0x1140..+0x11F2`: **179 bytes / 58 instructions**, SHA-256
`fdced3a4a7f0c069d55a9a4fef8a048eca70e649ec66559b84240259d2280c5c`.
It ends in `RETF 6` at `+0x11F0`; the next `+0x11F3` prologue follows
immediately. The sole far callee is SYSTEM GetMem `AF8:023F`, reached at
`+0x114A` and again at `+0x11A8` for each additional node. Its original
START.EXE success-path wrapper begins at file `0xAEB5` (expanded image
`0xB1BF`); the first 15 bytes through `RETF 2` at `+0x024B` hash to
`03f20398c0c606d78882c4e607335778fea88f52b4490cd8d4d0c8c7aa6ac8e3`.
The `JB` at `+0x0249` enters an unresolved heap/runtime-error path.

The first call allocates `0x2E` bytes and returns a far pointer in DX:AX.
The handler writes that pointer through the caller's far output pointer at
`SS:[BP+6]`, then rereads it from memory. It clears the new node's next
pointer at offsets `+0x2A/+0x2C`, byte `+0x29`, and first byte at `+0x00`.
It reads the signed word count at `[BP+0x0A]`: counts at most one create only
the head. For each value from two through a positive count, it allocates
another node, links the prior tail to the returned far pointer, advances by
reading that link, and clears the same fields in the new tail. On return it
consumes six caller argument bytes. It rereads far pointers at each `LES`,
so output/stack/heap aliasing remains observable.

The C model translates all 58 instructions' in-body RAM/CPU/flag and return
effects. It exposes GetMem as a fail-closed callback at the two call sites;
the callback owns all allocator RAM/CPU effects, including DX:AX and any
error. `check_overlay_u3b0_entry6_1140.py` pins the original hashes,
instruction/call boundaries and next prologue, then compares original-byte
Unicorn execution with C in 21 cases: count `0,1,2,3,4,-1,-32768`,
separate and aliased DS/SS, output pointer aliasing the far return frame,
and an output pointer at offset `FFFF`. It checks the complete 1 MiB RAM,
all 14 CPU fields and every allocator call-entry CPU/argument trace. The
offset-`FFFF` case specifically verifies contiguous physical four-byte
`LES` reads across the segment boundary. The focused strict C test checks
node links, field clears, output pointer, far return, and abort at both
allocator sites.

No reached native `3C5:003E` entry/return pair was found in a bounded local
capture search. The GetMem heap core and error path, plus live C-native
parity, remain open. This is **provisional**: no new identity, strict full-C,
or count credit is requested. It resolves the VERTICAL MENU pointer-shape
dependency through an explicit allocator callback; the caller's subsequent
menu/list effects still require separate work.
