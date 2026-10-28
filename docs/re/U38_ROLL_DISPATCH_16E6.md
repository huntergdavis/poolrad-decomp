# U38 entry 57: bounded roll/dispatch wrapper

`OVL_0038_16E6` is a new **bounded translation**, not a composed function or a
strict-credit claim. The source is `overlay_u38_roll_dispatch_16e6.c`; the checker
is `check_overlay_u38_roll_dispatch_16e6.py`, declaring this exact CHECKER_ROWS
identity. Accounting and credit tables are intentionally unchanged.

## Original identity and boundaries

Hash-pinned START.EXE (`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`)
and GAME.OVR (`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`)
identify public entry 57 of unit 0038, `[16E6,170B)`: 37 bytes, 18 instructions,
file offset 66369. Body SHA-256:
`26319a64c3724af34e74edb22250387fe28a05e17d3be802e26ef2db1f2bbcee`.
The only relocation in the extent is the segment operand at 16F8.
U9E dispatcher slot 69 names this handler in `OVL_009E_0000_HANDLERS.md`.
That static slot identity does not establish live arrival.

The wrapper saves BP and forwards two argument words, preserving AH while
passing roll count 1 and sides 9. The far call at 16F5 goes through the relocated
`:0048` stub for U9E+0DE3. On an externally supplied return at 16FA, it clears AH,
adds 10 to AX, pushes the result, replaces AL with FE while preserving the new
AH, and builds a far frame for U38+15EB via PUSH CS / near CALL at 1704.
U38+15EB remains untranslated. On an externally supplied return at 1707, the
wrapper restores BP and executes RETF 0Ah.

## Enforced scope

`pr_u38_roll_dispatch_16e6` requires an explicit ENTER, AFTER_ROLL, or
AFTER_DISPATCH phase and the wrapper's loaded CS. It checks the corresponding
CS:IP before any mutation. Both calls return a distinct stop status with the
call-entry frame intact. No callback runs, no callee return is synthesized,
and attempting to resume at either callee address is rejected unchanged.
Continuation phases are conditional contracts supplied by the caller; they do
not demonstrate how the missing callee returns or what effects it has.

The CPU14/flat-RAM interface preserves non-arithmetic flags, reads relocated
segments from RAM, wraps effective offsets at 16 bits and physical addresses
at 1 MiB, and treats the second byte of a word as physically contiguous.
Instruction bytes are immutable except the segment relocation; stack/data
must not overwrite code, and CPU/RAM storage must not overlap.

## Evidence procedure

Run through the shared heavy wrapper:

```
python3 tests/recovered/check_overlay_u38_roll_dispatch_16e6.py --mutants --output resources/t6-u38-16e6/check.json
```

The checker pins media/body hashes, entry identity, next function boundary,
fixup, decoded instruction count, call sites, and return. It rejects one-byte
truncation, one-instruction extension, and a changed call operand. It compares
compiled C with original-byte Unicorn execution independently over each phase,
stopping **before** either callee executes. Every comparison covers all 14 CPU
words and all 1 MiB RAM, without masks. Cases cover all 256 incoming roll-result
bytes, dirty AH/flags, relocated wrapper and roll segments, stack-offset wrap,
and independent return frames. Physical 1 MiB word wrap has a separately labelled
synthetic contract check; it is not counted as an original-byte Unicorn case.
Argument rejection and resumption at stopped callees must preserve all state.
Compiled negative mutants target forwarding, roll parameters, stack writes,
AH clearing, bias, flags, dispatch argument/frame, forbidden continuation past
15EB, resume validation, and return cleanup. The receipt names the assertion
that kills each mutant and the sites reached.

CPU layout, media pins, and address helper utilities are reused from
`check_overlay_u38_target_select_101d.py`; the adjacent `overlay_u38_roll_dispatch_1699.c`
provided stack-frame implementation context. No existing callee fixture is used
as evidence of real callee behavior. Fresh queue evidence on base
`dadfec5806de11c14e41f28cd9b4ad69a6090c59` lists U9E+0DE3 strict and U38+15EB
untranslated. The latter prevents closure.

**Measurement pending:** no passing C comparison, native execution, composed
callee execution, live dispatcher arrival, or credit is asserted by this draft.
