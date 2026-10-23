# Unit 0x6A +0214 record walk

`pr_u6a_record_walk_0214` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x6A:+0214..+025B`. It walks a far-linked record list and
composes two merged models on each record:

- the [`+0159` high-item clearer](U6A_RECORD_CLEAR_HIGH_0159.md);
- the [`+019C` node clearer](U6A_LINK_HIGH_CLEAR_019C.md), which reaches unit
  0x86 public entry 6 `+323D` through the overlay stub `0096:003E`.

Both run on the same RAM and CPU words. The recall attempt for this lane
found nothing to reuse.

## Loaded-state precondition (inherited)

Added 2026-10-22 by par-e (credit authority), VP i21329, ruling i21377. The credit is unchanged. `OVL_006A_0214`'s C
composes `pr_u6a_link_high_clear_019c` (`OVL_006A_019C`), and the outer C now checks its inherited loaded bindings before mutation. This credit retains
the same loaded-state precondition: every stub listed in [U6A_LINK_HIGH_CLEAR_019C.md](U6A_LINK_HIGH_CLEAR_019C.md#loaded-state-precondition)
must hold its loaded far `JMP`. If one is absent or malformed, **the C refuses with `ARGUMENT` before changing CPU14 or RAM**. The original overlay-manager path remains outside the C model. This section covers only the composed model. It makes no claim about stubs this
body calls directly.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x86 `+323D` | 77 bytes, SHA-256 `2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd`, reached by `+019C` |
| Unit 0x6A | GAME file base `0x17831`, stub paragraph `0075`; `+0214` is an internal prologue |
| Function | GAME file `0x17A45..0x17A8C`; **72 bytes, 27 instructions, one conditional branch, two near-call sites** |
| Function SHA-256 | `35ee0bd09567048899c218e1874c5884fa7eaad1c08ccdc2198b2b85eb2fe939` |
| Near calls | `+0236` `CALL +0159` and `+0240` `CALL +019C`, each preceded by `PUSH CS` |
| Exit | `RETF` at `+025B`; exact end `+025C` exclusive |

Decoding every unit-0x6A routine from its own prologue finds two aligned
`PUSH CS; CALL +0214` sites, `+1ECA` and `+1FB3`, both inside `+1E6F`.
`+1E6F` is unit 0x6A's public entry 1. No gameplay meaning is inferred;
"record walk" names the loop.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP and `[SP+2]`
return CS and no arguments; `RETF` leaves SP four bytes above entry. The
routine pushes BP and reserves four bytes, which hold the current far record
pointer: offset `[BP-4]` and segment `[BP-2]`.

The steps are:

1. The pointer starts as `DS:5D96/5D98`.
2. While `[BP-4] OR [BP-2]` is nonzero, the routine pushes the pointer words
   (segment first) and calls `+0159`.
3. It pushes them again and calls `+019C`.
4. `LES DI,[BP-4]` then loads the record's far link `+104h/+106h` as the next
   pointer.

On the null pointer `MOV SP,BP; POP BP; RETF` ends. AX is 0, the final flags
are `OR`'s (CF and OF clear, ZF and PF set, undefined AF represented as zero),
and DX and ES:DI hold the last link load.

Each near call pushes CS and the return offset relative to the entry IP
(`0x0214` in the game). It enters the merged model at `ip − BBh` (`+0159`) or
`ip − 78h` (`+019C`) and requires its `RETF 4` to come back to the frame:

- `TRANSFER` is reported otherwise.
- A callee that rejects its entry reports `CALLEE` with its status.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. `LES` and the RETF frame are read
physically contiguous. `SUB SP,4` writes only flags that the `OR` overwrites.

A cyclic record list never returns in DOS, so the host caps the records
walked; the next record would stop before `+0227` with status `LIMIT`. A
cyclic node list inside `+019C` is that model's own contract and is not capped
here. Null RAM/CPU or RAM that is not exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also ASan/UBSan)
uses synthetic stub metadata only. It covers:

- a three-record list, where every high item byte and each `+2C` byte is
  cleared, the low bytes keep their values, and AX is 0 with `OR`'s flags;
- a list headed at the second record, which leaves the first untouched;
- an empty list;
- the cyclic-list cap;
- the host guards.

The original-byte oracle passes **125 returning cases**, each comparing the
entire 1 MiB RAM and all 14 CPU words with no flag mask. The cases are:

- an empty list and lists of one to five records with random items and node
  lists, plus 80 random worlds that also vary flags, ES and every entry
  register;
- a list headed at a later record;
- records across offset wrap and A20, and nodes across A20;
- four stack offsets near both segment ends, and SS and DS across A20;
- a random background and preserved DF.

Two originals never return. One is a cyclic record list; the other is a
record whose link word aliases the list head. The model stops both with
`LIMIT` under the host cap, and they are not compared. All 27 instruction
starts and both conditional outcomes are reached.

The DOSBox-X normal-core/386 synthetic oracle passes **30 cases** in six
runs. Each run's COM bakes one world: the predicate tables, five records with
item bytes, node lists and links, and the nodes. The COM holds the original
`+0159..+025B` span, the original `+323D` body and a stub whose paragraph the
`+019C` far-call segment word is relocated to by the loader's arithmetic.
Each case restores the record and node areas from pristine copies and enters
with interrupts disabled. The oracle compares all 14 CPU words, the
128-byte frame window, and the record and node areas with no mask. The
combined result SHA-256 is
`4066d6355c008bde9f845a31e71e2a0aab566e074a5bd4bab02f55677f895e12`.

Original bytes and generated images stay under ignored `resources/`.

```sh
cmake -S . -B resources/build-0214 -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-0214 --target poolrad_overlay_u6a_record_walk_0214_tests
ctest --test-dir resources/build-0214 -R record_walk_0214 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/u6a_record_walk_0214_audit.py --unicorn --native --output resources/u6a-0214-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x6A` `+0214..+025B` with `+0159` and `+019C` (and u86 `+323D`) composed |
| `source_candidates` | `src/recovered/overlay_u6a_record_walk_0214.c` |
| `signals` | Exact hash/CFG, two aligned callers in public entry 1, whole-RAM/CPU differential with full coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Cyclic record lists never return (host LIMIT cap); cyclic node lists follow `+019C`'s own contract; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_006A_0214 --require-current-main
```

Updated refusal/parity validation: [guard work report](../LOADED_STUB_GUARDS.md); measurements pending.
