# U77 `+275B`: canvas capacity check over two credited residents

**Body.** GAME.OVR unit `0x77`, `[275B,27DF)`, **132 bytes / 45 instruction
starts**, terminating `RETF 8`. Body SHA-256 `91d407737828c3a1912ca24a72d2260b43f0d99d93028174bd9325a3419435b7`.
The extent ends at `27DF` because `RETF 8` at
`27DC` occupies three bytes; a first-`RETF`-exclusive read reports 130 and is
wrong.

**What it does.** Two far pointers in the frame: target at `[bp+0x0A]` with its
**segment at `[bp+0x0C]`**, and the record at `[bp+0x06]`/`[bp+0x08]`. It reads the **target** object's `+0xC7` (type byte, compared against 15) and the
**record** object's `+0x37` (quantity word) and `+0x39` (count byte), multiplies quantity by count when the count is
non-zero, adds the composed `00BA:13AB` result plus **1500**, and compares that
sign-extended 32-bit total with the first pointer's `+0x102` word plus the
product. `AL` returns the flag; it is also stored at `[bp-1]`.

**Composition.** Both far calls compose credited raw residents — `00BA:0BB8`
through `pr_res00ba_0bb8_cpu` and `00BA:13AB` through `pr_res00ba_helpers_cpu`
with entry `13AB`. No callback stands in for original code.

## Ownership

Two callers, both in game code:

* **Far, fixup-confirmed:** unit `0x23` at `+0239` (exact prologue
  `push bp; mov bp,sp; sub sp,14h`) issues `lcall 0x0085:0x004D` at **`+024D`**
  with the segment word relocated. `0x0085` is unit `0x77`'s `memory_segment`, so
  this is a fixup-backed call into game unit `0x77` from game unit `0x23`.
* **Near, same unit:** `push cs; call 0x275B` at **`+178A`**, inside the function
  that starts at `+1734`.

Both enclosing functions are `game_evidenced`, and both callees this body composes
(`00BA:0BB8`, `00BA:13AB`) are **credited game residents**. The body's own
behaviour — the capacity comparison over item quantity, count and a 1500
constant — matches the record/list fields the credited residents maintain.

**`+0x102` is rewritten by the resident, not merely zeroed.**
`00BA:0BB8` sets it to 0 at `0C14`, then runs `add es:[di+0x102], ax` at `0C5A`
once per chain node (`ax` is quantity x count, and the `jbe 0C54` path still adds
the quantity when the count is 0). So the fixture's pre-set value never reaches
the compare: it is **0 for a null chain and 48 / 60 for the 16- and 20-node
fixtures** (16x3, 20x3). `0C09` writes `+0x100`, a different field. An earlier
version of this document described six fixtures that varied `+0x102` as
"sign-edge" cases; **that claim is withdrawn** - those fixtures exercise the
resident, not the compare.

## Differential

`tools/overlay/u77_275b_cpu_audit.py` executes the caller's **original bytes**
under Unicorn and substitutes the same credited residents at the two call sites,
so any difference is attributable to the caller alone. It compares all **14 CPU
words** and **every byte of 1 MiB RAM**, with **no flag masking** and **no
instruction substitution inside the body**.

Current result, stated exactly:

```
cases 49 · starts 45/45 (+0 excluded) · outcomes 8/10 (+2 excluded sides)
flag_masks 0 · substitutions 0 · stops: return 49 · both residents called in every case
```

**Correction, 2026-10-21 (found by composing this body under OVL_0023_0239, which calls it).** The C model stored the three
flag bytes `[bp-2]`, `[bp-2]` and `[bp-1]` with the word writer, so each store also wrote the byte above it. The store at
`[bp-1]` therefore wrote **`[bp+0]`, the low byte of the saved BP, as zero**, and `RETF 8` returned with
`BP = (caller BP) & 0xFF00`. Every earlier fixture, the unit test and the native harness entered with `BP = 7F00`, whose
low byte is already zero, so the defect was invisible to all of them: a fixture-value blind spot, not a coverage gap in the
branch accounting. The model now uses a byte store (`wr8`); the differential gained 15 cases whose BP low byte is nonzero
(`7F01`, `7FFE`, `1234`, `FFFF`, `00FF`, each with the type, over-count and saturated variants), and the unit test checks
that BP survives for the same five values. With the old model the differential fails at the first such case
(`bp 7F00 != original 7F01`) and the unit test at its first assertion. The body bytes, hash, identity and credit are
unchanged; the native corroboration below still enters with `BP = 7F00`, so it cannot see this bug class and is not
re-run here.

