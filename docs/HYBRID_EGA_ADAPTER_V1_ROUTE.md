# Hybrid executor: the EGA/bus adapter, built and checked on the hottest body (VP i20637)

**Result.** `RES_0709_01DF`, the glyph routine, is the body that does 824 of the 976 credited arrivals on the cold-start
path. Its C now draws the game's text on the real VGA inside the running game.

- **V1, stream equivalence in shadow form.** On **817 of 824** live arrivals, our C's ordered device stream equals the
  original's element by element: every VRAM byte read with its value, every VRAM byte write, every OUT. Its registers and
  all 1 MiB of RAM equal the original's too. The other 7 arrivals had an interrupt inside the original's window and were
  excluded. There were 0 differences.
- **ROUTE.** In a second cold start the C drives the real VGA itself on **all 824 arrivals**, with 0 stops. The main-menu
  screen at 55 s has **0 differing pixels** against the unsubstituted run, and the 270 per-ID arrival counts are identical.

Both comparisons are shown able to fail by one mutant, an OUT at `03AD` sending `AX ^ 1`:

- in V1 it gives **814 `V1_STREAM_DIFF`**, each naming the first divergent event (`event 0: C O 003C4=3/1, original O
  003C4=2/1`);
- in route mode it gives **7,896 differing pixels** on the same screen.

The adapter, scoped in [HYBRID_EGA_ADAPTER_SCOPE.md](HYBRID_EGA_ADAPTER_SCOPE.md), is built. It is checked on one of the
five `pr_bus` bodies the cold start reaches.

## What was built

The cumulative patch is [`tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`](../tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch)
against pinned DOSBox-X `5a9c7c53`. It reverse-applies to the tree that ran.

- **The recorder** (`src/cpu/pr_hybrid_record.h`, plus a hook pointer in `src/hardware/iohandler.cpp`).
  - While on, it records the CPU's every access to `A0000h..AFFFFh` as byte events, `R` with the value read and `W` with
    the value written, plus every port access (`O`/`I` with size).
  - The hook point is the core's `LoadMb..SaveMd` macros, which string instructions use too
    (`core_normal/string.h`). So the stream is complete whichever VGA page handler serves the access. The fast planar
    handler bypasses the generic one (`vga_memory.cpp:636-660`), so hooking inside the handlers would have missed writes.
  - Word and dword window accesses are issued and recorded as bytes, low first. That is exactly how the planar handlers
    split them (`vga_memory.cpp:594-608`), so device behaviour is unchanged.
- **The replay bus (V1).** The hybrid build defines its own `pr_bus_read8`, `pr_bus_write8` and `pr_bus_out` in place of
  `memory_bus.c`, which is not linked.
  - Outside the window, RAM is guest RAM (`MemBase`).
  - Inside the window, and for ports, every access is answered from, and checked against, the original's recorded stream.
  - The first mismatch is kept with both events. A C that issues fewer events than the original is also a mismatch.
- **The routed bus (route mode, `PR_HYBRID_BUS_MODE=route`).** Window reads and writes go to DOSBox-X's own
  `mem_readb_inline`/`mem_writeb_inline` (the current page handler, real latches). OUTs go to `IO_WriteB`/`IO_WriteW`.
  `pr_ega` is never used; this is "route, don't translate" from the scope.
- **The dispatch (V1, shadow form).** At each arrival the original runs first with the recorder on. The snapshot is then
  restored, the C runs on the replay bus, the stream and the state are compared, and the game continues from the C's
  registers and RAM.
  - The device keeps the original's effects. Equal streams from the same start state make them the C's.
  - If the C stops before returning, the device already holds the original's full effects, so a mid-body hand-back would
    apply them twice. The original's result is kept instead.
- **The dispatch (route mode).** At the arrival, before the entry instruction executes, the C runs on the routed bus and the
  game continues from its state. A stop would hand back mid-body: the device holds exactly the C's accesses up to that
  point. There were 0 stops in this run.

## Checks that pin each claim

