# Slot-A one-Up minute fields: static writer identified, live route unproved

**Later result:** [the native write watch](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md)
measured the `+0x18E` change and observed this writer on exactly one saved
Up. The analysis below records the earlier static-only boundary.

The owner-held DOS oracle displays 10:50 before one Up key and 10:51
afterward. The merged [unit `0x7D` entry-1 audit](U7D_ENTRY1.md) now
identifies a **static instruction that rewrites** restored window-0 words
`+0x18C..+0x199`, including the minute fields `+0x18E/+0x190`.
**Execution of that instruction on the saved one-Up route, and the
post-Up memory delta, remain unproved.** The screenshot capture retained
no post-Up memory state. This PR adds no C translation or count change.

Before debugging, `deja "Poolrad slot A Up minute writer window0 0x18E 0x190 50 51"`
returned only the new assignment, with no earlier writer trace. On the
PR #142 review update, `deja "Poolrad unit 0x7D entry 1"` found the
now-merged unit `0x7D` work; its code span and store were rechecked
against the ignored binary. Reused
and rechecked: [the merged slot-A ECL gate](SLOT_A_UP_ECL_GATE.md) (PR
#140), [the Up preflight](U68_UP_PREFLIGHT_068D.md) (PR #138),
[the gameplay loop](GAMEPLAY_PATH.md),
[the merged coordinate writer](U68_COORDINATE_WRITER_073B.md) (PR #141),
[the merged unit `0x7D` entry-1 model](U7D_ENTRY1.md) (PR #142),
[the save-frame map](../formats/DOS_SAVGAM.md),
[the ECL opcode ledger](ECL_CONTRACTS.md), and
[the resident display reader](RESIDENT_ROUTE_290B.md). Claude p6 owns
unit `0x68:+0x073B` through the next prologue; this note uses only its
published x/y write-site addresses and does not implement that span.

## Hash-locked inputs and the observed boundary

| Ignored local input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| saved ECL region `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |

The save has window-0 words `0x49C7=0`, `0x49C8=5`, `0x49C9=10`. The
hash-locked resident display body `RES_00BA:290B`, image
`0x034AB..0x036FD`, file `0x0356F..0x037C1`, 595 bytes / 238 decoded
instructions, SHA-256
`197d6b93ad7761754f5cd680c5c16c12dccff53bcc6137764c09f16506172c78`,
loads window-0 pointer `[0x49D2]` at image `0x0354C` and `0x03558`,
multiplies record word `+0x190` by 10 at `0x03553`, adds word `+0x18E`
at `0x0355C`, and passes the result at `0x03563`. The saved values yield
50. The body directly writes only its stack frame; its formatting and
display callees remain separate effects. These are **read sites**, not a
writer proof.

The merged one-Up gate audit establishes ECL `0x9914 → 0x9934` equal
branch `0x993B → 0x996B`, then AND to `0x6E82` and EXIT at `0x9974`;
CALL `0x9945` is bypassed for the zero gate. In the later search entry,
`0x9975` SAVE targets `0x6DE1`, then `0x997B` invokes PICTURE. The
gameplay loop calls unit `0x68` entry 1 between run and search. In its
decoded caller path, a result of 1 from unit `0x3BD` entry 3 sets the
local movement flag, and a nonzero flag calls the x/y writer at
`+0x102E → +0x073B`. The saved cell lookup supports that result **if
the restored map and selector remain as saved**; no guest PC trace proves
this branch in the one-Up replay. The x/y writer's published sites are
`+0x0752/+0x0761`; its own body has no direct window-0 write. Its
last far call is the unit `0x7D` entry-1 static writer below.

## Static minute-field store and conditional caller path

Unit `0x7D` entry 1 is exact span `+0x03D5..+0x0479`, GAME.OVR file
`0x241A1..0x24245`, 165 bytes / 67 instructions, SHA-256
`7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4`.
Its write-back loop at `+0x043A..+0x0466` stores all seven window-0
words `+0x18C..+0x199`. The exact store is `mov word ptr
es:[di+0x6E00], dx` at **`+0x045D`** (file `0x24229`); loop indexing
selects each word in turn. Its near normalizer `+0x02F2` may carry into
the next local word before write-back. The coordinate writer calls
entry 1 with `(n, idx)=(1,1)` when window-1 word `+0x594` bit 0 is clear,
or `(1,2)` when set. Those indices select the increment of window-0
word `+0x18E` or `+0x190` respectively; **all seven words are rewritten
on either call**. See [U7D_ENTRY1.md](U7D_ENTRY1.md) for the bounded
model, normalizer, and unresolved host effects.

This establishes a statically connected writer, not that the conditional
unit `0x68` route ran in the captured one-Up replay. It also does not
establish the runtime carry limits or prove that the displayed 50→51
corresponds to a changed `+0x18E` word.

## Static writer search, with its limits

Starting only from saved search entry `0x9975`, a conservative walk of
decoded fallthroughs, both sides of each IF, GOTO/GOSUB targets, and all
ON GOTO/GOSUB list targets found **697 distinct opcode starts**, no
decode errors, **104 SAVE opcodes**, no ECL CLOCK opcode `0x34`, no
tag-`0x03` indirect operand among the fixed operands, and no direct
operand or SAVE destination `0x49C7/0x49C8/0x49C9`. The walk is an
overapproximation, not a one-Up execution trace. The `0x9914` run
prefix was separately checked in the merged gate note and likewise has
no minute destination. Thus the saved script gives no direct minute
store, but a native handler or callee may update the record indirectly.

The first search-side native handler, unit `0x1C:+0x0856` (PICTURE), has
an exact 251-byte / 91-instruction body at GAME.OVR file `0x0159C`,
SHA-256
`51628c4f1c9879395122bd027bd60463b7ad58181b768d48be6f1b697ee17663`.
Its own window-0 operation reads record `+0x1CC`; its direct writes
target other globals. Its called overlay/resident routines are not
excluded by that local write scan. Unit `0x68` entry 1
`+0x0D6D..+0x10A6`, file `0x17438`, 826 bytes / 350 instructions,
SHA-256
`a28055269b7373f984337022d88bfdea89993f8eb4a44ee69d6d8c23b2240c55`,
has no direct store to the minute offsets; its reachable callees include
the static unit `0x7D` writer above, so its own store scan is not a
closed effect proof. The separate internal `+0x0818`
routine directly writes window-0 `+0x186/+0x188` on two branches,
not `+0x18E/+0x190`; it is not identified as the minute writer.

A read-only scan of the loaded START image found six `LES DI,[0x49D2]`
sites, all in `RES_00BA:290B` (`0x034CB`, `0x034D7`, `0x034FF`,
`0x0354E`, `0x0355A`, `0x035A4`). The scan cannot rule out a resident
routine receiving a copied far pointer. The unit `0x7D:+0x045D`
computed-offset store illustrates why a literal-displacement scan alone
cannot rule out a writer.

## Native-watch blocker and next proof

The installed DOSBox-X is version `2026.01.02`. It accepts
`-break-start`, but its binary has no `BPM` command text. The upstream
[debugger source](https://dosbox-x.com/doxygen/html/debug_8cpp_source.html)
builds memory-change breakpoints only under `C_HEAVY_DEBUG`; the
[official build guide](https://github.com/joncampbell123/dosbox-x/blob/master/BUILD.md)
uses `--enable-debug=heavy`. The matching source tag exists, but this
host has no autoconf/automake for that private rebuild. The existing
one-Up capture script keeps screenshots and a log, not a memory dump.
Consequently no watchpoint hit, post-Up word value, or **executed** owner
CS:IP is available from this run, despite the identified static store.

A decisive replay should break after slot A restores window 0, read its
actual far pointer from `[0x49D2]`, watch both words at offsets
`+0x18E/+0x190`, and run exactly one Up key. Record the first changed
word, owner CS:IP, caller stack, and pre/post values as addresses and
numbers only. This also tests whether the displayed +1 corresponds to
a persistent word write and whether unit `0x7D:+0x045D` executes. No
game bytes or private capture should enter Git.

## Next parity-friendly C target

For a separate parity-gated C task, nominate **unit `0x68:+0x05AB`**,
file `0x16C76..0x16CB8`, 67 bytes / 25 instructions, SHA-256
`f757b69105918b46cfc641ccdfafd78a00f0680dd9e5280898517ea8ac86fd0d`.
It is a call-free, bounded scan of up to 21 bytes at a far record
`+0x17..+0x2B`, returning the first matching index or `0xFF`.
Near calls at `+0x0614` and `+0x066D` are in the same unit; the latter
is inside `+0x0648`, which unit `0x68` entry 1 calls at `+0x0EE4` and
`+0x1021` on its decoded branches. This gives a connected, small C
candidate with synthetic hit/miss/boundary cases. Its exact live
one-Up reachability and game-specific role still need caller evidence
before any strict or identity credit.

No C routine is added and no count is claimed in this evidence note.

Reproduce the address-only static counts and hashes with the ignored
owner-supplied inputs:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/slot_a_minute_static_audit.py \
  resources/oracle-runs/slot-a-walk-001/POOLRAD
```
