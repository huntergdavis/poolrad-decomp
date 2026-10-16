# Unit 0x25 +0x1FB1: provisional full-state public entry 23

`pr_u25_entry23_1fb1` translates the complete U25 public entry 23 (far stub
`2B:009B`) over caller-owned A20-off 1 MiB RAM and a fourteen-word CPU
frame. Its only external callee, resident `BA:2199`, is a fail-closed
call-site callback. The merged
[`pr_rec_res_00ba_2199`](RESIDENT_SIDECAR_LOOKUP.md) translation is not
modified or called. **Game identity is not asserted, and no strict full-C,
identity or count credit is requested.**

## Original boundary and ownership

Ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25` starts at file `0x74D3`. The executable routine is exactly file
`[0x9484,0x9551)`, unit `+1FB1..+207D`, **205 bytes / 74 instructions**,
SHA-256 `8d6da69d20a93bea32da791cf65623426c7cd35910e9c9118158c564110882b3`.
Its only return is `+207B RETF 8`, and all jumps are internal.

The ledger CSV's 283-byte bound wrongly includes `+207E..+20CB`. Those 78
bytes are literal data owned by `+20CC`, whose `MOV DI` loads at
`+2117..+21A8` are their only references, and `+20CC`'s prologue follows
them. The text is not reproduced.

The only caller is the far call through `2B:009B` at U1C `+1F1A`. pG
cleared this interval relative to its own work, and pF had already cleared
it. No source, branch or sibling-clone file owned it at `862a498`.

`BA:2199` was decoded from START.EXE expanded with the repository's
`resident_loader_map.unpack`. It sits at image `0x2D39`, with a Pascal
prologue and a sole `RETF 0x000A` at `0x2DA5`. Its arguments are a cursor
far pointer, a key word (low byte) and a character far pointer, and it
returns AL=1 when found. Both call sites here push exactly these five
words, `MOV AL,key; PUSH AX` carrying AH through.

## Behavior

Arguments are two far byte pointers: minimum `[BP+6]` and maximum
`[BP+0xA]`. The frame is `SUB SP,0xA`: node `[BP-4]:[BP-2]`, cursor
`[BP-8]`, value `[BP-9]`.

1. Load the list head from `DS:[5D98]:[5D96]` and seed **both** outputs
   with that head node's byte `+0x11C`. This happens even when the head is
   null, so the byte is read from `0000:011C`.
2. While the node pointer is non-null:
   1. Take its byte `+0x11C`.
   2. Call the lookup with key `27h`. If found, double the value with
      `SHL`, truncated to a byte.
   3. Otherwise call it with key `2Ah`. If found, halve the value.
   4. Raise the maximum if the value is above it, and lower the minimum if
      it is below it.
   5. Follow the link at `+0x104/+0x106`.

Every pointer, byte and output is re-read where the original reads it, so
callee or alias writes (cursor, outputs, list) take effect in source order.
A cyclic list does not return, as in DOS; the C has no host cap.

Flags are exact wherever the hook or the return observes them:

- **First lookup:** the `OR AX,[BP-2]` loop test.
- **Second lookup:** `OR AL,AL` after the first lookup.
- **Return:** the final `OR` that finds the null link.

The `XOR`, `SHL` and `SHR` flags are dead before the next `CMP`. `OR` uses
the reference emulator's cleared AF, which is architecturally undefined.

## Verification

`test_overlay_u25_entry23_1fb1.c` uses a synthetic lookup keyed per node.
It checks:

- 400 seeded lists of 1 to 8 nodes against a reference min/max with byte
  truncation
- an explicit path with doubling overflow, halving and the call order
- the leftover value and cursor
- null-head seeding from `0000:011C` with no calls
- the final flags and return
- an abort at the second call
- argument rejection

It passes strict C11 `-Wall -Wextra -Wpedantic -Werror` and ASan+UBSan.

`check_overlay_u25_entry23_1fb1.py` pins the file and body hashes, both
call targets, internal jumps, the sole `RETF 8`, and `+20CC`'s ownership of
the trailing data. Its hook models the documented `BA:2199` list walk. Its
source pins have run statically. Its Unicorn differential has **not** run
and waits for a PM serial slot. When run, it covers 62 cases:

- empty, single, mixed-key and five-node lists under normal, SS=DS (cursor
  aliasing the head global), low and high stack offsets
- min/max-output and value-byte aliases
- 40 seeded cases

For each case it compares all 1 MiB of RAM, the CPU words and every call
trace, and requires every instruction start and every conditional outcome.

No reached native `+1FB1` frame was searched. Callee and caller native
parity remain open.