| Claim | Check | Run |
|---|---|---|
| The C's device stream equals the original's | V1 replay: element-by-element, including read values and order | `hybrid-v1-glyph-a`: 817 `V1_STREAM_AND_STATE_EQUAL`, 0 diff, 7 interrupted |
| …and the compare can fail | glyph mutant (OUT `03AD` sends `AX ^ 1`) | `hybrid-v1-glyph-mutant`: 814 `V1_STREAM_DIFF`, 1 equal (an arrival that does not reach that OUT), 9 interrupted |
| The C's registers and RAM equal the original's | the same V1 compare: 32-bit registers, segments, flags, 1 MiB | the same run |
| The C draws the screen correctly on the real device | route mode; pixel compare of the 55 s menu against the unsubstituted run `hybrid-stage0-b` | `hybrid-route-glyph-a`: 824 routed returns, 0 stops, **0 differing pixels** |
| …and the pixel compare can fail | the same mutant in route mode | `hybrid-route-glyph-mutant`: **7,896 differing pixels** |
| The rest of the game is undisturbed | per-ID observer counts, all 270, against the unsubstituted run | both runs: 0 IDs differ |
| The mutant builds left nothing behind | the binary's sha256 prefix after restoring the correct object | `fab220cf6005abd8` (V1 build), `db6f24103220740d` (route build): equal before and after |

In the same runs, `RES_0899_0104` (46 equal, 5 hand-backs) and `RES_0464_2595` (1 mid-body hand-back, 1 full; both equal)
held unchanged.

## What this does not show

- **Four of the five reached `pr_bus` bodies are not done:** `RES_0709_05EB`, `RES_0709_0626`, `RES_03F1_0137` and
  `OVL_03B0_01DA`. Each needs its own dispatch glue: confirmed symbol, CPU copy, budgets, and (for 0626) its composed
  callees.
- **The pixel check covers one screen.** The 55 s menu is a single frame. Glyphs drawn earlier, on the loading, credits and
  code-word screens, and then overwritten are covered only by V1's per-arrival stream equality, not by pixels.
- **Route mode has no per-arrival check.** Its arrival-by-arrival evidence is V1, on the same body and the same path, in a
  separate run. The route run itself is checked by counts and pixels only.
- **The full-EGA-state comparison (V2, the 0294 standard) is not built.** V1 compares the device's input stream, not its
  end state. The argument that equal streams give equal state rests on DOSBox-X's device being deterministic.
- **Timing.** The C consumes no emulated cycles. VGA access delays are charged from inside the C call in route mode. No
  effect was visible in counts or pixels.
- **The raw-RAM VGA-window guard is implicit, not separate.** For the RAM-only bodies (0104, 2595), any C write to
  `MemBase[A0000h..]` would show as a full-state difference against the original, which never writes `MemBase` there. A
  standalone guard for a raw body in route mode is not built, because no raw body runs in route mode yet.

## Build and run (local; the emulator stays under ignored `resources/`)

1. Apply the patch to pinned DOSBox-X `5a9c7c53` (the heavy-debug configuration).
2. Compile with `gcc -std=c11 -O2 -I include -c`:
   - `resident_0899_idle_0104_cpu.c`
   - `resident_0464_bitmap_release_2595_cpu.c`
   - `system_freemem_0254_cpu.c`
   - `resident_0709_glyph_01df_cpu.c`
   - `system_runtime_leaves_cpu.c`

   Combine them with `ld -r` into `resources/hybrid/pr_models.o`. Do not link `memory_bus.c` or `ega_hw.c`: the patch
   defines `pr_bus_*`.
3. Add `-I<repo>/include` to `CPPFLAGS` in the local `src/cpu/Makefile`, and add the object to `LIBS` in `src/Makefile`.
4. Run `tools/hybrid/run_observer.sh <name> resources/hybrid/registry.tsv` with
   `PR_HYBRID_DISPATCH=RES_0899_0104,RES_0464_2595,RES_0709_01DF` set. Add `PR_HYBRID_BUS_MODE=route` for route mode.