**Second correction, same day (found by the 0239 boundary cases).** The model's last compare tested only the carry flag
(`CF == 0` meant over capacity), but the original's `JBE` at `27CA` is `CF | ZF`: **equality is not over capacity**. At
`S+P == R+1500` the model returned 1 and the original 0. The branch accounting had never been able to see this: both sides of
`JBE` were taken, but only through `CF`, and no fixed fixture value makes the two operands equal. The differential now
**measures** the operands from the original (a calibration run reads `AX = S+P` and `CX = R+1500` at `27C2`) and adds three cases
from them, one below, equal and one above (3 more cases, 46 to 49; for the all-zero object `R+1500 = 1500`, so `1499`/`1500`
return 0 and `1501` returns 1, asserted against the original's own result). The old model fails the equal case
(`ax 0501 != original 0500`); the unit test carries the same three values. The compare's `JG`/`JL` half and the two excluded
sides are unaffected. Body bytes, hash, identity and credit are unchanged.

**Third correction, 2026-10-21 (load base).** The model entered both resident calls with `CS := 00BAh`, a constant, where the
original's `9A LCALL` carries a segment word the loader relocates (`276A` and `27AB`). At the default base the constant and
the word agree, so no differential can tell them apart; and because the residents' own models never look at their entry CS
(and the audit substitutes them at the call), no differential could observe the difference at another base either. The model
now takes each segment from the live word in the caller's code (`CS:276A`, `CS:27AB`) for the transfer. The catcher is a
**spy test** (`test_overlay_0077_275b_live_cs.c`): the two resident models are replaced by spies that record the CS they
are entered with, and the test sets the words to `00BA`, `1234`/`5678`, `0001`/`FFF0` and `10BA`; it fails on the old
model (`entry_cs[0] == words[k][0]`) and passes on the new one. This is the load-base class. par-e's sweep counted it at 0DE3/0E0E (1 site) and 1010 (3 sites) and here (2 sites); **0918
(1 site, `cs = 0x00BAu`) was found by my census, not par-e's**, whose regex allowed only one leading zero (`0x0?BA`) and so
missed it. The confirmed total is therefore **four models, seven sites** (0DE3, 1010, 0918, 275B); the same census later found
`u7f_roll_call_0fde.c:31` (`cs = 0x00B0`), which makes five models and eight sites. Each other model is a separate fix. Nothing else about the body, its hash or its credit changed.

That is **not** "100% of reachable" and **not** "all outcomes": **no starts** are
excluded (`EXCLUDED_STARTS` is empty) and **two** conditional sides are, for the
contract reason below. Branch
accounting is **successor-based** — when a conditional jump executes, the next IP
*is* the side taken — so no flag interpretation can retire an untaken path.

## Exclusions, verbatim with their evidence

### 1. `0x277B` - **now covered**, previously excluded in error

The earlier version of this document claimed the type-`> 15` path was unreachable
because `00BA:0BB8` zeroes `es:[di+0xC7]`. That conclusion came from fixtures with
a **null item chain**. `0BB8` zeroes `+0xC7` and then walks the list at
`+0xC8`/`+0xCA` (each node's next pointer at `+0x2A`/`+0x2C`), doing
`inc byte es:[di+0xC7]` at `0C2C` **once per node**. With **16 and 20 nodes**
attached the byte passes 15 and the branch executes.

The exclusion was removed and the chain fixtures added; the checker's own
`EXCLUSION NO LONGER TRUE` guard fired when they did, which is how the error was
caught. Coverage is now **45/45 starts**.

### 2. `jg 0x27CC` and `jl 0x27D0` - unreachable **by contract** (closed form)

At `0x27C2`, `cmp dx,bx` with `dx = 0` and `bx =` sign-extension of
`(AX + 1500)`, where `AX` is the `00BA:13AB` result.

**The contract, read off the body.** `13AB` (decoded at `0x13AB`) returns a
value selected from `AL`:

| `AL` | return |
| --- | --- |
| 1..3 | `0xFEA2` = -350 |
| 4..5 | `0xFF06` = -250 |
| 6..7 | `0xFF6A` = -150 |
| `0x0C`..`0x0D` | 100 |
| `0x0E`..`0x0F` | 200 |
| `0x10` | 350 |
| `0x11`..`0x15` | 500 + 250·k, k=0..4 |
| `0x16`..`0x1A` | 2000 + 1000·k, k=0..4 |
| `0x1B` | 7500 |
| `0x1C`..`0x1E` | 9000 + 3000·k, k=0..2 |
| otherwise | 0 |

So the return lies in **[-350, 15000]** and `AX + 1500` lies in
**[1150, 16500]**: bit 15 is never set, `bx = 0`, `cmp 0,0` is equal, and the
32-bit test always proceeds to `cmp ax,cx`. Neither sign branch is reachable.

**State that would make them feasible:** a `13AB` return outside
`[-1500, 31267]`, which the decoding above excludes for every `AL`.

**The exclusions are asserted in code**, not merely documented: the checker fails
with `EXCLUSION NO LONGER TRUE` if any fixture later reaches one of these
starts or sides, so the document cannot drift into a coverage claim.

## Native corroboration (declared stubs)

`tools/overlay/u77_275b_cpu_native.py` runs a DOSBox-X normal/386 capture of the
caller's original bytes. Its declaration, per the five required points:

1. **Stubbed endpoints:** `00BA:0BB8` and `00BA:13AB`, at their entry points,
   substituted with `RETF 4` stubs. The C side is linked against equivalent
   stubs (`tools/overlay/u77_275b_native_stubs.c`) so both sides see the same
   declared endpoint.
2. **What the stub does not do:** it does **not** execute the residents'
   closure, so their internal SYSTEM endpoints are **not exercised by this run**
   — only by the differential.
3. **Division of labour:** the differential (the original 31 fixtures, to which 15 Unicorn-only BP cases were added on 2026-10-21, 14 CPU words, all
   1,048,576 RAM bytes, successor accounting, asserted exclusions) carries the
   **composition** claim. This run corroborates the **caller's 132 bytes**
   against the original instruction stream.
4. **Coverage reduction, stated:** this set is **28 of 31** fixtures. The three
   non-default-`SP` fixtures are excluded **by construction** because the
   capture assembly does not vary `SP`; closing them needs assembly that sets
   `SP` per case. Compared per case: **12 CPU words**, both 0x300-byte object
   blocks, and the arithmetic flags under mask `0x0FD5`.
5. **Frame:** the run uses its **own declared frame** — return IP/CS are the
   harness's (`CS`/`IP` are therefore **declared not compared**, since the body
   never computes them), with `DS = 0x1000`, the fixture's entry flags applied
   by `POPF`, and `SP` at the post-`CALL` value. It is **not asserted** to be
   byte-identical to the differential's fixture frame.

Result: **28 of 31** captured cases agree on the 12 compared words, both 0x300-byte
object blocks and the masked flags; `RESULT.BIN` SHA-256
`fec5fd7778014b68717a6a3a899f14d9d3b81788234149cf2eeed5e7d81ba120`.

**What the native run found that the differential could not:** with the real
resident composed, `00BA:13AB` overwrites `AX`, which hid a missing
`mov ax, es:[di+0x37]` in the model. Under the stub the caller's `AX` survives
to the comparison, exposing it. The model now reproduces that instruction, and
the differential still passes — so the stub run did real work rather than
rubber-stamping.

## Limits

- The differential runs the caller's bytes from a **synthetic entry frame**. No
  natural gameplay route is claimed.
- The two callees are composed as their accepted raw models, not re-derived here.
- Interrupts, device state, self-modifying code, wrapping stack frames and
  arbitrary aliasing are outside the model.
- Native corroboration has landed with **declared stubs** (see above); it
  corroborates the caller only, and the composition claim is the differential's.
- **No credit is claimed by this document** (as authored; accepted since, see *Status at acceptance* below). Identity and strict credit are the
  reviewer's decision, not the author's.

## Status at acceptance (dated note; added 2026-10-20 by par-c, record only)

`OVL_0077_275B` was accepted after this document was written: strict full-C credit and
verified game identity, by reviewed PR #1188 through acceptance PR #1191 (commit
`63b4ae3a`, 2026-10-20 11:31 PDT), moving the checkpoint to 245 strict / 291 identities.
Sentences above that say no credit is claimed, that the row has no strict or
identity row, that the classifier label is provisional, or that rows are proposed
"as text only" describe the author PR at the time it was written and are
superseded by this note. The evidence in this document is unchanged by it. The
authority for the credit is the acceptance commit and the two credit CSVs, not this
note.
