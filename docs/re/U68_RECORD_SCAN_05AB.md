# Unit 0x68 internal +0x05AB: bounded record-byte scan

`pr_u68_record_scan` translates the entire DOS v1.3 `GAME.OVR` body at
unit `0x68:+0x05AB..+0x05ED`. It searches a far record for the first
matching byte in 21 positions and returns its index, or `0x00FF` when
none match. The body has no calls or external writes. This is a
**complete routine translation for a valid DOS segment view**, with
host-only argument guards. PM byte-parity review of PR #145 found a complete
match for valid inputs. The postmerge caller audit below establishes its
game-state ownership, so it earns one strict full-C game-function credit.

Before implementation, `deja "Poolrad unit 0x68 +0x05AB record scan +0x0614 +0x066D"`
and the shorter `deja "Poolrad u68 record scan"` found no prior exact
scan trace. Reused and independently checked the candidate and static
caller connection from [the merged slot-A minute boundary](SLOT_A_MINUTE_WRITER_BOUNDARY.md)
(PR #143), unit `0x68` entry-1 paths from
[the Up preflight note](U68_UP_PREFLIGHT_068D.md) (PR #138), and the
[coordinate-writer note](U68_COORDINATE_WRITER_073B.md) (PR #141).
Those notes do not establish live execution of this scan on the saved
one-Up replay.

## Binary boundary and ABI

The ignored local `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x68` begins at file `0x166CB`. Its scan body is file
`0x16C76..0x16CB8` inclusive: **67 bytes / 25 decoded instructions**,
SHA-256 `f757b69105918b46cfc641ccdfafd78a00f0680dd9e5280898517ea8ac86fd0d`.
Decode ends at the next prologue `+0x05EE`; there is no trailing body
or call to import. The byte-checked last instruction at `+0x05EB` is
`retf 6`.

The two callers each use `push cs; call near +0x05AB`, matching the
far return. On entry, the saved frame contains a far record pointer at
`[bp+6]` (offset) and `[bp+8]` (segment), followed by a word at
`[bp+0xA]` whose **low byte** is the sought value. The callee loads the
far pointer with `LES`, reads the record only, writes its two stack
locals, and returns AX with AH zero. `retf 6` consumes the three word
arguments. The portable API supplies the addressed 64 KiB ES segment,
record offset, word value, and AX output separately. A full segment
view preserves 16-bit effective-offset wrap; missing/short host views
are rejected before access, a guard absent from DOS.

| Offset | Decoded path and effect |
| --- | --- |
| `+0x05B1..+0x05B9` | Set byte index to zero; unsigned `ja` exits the loop after index 20. |
| `+0x05BB..+0x05C9` | Zero-extend index in AX, load far pointer, read `ES:(record offset + index + 0x17)` with 16-bit offset arithmetic, compare with low byte of `[bp+0xA]`. |
| `+0x05CC` | Equal branch goes to the result block; the first hit wins. |
| `+0x05CE..+0x05D1` | Unequal branch increments byte index and loops. |
| `+0x05D3..+0x05E5` | For index 0..20, return `AX=index`; after index 20 misses, return `AX=0x00FF`. `AH` was cleared at `+0x05BE` on every visited position. |
| `+0x05E8..+0x05EB` | Restore frame and `retf 6`; no record write or other callee. |

The inclusive searched record offsets are `+0x17..+0x2B` relative to
the far pointer. A matching value at `+0x2C` is outside the loop.

## Static caller proof and route boundary

An instruction walk of unit `0x68`'s prologue-bounded functions found
**exactly two near calls to `+0x05AB`**:

| Site | Containing routine | Argument and result use |
| --- | --- | --- |
| `+0x0614` | `+0x05EE..+0x0647`, 90 bytes, SHA-256 `a7c44a16897e088df3dd64221b58be8efb68344b21ab3083b0748de5b21dc10f` | Passes the low byte of its own word argument and the current far node from list head `[0x5D96]`. `AL=0xFF` advances through node pointer `+0x104`; otherwise the caller returns that node. |
| `+0x066D` | `+0x0648..+0x068C`, 69 bytes, SHA-256 `16c166e8281ce17e48fb68a5cc95f64c098ede2f6a1f66d85de1f78597c91146` | After `+0x0652` asks the list helper for value `0x1F`, passes the returned node and `0x1F` again. It uses AL as a byte index to clear node byte `+0x17+index`, then returns 1; a null node returns 0 without scanning. |

Entry 1 (`+0x0D6D`) calls list helper `+0x05EE` at `+0x0E18` when
`[0x6DE0]` is nonzero. It also calls `+0x0648` at `+0x0EE4` and
`+0x1021` on decoded branches testing local byte `0x4B`. These are
static caller connections, not a guest trace of the saved Up key.
Neither the branch result nor the live role of the byte-list operation
is asserted from the slot-A screenshot. The scan's 67-byte behavior is
independent of those caller questions.

**Postmerge ownership gate.** The PM independently decoded the two
prologue-bounded callers above against the pinned `GAME.OVR` bytes. The first
reads the documented party-list far pointer `[0x5D96]`, advances members by
`+0x104`, and uses the scan result to select a member. The second calls that
list search for `0x1F`, scans the returned member again, and clears the
selected record byte at `+0x17+index`. Unit `0x68` entry 1 calls these
helpers on decoded game-state branches. This is a game-specific use of the
record layout, not merely a generic search leaf; the only two near calls
resolve to that chain. The 67-byte scan is a complete call-free translation,
with its host guards outside the valid DOS input domain. Promote only
`0x68:+0x05AB` from provisional helper to verified identity and award one
strict full-C credit. Neither caller is translated by this accounting step,
and no live saved one-Up execution is claimed.

## Synthetic verification

`test_overlay_u68_record_scan.c` uses a fabricated 64 KiB segment. It
tests each first-hit index 0..20, index 20 specifically, a miss and
an out-of-range match at index 21, duplicate matches selecting the
first, the ignored high byte of the argument, `AX=0x00FF` versus a
matched value byte `0xFF`, 16-bit offset wrap across `0xFFFF`, and
host argument guards leaving the AX output untouched. No game data or
decoded media is in the test.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u68_record_scan_tests
ctest --test-dir build -R '^poolrad_overlay_u68_record_scan_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_0068_05AB` | the whole `poolrad_overlay_u68_record_scan_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u68_record_scan_tests` passed (1/1 under ctest).
