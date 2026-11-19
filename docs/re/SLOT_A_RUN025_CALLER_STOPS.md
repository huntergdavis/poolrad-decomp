# Saved slot-A Up: run 025 caller stops (`345B`, `348D`, `3492`)

A fresh isolated native run, `t4492-slot1-capture-025`, reached the unit `0x1C`
caller stops of [the post-EXIT note](SLOT_A_POST_EXIT_NATIVE_RETURN.md) in one
invocation: slot A loaded, breakpoints at `3582:345B`, `3582:348D` and `3582:3492`,
then exactly one Up. The game media, full RAM dumps and screenshots stay in ignored
`resources/`; this note holds only hashes, addresses and numeric state. It is
evidence for those three stops only. It does not replace the hash-locked
audit of the original run in the post-EXIT note, and it claims no complete one-Up
frame, no same-invocation C route, and no function count or credit.

## Pins

Ignored inputs: `GAME.OVR` (206,105 bytes) SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; the loaded
55-byte caller at `3582:345B..3491` is
`785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a` at every stop
(the original span digest is in the post-EXIT note).

| Stop | RAM SHA-256 (655,360 B) | CPU file SHA-256 |
| --- | --- | --- |
| `345B` | `82f6f9c602dd9d40b3ff7b85041e6cbfd2a7add6347153fde8412c6350d2e981` | `63e76717d312b802c063ba421fb74cab7fec81d9d2577a5e3c2be91e0c9d8826` |
| `348D` | `8c9aa3eee434e0bcc9ee3b260a588f2df645ee12a1fb6cbab0ea90b89eed0ea0` | `eacba5ce0db51b360c0ba676dd876e507f31a7733a4ed45bdb404e6f99dd3d8b` |
| `3492` | `f66a6d9a9234121c4d96e3b81adfddc818deb36c4973d87c4622a7628511c04c` | `ccbac0c055fc1868b5ceef5b4942d2eb72422111678d9cc7aebb0ae7d4e95433` |

The `348D` CPU file has the same hash as the `CPU-348D` transcription already pinned
by the run003 caller replays. The CPU files are single lines, hex:

```text
345B  AX=0000 BX=3ED4 CX=0000 DX=0000 SI=3E8A DI=84CA BP=3EF8 SP=3EF6 CS=3582 IP=345B DS=14A0 ES=14A0 SS=1D16 FLAGS=7202
348D  AX=0004 BX=3ED4 CX=0000 DX=0000 SI=3E8A DI=0000 BP=3EF8 SP=3EF6 CS=3582 IP=348D DS=14A0 ES=3996 SS=1D16 FLAGS=7246
3492  AX=0000 BX=3DA8 CX=0000 DX=03C5 SI=3D9A DI=6B06 BP=3EF8 SP=3EF6 CS=3582 IP=3492 DS=14A0 ES=14A0 SS=1D16 FLAGS=7246
```

## Measured state

Each RAM is the ten 64 KiB conventional segments, reassembled. At every stop the
window-0 pointer is `3996:0000`, the caller gates `[442E]`, `[442F]` and `[49FF]`
are zero, and the stage bytes `[84E0]`, `[84E1]` are zero. Selected fields:

| Stop | party `(x,y,facing)` | window-0 `+18E / +190 / +192` | window-0 `+1E0 / +1E2` |
| --- | --- | --- | --- |
| `345B` | `(15,4,6)` | `0 / 5 / 10` | `1 / 4` |
| `348D` | `(15,4,6)` | `0 / 5 / 10` | `15 / 4` |
| `3492` | `(14,4,6)` | `1 / 5 / 10` | `15 / 4` |

* `345B → 348D` changes exactly one byte, physical `0x39B40` (window 0 `+1E0`),
  `1 → 15`. This is the same single write the post-EXIT note attributes to the
  caller prefix.
* `348D → 3492` changes **6,201** bytes: party `(15,4,6) → (14,4,6)`, window-0
  `+0x18E` `0 → 1`, and `+0x190/+0x192` unchanged. The count includes transient
  overlay and display bytes; this note attributes none of them.

The private audit reproduces these numbers from hashes and offsets only:

```sh
python3 tests/replay/audit_slot_a_run025_caller_stops.py GAME.OVR \
  RAM_345B RAM_348D RAM_3492 CPU_345B CPU_348D CPU_3492
```

It locks all inputs by size and SHA-256, checks the loaded caller digest and its
unrelocated prefix at each stop, the exact one-byte delta, the 6,201 count, the CPU
line fields, and the party/window tuples above. It prints no game or RAM payload.

## Limits

* The CPU files are manual transcriptions of debugger `EV` output, not an independent
  register audit; the debugger's own breakpoint display is the only stop receipt.
* At cleanup the observer record exited 125 with no `post-audit.txt`, the controller
  reported a wait identity/status mismatch, and the supervisor had to SIGKILL the
  debugger-held process. The capture record is partial in that sense.
* The run's own manifest verifies 16 of 17 files: it listed `actions.log`, which was
  appended to after hashing. The per-file hashes above are the pins; the manifest
  hash is not used.
* Only the 640 KiB conventional range was dumped. No device, EGA or high-memory
  state was captured, and none is zero-filled.
* This run has no `0D6D` or `0742` stop. It supports the caller stops only; the
  entry-1 and resident-dispatch boundaries rest on the earlier runs described in
  [the raw entry-1 handoff](SLOT_A_RAW_ENTRY1_0742.md) and
  [the continuous prefix](SLOT_A_CONTINUOUS_C_1386.md).
