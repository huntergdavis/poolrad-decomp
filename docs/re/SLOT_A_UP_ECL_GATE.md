# Saved slot-A Up ECL gate and continuation

This note checks the restored slot-A script run entry at ECL PC `0x9914`,
whose conditional CALL at `0x9945` was identified in [PR #136](https://github.com/huntergdavis/poolrad-decomp/pull/136).
The result is a **bounded reachability finding**, not a native replay or a
new full C translation. Entry 19 remains an isolated translated routine;
this Up observation supplies no reachability credit for it.

Before this audit, `deja "Poolrad slot A ECL Up key saved script 0x9914 0x9945 reachability"`
found no earlier technical trace of this exact gate. Reused and checked:
[the PR #136 entry-19 note](U25_MOVE_ENTRY19.md) for the CALL operand and
caller, [the save-frame map](../formats/DOS_SAVGAM.md) for the copied
windows and party bytes, [the ECL opcode table](ECL_CONTRACTS.md),
[the merged COMPARE/IF contracts](FUNCTION_LEDGER.md),
[the gameplay-loop route](GAMEPLAY_PATH.md),
[the cell lookup](U3BD_FIELD_ENTRY3.md),
[the merged Up preflight proof](U68_UP_PREFLIGHT_068D.md), and
[the resident display-field note](RESIDENT_ROUTE_290B.md).
All addresses and hashes below were checked again against ignored local
inputs. The Up-key preflight and coordinate writer are owned by Claude p6;
this audit does not change either span or its recovered source.

## Input and saved state

| Ignored input | Size | SHA-256 |
| --- | ---: | --- |
| `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| saved script file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GEO2.DAX` | 2,572 | `2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595` |

The save reader maps file `0x0001..0x0800` to window 0 and
`0x0801..0x1000` to window 1. Each script word has a two-byte little-endian
slot. Therefore window-1 word `0x6DD5` is at index `0x2D5`, byte offset
`0x5AA`, file offset `0x0DAB`. The saved word is **zero**. Window-0 word
`0x49C9` is 10. The five saved script entry words are `0x9914`, `0x9975`,
`0x9A0E`, `0x9A49`, and `0x9A5E`; logical PC `0x9900` maps to the beginning
of the script region, including its header. The saved party tuple is
`(x,y,facing)=(15,4,6)` and selector `[0x49FA]` is 1.

## Run-entry branch, byte-checked at every listed PC

| PC | Decoded operation | Consequence on the saved values |
| --- | --- | --- |
| `0x9914` | SAVE immediate 11 to `0x6E7D` | Does not write `0x6DD5`. |
| `0x991A..0x9920` | COMPARE `0x49C9` with 14; IF >= | Saved 10 < 14, so the single SAVE at `0x9921` is skipped. Even if taken, it targets `0x6E7D`. |
| `0x9927`, `0x992E` | SAVE to `0x49FD`, then `0x49FE` | Neither targets the gate. |
| `0x9934..0x993A` | COMPARE word `0x6DD5` with zero; IF = | The merged IF handler executes the next statement when equal and skips it otherwise. Saved zero makes equality true. |
| `0x993B` | GOTO `0x996B` | Executed for a zero gate. It bypasses SAVE `0x993F` and CALL `0x9945`. |
| `0x993F`, `0x9945` | SAVE zero to `0x49FB`; CALL `0xC01E` | Reached only with a nonzero gate, subject to the preceding instructions completing. PR #136 identifies the CALL's conditional entry-19 dispatch. |
| `0x996B..0x9974` | AND `0x7F` with pseudo source `0xC04F`, store to `0x6E82`; EXIT | The zero-gate path ends at `0x9974`. No direct x, y, or clock-field destination appears in this operation. |

The `0x996B` opcode is handled by unit `0x1C:+0x0DEF..+0x0E5D`,
file `0x01B35`, 111 bytes / 43 decoded instructions, SHA-256
`0c16bd23a8d5f91bf2131b9b8c2ae9cf785a88f919bf2f613ff6abb10f226967`.
It forms a 16-bit AND and calls the normal store-word route for the third
operand. The pseudo source `0xC04F` is outside the merged load-word core,
so its resulting value is not claimed here. The class-1 hook at unit
`0x25:+0x07BC` has no special case for window-1 index `0x2D5`, and the
earlier SAVEs do not target it. The saved-state path emitted by the
read-only verifier is `9914, 991A, 9920, 9927, 992E, 9934, 993A, 993B,
996B, 9974`: **CALL `0x9945` is not reached for the saved zero gate**.

The merged [PR #138 Up preflight note](U68_UP_PREFLIGHT_068D.md) proves
that an in-range Up path clears `[0x6DD5]` and leaves it zero; only a
coordinate clamp sets it to one. It also records the saved zero word
independently. Coupled with this ECL branch, the decoded ordinary in-range
Up route bypasses `0x9945`, provided no untraced resident effect changes
the word between the key handler and the compare. This is stronger than
the saved-state result alone, but remains a static route rather than a
captured guest PC trace.

## Actually selected coordinate writer

After the run entry returns, the gameplay loop calls unit `0x68` entry 1
at `0x1C:+0x348D` and later invokes the search entry `0x9975` at
`0x1C:+0x34E7`. Entry 1 is unit `0x68:+0x0D6D..+0x10A7`, file
`0x17438`, 827 bytes / 351 decoded instructions, SHA-256
`b3fd5c1143ccc14da56fcd3c28a8f29833f701ac2fb483ea35b67260ceee1db9`.
It gates on selector 1 at `+0x0D78`, reads window-1 field `+0x592`
at `+0x0D82`, calls unit `0x3BD` entry 3 with current x/y/facing at
`+0x0D9E`, and, when that result is 1, takes `+0x0DA7` to its local
flag. A nonzero flag calls internal `+0x073B` at `+0x102E`.

The ignored `GEO2.DAX` block 20 is 1,026 unpacked bytes, SHA-256
`8fc671263b8606861bcfac3a6ff33476e2ad1d0d1c1592bf20643aebf8519663`.
The saved current-cell lookup `(15,4,6)` selects a zero nibble; the
reviewed entry-3 contract returns 1 on that cell. This supports the
flag-to-`+0x073B` route if the restored map and selector remain as saved
at the call. It is a conditional static route, not a captured guest PC.

Internal `0x68:+0x073B` is the coordinate writer: its x and y write
sites are `+0x0752` and `+0x0761` for `[0x6AAD]` and `[0x6AAE]`.
Claude p6 owns its full instruction contract. This note establishes
only the caller-side route to it and does not claim its direction-table
values or full behavior. The observed DOS `(15,4)` to `(14,4)` step is
consistent with this route, not a guest-PC capture of it.

## Minute attribution boundary

The ignored slot-A save has window-0 words `0x49C7=0`, `0x49C8=5`, and
`0x49C9=10`. The decoded resident display route `RES_00BA:290B` reads
record offsets `+0x18E`, `+0x190`, and `+0x192` and passes
`10 * word[+0x190] + word[+0x18E]` to its display callee, so the saved
values yield 50. The DOS oracle screenshot reports the displayed minute
changing 50→51 after Up. This screenshot does **not** identify which
instruction wrote the underlying word or when it ran.

The checked `0x9914` path writes neither `0x49C7` nor `0x49C8` nor
`0x49C9`; the `0x996B` AND targets `0x6E82`. The x/y write sites above
target party position bytes. The next search entry starts
at `0x9975` with SAVE `0xFF` to `0x6DE1`, then reaches PICTURE
opcode `0x0E` at `0x997B` before its
later conditional flow. Its handler/callee effects, any upstream Up-key
time effect, and the display callee's indirect effects have not been
traced to a write of the window-0 clock fields. **The minute writer is
unresolved.** The next decisive evidence is a native memory write watch
on the restored window-0 record offsets `+0x18E/+0x190` during the one-Up
oracle, with the owner instruction address and call stack recorded; an
address-only trace would distinguish preflight, entry 1, search, and
display-side effects. No writer is inferred from the displayed change.

## Reproduction and scope

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s tools/overlay -p test_slot_a_ecl_gate.py -v
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/slot_a_ecl_gate.py \
  resources/oracle-runs/slot-a-walk-001/POOLRAD/SAVGAMA.DAT
```

The verifier checks the whole save hash, script hash, five entry words,
each listed gate instruction's opcode/operand/end PC, the two saved
words, and the selected branch. It prints only hashes, addresses, word
values, and a Boolean CALL verdict. Three synthetic tests cover the
zero and nonzero gate paths, the hour-side branch, and rejection of
changed GOTO/CALL/AND operands. No original save, script, text, pixels,
or game binary bytes are in Git. No strict full-C or game-identity count
is claimed.
