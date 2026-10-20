# U77 `+275B`: canvas capacity check over two credited residents

**Body.** GAME.OVR unit `0x77`, `[275B,27DF)`, **132 bytes / 45 instruction
starts**, terminating `RETF 8`. The extent ends at `27DF` because `RETF 8` at
`27DC` occupies three bytes; a first-`RETF`-exclusive read reports 130 and is
wrong.

**What it does.** Three frame words: a far pointer at `[bp+0x0A]`, a word at
`[bp+0x0C]`, and a far pointer at `[bp+0x06]`. It reads the second pointer's
object fields `+0xC7` (type byte, compared against 15), `+0x37` (quantity word)
and `+0x39` (count byte), multiplies quantity by count when the count is
non-zero, adds the composed `00BA:0BB8` result plus **1500**, and compares that
sign-extended 32-bit total with the first pointer's `+0x102` word plus the
product. `AL` returns the flag; it is also stored at `[bp-1]`.

**Composition.** Both far calls compose credited raw residents — `00BA:0BB8`
through `pr_res00ba_0bb8_cpu` and `00BA:13AB` through `pr_res00ba_helpers_cpu`
with entry `13AB`. No callback stands in for original code.

## Differential

`tools/overlay/u77_275b_cpu_audit.py` executes the caller's **original bytes**
under Unicorn and substitutes the same credited residents at the two call sites,
so any difference is attributable to the caller alone. It compares all **14 CPU
words** and **every byte of 1 MiB RAM**, with **no flag masking** and **no
instruction substitution inside the body**.

Current result, stated exactly:

```
cases 29 · starts 44/44 (+1 excluded) · outcomes 7/10 (+3 excluded sides)
flag_masks 0 · substitutions 0 · stops: return 29 · both residents called in every case
```

That is **not** "100% of reachable" and **not** "all outcomes": one start and
three conditional sides are excluded, each for a stated reason below. Branch
accounting is **successor-based** — when a conditional jump executes, the next IP
*is* the side taken — so no flag interpretation can retire an untaken path.

## Exclusions, verbatim with their evidence

### 1. `0x277B` — unreachable **by composition**

The instruction is `mov byte [bp-2],1` on the type-`> 15` path after
`cmp byte es:[di+0xC7],0xF` / `jbe` at `0x2773`/`0x2779`.

**Argument.** The type byte lives in the same object the body hands to
`00BA:0BB8`, and **`0BB8` clears `+0xC7` before the comparison**. Measured across
pre-set values `00, 01, 0E, 0F, 10, 11, 20, 7F, 80, FF`: the post-call byte reads
**0** in every case, so the `JBE` is always taken.

**State that would make it feasible:** a `00BA:0BB8` state that does **not**
clear `+0xC7`. If such a state is found, this exclusion becomes false and the
document must be updated.

### 2. `jg 0x27CC` and `jl 0x27D0` — unreachable **by contract**

At `0x27C2`, `cmp dx,bx` with `dx = 0` and `bx =` sign-extension of
`(AX + 1500)`, where `AX` is the `00BA:13AB` result.

**Argument.** `13AB` returns **`0xFFFE`** in every measured state, so
`0xFFFE + 0x05DC` wraps to **`0x05DA`**, `bx = 0x0000`, and `cmp 0,0` is equal:
neither `jg` nor `jl` can be taken, and the 32-bit test always proceeds to
`cmp ax,cx`. **120 input combinations were searched** for a `13AB` state
returning in `[0x7A24,0xFFFF]` and none was found.

**State that would make them feasible:** a `13AB` return in `[0x7A24,0xFFFF]`,
i.e. a value whose sum with 1500 keeps bit 15 set.

**The exclusions are asserted in code**, not merely documented: the checker fails
with `EXCLUSION NO LONGER TRUE` if any fixture later reaches one of these
starts or sides, so the document cannot drift into a coverage claim.

## Limits

- The differential runs the caller's bytes from a **synthetic entry frame**. No
  natural gameplay route is claimed.
- The two callees are composed as their accepted raw models, not re-derived here.
- Interrupts, device state, self-modifying code, wrapping stack frames and
  arbitrary aliasing are outside the model.
- Native corroboration and its synthetic-frame declaration are recorded
  separately when the native harness lands; until then this document's evidence
  is the Unicorn differential only.
- **No credit is claimed by this document.** Identity and strict credit are the
  reviewer's decision, not the author's.
