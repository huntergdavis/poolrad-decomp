# Unit 0x38 public entry 107: effect scale or clear at +0x2D88

## Provenance and route

`deja "poolrad U38 +2D88 entry107 +101D strict C parity"` returned no
prior-session match. This work reuses the checked
[+0x101D target selector](U38_TARGET_SELECT_101D.md), its
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md), and the
[effect-table installer](U38_EFFECTS_INSTALL_31A2.md). The original
bytes and entry boundaries were independently verified against these
ignored local inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is unit `0x38:[+0x2D88,+0x2DE4)`, `GAME.OVR`
file `[0x119E3,0x11A3F)`: 92 bytes / 32 contiguous instructions,
SHA-256 `cf1d8b0d9a97cc7958605493f70ebdf99288eb78ab49496dbc2ab37ee61290cd`.
The next public entry starts at +0x2DE4. There are no wrapper fixups.
Its only call is `+0x2D97 -> +0x101D`; that helper has the verified
loaded-segment resident call at `:2E8B`.

The +0x31A2 installer stores entry-107 stub `0041:0237` at `DS:6A14`,
effect-table item 123. The four instruction sites are +0x37CA,
+0x37CD, +0x37D0, and +0x37D3; the segment operand has loader fixup
+0x37CE. The U9E dispatcher uses this table, but no native frame
through item 123 into this entry has been reached. Game identity and
strict count remain provisional.

## Exact behavior and boundary

The wrapper passes the active character far pointer from
`DS:5D92/5D94` to +0x101D. A null target clears `DS:6818`.
For a nonnull target, byte `+0x32` nonzero preserves `DS:6818`.
When byte `+0x32` is zero and byte `+0x31` is `0xB1`, it halves the
unsigned effect byte with `SHR AX,1`. Otherwise it clears `DS:6818`.
The body repeats the byte-32 comparison on the preserve or clear path;
the C model repeats it too so the returned FLAGS match. It exits with
`RETF 10` after five caller words.

`pr_u38_effect_scale_clear_2d88` models the complete wrapper in a 1 MiB
RAM and 14-field CPU state. It composes the merged +0x101D C model and
retains its original near-call frame. The only unsupported external
operation is the helper's resident far call to loaded-segment `:2E8B`;
its callback owns the resident register, flag, and memory effects.
Callback refusal returns `PR_U38_2D88_CALL_ABORT` with that call frame
still visible.

## Verification and limits

The original-byte Unicorn checker executes both complete bodies and
their actual near/far calls. Across 120 cases it compares the entire
1 MiB RAM, all 14 CPU fields, and each resident callback frame. Cases
vary helper selection, null/selected/fallback target, byte-31/32 values,
odd/even amount, loaded far-call segment, DS/SS alias, and stack wrap.

Four DOSBox-X normal-core synthetic far-entry frames execute the pinned
wrapper and helper with a deterministic resident stub. They compare all
14 CPU words, 52 stack bytes, and `DS:6818` against the C model. AF
after `SHR AX,1` is undefined and is masked only on the halving frame;
all other FLAGS bits are checked. The native capture is under ignored
`resources/u38-2d88-parity/native/`, SHA-256
`cfb62f1e0fba5782a9ac04c93fe8a6cb790d7777e57e800001ec172dee93bdd4`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan tests cover
null, preserve, halve, clear, and callback refusal.

These checks establish bounded behavior with a synthetic resident stub.
No natural game caller or item-123 dispatch has been measured. No strict
identity or function-count credit is claimed.

## Pinned flag split after #1433 (2026-10-22, no claim moves)

After #1433 the composed model sets AF after a non-zero shift per the pinned rule; raw Unicorn leaves it clear, so `tests/recovered/check_overlay_u38_effect_scale_clear_2d88.py` failed on main (`flags 0x213` vs `0x203`, AF).

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py`: it tracks the last FLAGS-writing instruction the original executed (decoded from the original bytes at run time) and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` the six arithmetic bits must equal the pinned `instructions.h:655-673/743-761` values, and after a shift with a non-zero count AF must be **set** (pinned `get_AF` = `count & 1Fh`). A model that reverted to AF=0 fails. Nothing is masked; the audit prints how many stops used the split.

Run (PR #1446): **120** composed full-RAM/14-CPU cases; the split was used at **42** stops, all after a shift.

**Per-credit sentence (OVL_0038_2D88): evidence did not depend on AF after a shift**; those 42 stops hold C to AF=1 and every other stop is exact. Credit stands.

Its check now also calls the helper's `finish()` after emulation (2026-10-23), so a word divide that is the very last instruction before a stop has its pinned bits settled; re-run: 120 cases, 42 split stops, unchanged.
