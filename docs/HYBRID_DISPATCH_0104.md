# Hybrid executor: the first credited bodies executed by our C inside the game (VP i20596, i20618)

**This is the first time in the whole port that a line of our code has run in place of the game's own, and the game did not notice.**

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
[`tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`](../tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch).

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

## What this does not show (the limits, verbatim, as VP i20618 asked)

1. **Nothing about the EGA bodies.**
2. **Nothing about other RAM-only bodies.** Each needs its own confirmed symbol and CPU copy.
3. **Nothing about timing.** The C consumed 0 emulated cycles, with no visible effect for a 7-instruction body.
4. **The hand-back used ARGUMENT, because 0104 has no callee stop.** A body with real CALLEE stops will test a mid-body
   hand-back.

Limit 4 is now tested; see the next section.

## The mid-body hand-back: `RES_0464_2595` (VP i20618, next item 1)

`RES_0464_2595` (resident `0464:[2595,2632)`, a bitmap-block release) is RAM-only. It is reached twice at cold start, and
it composes SYSTEM FreeMem. Its model, `pr_r0464_bitmap_release_2595_cpu`, stops **CALLEE** with exact partial state when
FreeMem stops. FreeMem stops LIMIT at `0AF8:040F`, before the next free-list walk iteration, once its declared
`max_heap_steps` budget runs out (`include/poolrad/recovered/system_freemem_0254_cpu.h`).

The same cold start, with both bodies dispatched (run `hybrid-dispatch-2595-a`):

| Arrival | What happened | Result |
|---|---|---|
| 1 | Our C was run with `max_heap_steps = 0` and **stopped inside the callee, at `131C:040F`** (SYSTEM, live), after executing 2595's prefix and FreeMem's entry. Its partial state was written to the guest, and **the original bytes continued from it**: the rest of FreeMem, the rest of 2595's release calls, and its `RETF 4`. | at 2595's return, the full state (32-bit registers, segments, flags, 1 MiB) equals the all-original run from the same snapshot: **`MIDBODY_HANDBACK_EQUAL`** |
| 2 | Our C was run with the full budget and returned. | **`C_EQUAL_ORIGINAL`** |

In the same run, `RES_0899_0104` again gave 46 `C_EQUAL_ORIGINAL` and 5 `ARGUMENT_RERUN_EQUAL`, with 0 interrupted and
0 timeouts for both bodies. The observer's per-ID counts again equal the unsubstituted run for all 270 IDs.

**The compare sees a wrong partial state.** In the mutant, the 2595 model writes its local `[BP-2]` as `AX ^ 1` (run
`hybrid-dispatch-2595-mutant`). That write happens in the C's prefix, before the stop, and the original reads it after the
hand-back.

- Arrival 1 gives `MIDBODY_HANDBACK_DIFF` (`ram 20ED2 18 vs 17`).
- Arrival 2 gives `C_DIFFERS` (`ram 21006 18 vs 17`).
- These are the first differing bytes; the log does not claim they are the mutated byte.
- Afterwards the correct models object was restored and relinked, and the binary's sha256 prefix `5d3f9a713730cc07` equals
  the one recorded before the mutation.

**What this adds.** The design's central property now holds in the live game for a real stop: a C stop inside a callee
leaves exact state that the original can finish. The evidence is one arrival. It is a single data point, and other stop
kinds (CODE, TRANSFER) are not exercised.

## Build and run (local; the emulator stays under ignored `resources/`)

1. **Apply the patch** to pinned DOSBox-X `5a9c7c53`, the heavy-debug configuration.
2. **Compile the models:** `resident_0899_idle_0104_cpu.c`, `resident_0464_bitmap_release_2595_cpu.c` and
   `system_freemem_0254_cpu.c`, each with `gcc -std=c11 -O2 -I include -c`. Combine them with `ld -r` into
   `resources/hybrid/pr_models.o`.
3. **Edit the local, ignored Makefiles:** append `-I<repo>/include` to `CPPFLAGS` in `src/cpu/Makefile`, and append the
   object to `LIBS` in `src/Makefile`.
4. **Run:** with `PR_HYBRID_DISPATCH=RES_0899_0104,RES_0464_2595` set, run
   `tools/hybrid/run_observer.sh <name> resources/hybrid/registry.tsv`.

`/usr/bin/dosbox-x` and the desk's deployed build were not touched.
