# Hybrid executor: the first credited body executed by our C inside the game (VP i20596)

**Result.** During one cold start of the audited v1.3 game to the main menu, `RES_0899_0104` (resident `0899:0104`, the
30-byte frame-and-return reached from the credits wait loop) arrived **51** times. Our C, `pr_r0899_idle_0104_cpu`,
**executed in place of the original bytes 46 times, and the game continued from the C's result each time.** The other 5
were deliberate hand-back tests.

**On every one of the 46 dispatches, the C's result equalled the original's:** all registers (32-bit), the segments, the
flags, and all 1 MiB of RAM. The original was run from the same entry state in the same run. The 5 hand-backs also
behaved correctly, with 0 interrupted windows and 0 timeouts.

The rest of the run was unchanged:

- the observer's 270 per-ID arrival counts equal those of the unsubstituted run `hybrid-stage0-b`
  (0 IDs differ; 976 arrivals over the same 12 IDs);
- the 55-second screenshot shows the same main menu.

This is the stage the VP asked for before the EGA adapter. It exercises three things that have nothing to do with the EGA,
on a RAM-only body:

- the hook dispatch;
- the register copy-in and copy-out;
- the hand-back at a stop.

It does not claim anything about the EGA bodies.

## How each arrival is handled

This is the in-run check, `src/cpu/pr_hybrid_dispatch.h` in
[`tools/hybrid/dosbox-x-5a9c7c53-dispatch-0104.patch`](../tools/hybrid/dosbox-x-5a9c7c53-dispatch-0104.patch).

1. **The arrival.** The stage 0 observer matches the body's signature at `CS:0104`. The dispatch snapshots **S**: all
   32-bit registers, the six segments, the flags, and guest RAM `0..1 MiB` (`MemBase`).
2. **The original runs** from S to its own return: CS:IP equal to the far return address on the stack, and SP equal to the
   entry SP + 4. Its state is **O**.
   - If any instruction executes outside `0899:[0104,0122)` before that return (an interrupt), the arrival is
     `INTERRUPTED`, O stands, and nothing is dispatched. Measured: 0.
   - A 10,000-instruction budget bounds the window. Measured: 0 timeouts.
3. **S is restored, and our C runs on the live guest:**
   - the 16-bit registers are copied into the model's 14-word CPU;
   - guest RAM is passed directly as the model's 1 MiB `ram`;
   - the result is copied back into DOSBox-X's registers, keeping the 32-bit high halves from S.

   The game continues from the C's state.
4. **The C's state is compared with O:** every register, the segments, the flags, and every byte of 1 MiB.
   - Result: `C_EQUAL_ORIGINAL` 46 times, `C_DIFFERS` 0 times.

**The hand-back test, every 10th arrival (5 times).** Instead of step 3:

- The model is called with a wrong RAM size, the stop this RAM-only body can produce (its only non-OK status is ARGUMENT).
  It must change nothing, checked against S.
- Control then goes back to the original bytes, which run again from S.
- That second original result must equal the first.
- Result: `HANDBACK_EQUAL` 5 times, `HANDBACK_*` failures 0.

This exercises the path every non-RETURNED status takes, and it shows the snapshot restore is exact. The original from a
restored S reproduced O byte for byte.

## Checks that pin the result

| Claim | Check |
|---|---|
| Our C executed in place of the original, 46 times | the per-arrival log `<run>/observer.dispatch`: 46 `C_EQUAL_ORIGINAL` lines, each written after `put(C)` set DOSBox-X's registers from the model's output and the game continued from them |
| C equals the original, in full state | step 4's compare: 32-bit registers, segments, flags, all 1 MiB; 0 differences |
| **The compare can see a wrong C** | **mutant run** `hybrid-dispatch-0104-mutant`: the model rebuilt without its parity-flag update. Result: 15 `C_DIFFERS` (for example `flags 00007202 vs 00007206`, PF missing) and 31 equal, where the result's PF is 0 anyway. Afterwards the correct model object was restored and the binary relinked; its sha256 prefix `9eb2ae1afaa0ab67` equals the one recorded before the mutation |
| The hand-back path is exact | 5 `HANDBACK_EQUAL`: ARGUMENT changed nothing, and the rerun original equalled the first original |
| The rest of the game is undisturbed | per-ID arrival counts equal the unsubstituted run for all 270 IDs; the same main menu at 55 s; 0 emulators left |

## What this does not show

- **Nothing about the EGA bodies.** 5 of the 12 reached bodies need the adapter
  ([HYBRID_EGA_ADAPTER_SCOPE.md](HYBRID_EGA_ADAPTER_SCOPE.md)).
- **Nothing about any other RAM-only body.** Each needs its own confirmed symbol, its own CPU copy, and its own stop
  semantics. The dispatch file knows exactly one body.
- **Nothing about timing.** The C consumed no emulated cycles. For this 7-instruction body the effect was not visible in the
  counts or the screen.
- **A natural stop.** The hand-back used ARGUMENT, because this body has no callee stop. A body with real CALLEE stops will
  test the hand-back mid-body.

## Build and run (local; the emulator stays under ignored `resources/`)

1. **Apply the patch** to pinned DOSBox-X `5a9c7c53`, the heavy-debug configuration.
2. **Compile the model:**
   `gcc -std=c11 -O2 -I include -c src/recovered/resident_0899_idle_0104_cpu.c -o resources/hybrid/pr_r0899_0104.o`.
3. **Edit the local, ignored Makefiles:** append `-I<repo>/include` to `CPPFLAGS` in `src/cpu/Makefile`, and append the
   object to `LIBS` in `src/Makefile`.
4. **Run:** with `PR_HYBRID_DISPATCH=RES_0899_0104` set, run
   `tools/hybrid/run_observer.sh <name> resources/hybrid/registry.tsv`.

`/usr/bin/dosbox-x` and the desk's deployed build were not touched.
