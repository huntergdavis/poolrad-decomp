# Unit 0x38 entry 22: sidecar lookup flag at +0x0918

## Selection and provenance

U86 `+0x100F` was dependency-gated before edits: its 47-byte wrapper calls
runtime `AF8:0B0B` and near U86 `+0x0F4F`, whose 190-byte body has three
more call dependencies. PM approved this distinct U38 entry after that
gate. A bounded `deja` query for U38 `+0x0918` and resident `BA:2199`
timed out without a prior-session result. Reused the merged
[unit-38 dispatch evidence](OVERLAY_UNIT38_EFFECT_ENTRIES.md) for the
installer/dispatcher layout and the independently translated
[resident lookup evidence](RESIDENT_SIDECAR_LOOKUP.md) for the callee ABI;
both were checked against the pinned original bytes.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The **complete original function** is GAME.OVR file `[0xF573,0xF5A2)`,
unit `0x38:[+0x0918,+0x0947)`: 47 contiguous bytes / 18 instructions,
SHA-256 `da2ea19cc6e9bfb6da374a2e8f3342839e094563651b39b5758cdc223ca4873e`.
It ends with `RETF 10` at `+0x0944`; public entry 23 begins at `+0x0947`.
The only call is fixup-confirmed `LCALL BA:2199` at `+0x092E` (segment
fixup word `+0x0931`). There are no near calls, loops, or indirect jumps.

## Game caller and effects

Public unit-38 entry 22 resolves to stub `0x41:0x008E`. The game-specific
unit-38 initializer `+0x31A2` installs that pointer at `DS:688C/688E`
at `+0x32DD..+0x32E9`. The merged unit-9E dispatcher at `+0x0000`
indexes the table beginning `DS:6828` by a caller-supplied item byte and
far-calls the selected entry; `DS:688C` is item 25. The dispatcher passes
the same five argument words this routine removes with `RETF 10`. The
routine reads the active character far pointer at `DS:5D92/5D94`; its
resident lookup call uses that character, key `0x18`, and a local cursor
cell at `SS:BP-4`.

Resident `BA:2199` is the separately merged full-C sidecar lookup. It
copies the character's sidecar head into the cursor, follows zero or more
9-byte linked nodes, and returns `AL=1` for a key match or `AL=0` for
exhaustion. The U38 wrapper sets `DS:681E` to 1 when returned `AL=0`,
then always subtracts 4 from byte `DS:6822` with 8-bit wrap. Its final
arithmetic flags are those of that byte subtraction. It preserves the
resident call's other CPU/RAM effects through an explicit callback, then
restores the frame and returns with ten argument bytes removed.

## Verification

The original-byte Unicorn checker runs all 47 source bytes in 96 cases:
empty sidecar head, first-node match, second-node match, and exhaustion;
six DS/SS and stack layouts including global/return-word aliases and
16-bit stack wrap; and four initial `DS:6822` values. Unicorn executes the
original LCALL, writing return IP and CS below its arguments before the
BA:2199 hook; the model writes the same words. Both pop that frame as
`RETF 10` does. It compares the
entire 1 MiB RAM, all 14 CPU fields, and the exact seven-word callee-entry
frame. A focused strict C test invokes the merged
`pr_rec_res_00ba_2199` through a RAM adapter for those four list routes
and tests call abort. It passes C11 `-Wall -Wextra -Wpedantic -Werror`
with ASan/UBSan.

Eight DOSBox-X normal-core 386 cases execute an actual far call into the
original U38 body, with only the loaded resident-call segment operand
redirected to a synthetic stub at the same `+0x2199` offset. That stub
writes a cursor and returns `AL=0/1`; its CPU/RAM effects are mirrored by
the C callback. All 14 returned CPU words, 46 stack-frame bytes including
the far-call return IP and CS, the
cursor, and both U38 globals match. The generated COM, copied body, logs,
and capture stay under ignored `resources/u38-0918-parity/native/`.
Result SHA-256:
`6e325bd412a771859f4cbb81ba361d6c2dc23720cd425ee1e3806d876005ab0e`.

The native replay is a synthetic caller, not a reached game frame; it does
not execute the original resident `BA:2199` body. The resident lookup's
behavior is exercised by the focused C composition and the byte-level
callback contract. This is a strict full-C **candidate** for PM review;
no new identity or count credit is claimed here.

## Correction, 2026-10-21: the LCALL's segment is the live code word, not 00BAh

The C model entered the resident lookup with `CS := 00BAh`, a constant, where the original's `9A` at `092E` carries a segment
word at `0931` that the loader relocates (`00BA` only at the unrelocated base). The original-byte parity (96 cases) and the
focused C test both ran at that base, where the constant and the word agree, and the unit test even asserted the constant, so
neither could see it. The model now enters at the word read from `CS:0931`; the unit test sets the word to `00BA` and to
`1234`, `0001`, `10BA` and `FFF0` and checks the lookup is entered at it (fails on the old model: `cpu->cs == f->word`). The
original-byte parity still passes (its code image holds the real word). Body bytes, hash, identity and credit unchanged.
