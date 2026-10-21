# Hybrid executor: scope (VP i20542)

**Status:** desk scope, written 2026-10-21 by par-b. No code was written for it. Every fact below names its source, either
a file in this tree or a measurement given with its command. Where something is not known, it says UNKNOWN.

**The question.** The checkpoint credits 270 strict bodies (233 overlay, 37 resident) and 315 identities
(`docs/re/function_accounting_checkpoint.json`). Each is proven against the original bytes. Nothing runs that C in place
of the original inside the game.

**The short answer.**

- **The first stage looks reachable.** Run the real game under a patched DOSBox-X that calls our C at credited entry
  addresses, with counters that prove it, through to the title. Most of that machinery has precedent in this project's own
  DOSBox-X build.
- **It is not a week for most of the 270.** Three gaps drive the cost:
  - the contracts are per-model, not shared (37 call shapes, 237 private CPU structs);
  - the first bodies the game reaches bring their own EGA, which must be rerouted;
  - overlays load at segments only known at run time.
- **My estimate:** stage 0 plus a narrow stage (a) in one lane-week; stage (b) two to four more; stage (c) is UNKNOWN in
  size, with a floor of two.
- **Not a full-game claim.** This would not be a C game that runs. It would be the original game with some of its functions
  provably executed by our C.

## 1. The mechanism

Three ways to call our C instead of the guest bytes:

**A. A dispatch check in the emulator's instruction loop (recommended).**

- **The hook point.** The project's pinned DOSBox-X build (source `5a9c7c53`) already calls a check before every
  instruction: `DEBUG_HeavyIsBreakpoint()`, at `src/cpu/core_normal.cpp:244`, in the build with `C_HEAVY_DEBUG 1`
  (`config.h:109`).
- **Precedent for a CS:IP test.** The local modified copy at
  `resources/dosbox-x-heavy-src` (ignored, in a sibling checkout) already tests a specific guest CS:IP at the same
  place (`0F2D:0DD1`, `core_normal.cpp:250`) to arm a memory watch.
- **What a hybrid adds:**
  1. When CS:IP is a hooked credited entry, copy the 14 registers out.
  2. Call the model with guest RAM. RAM is `MemBase`, the flat host pointer at `src/hardware/memory.cpp:261`.
  3. Copy the registers back.
  4. Continue at the returned CS:IP.
- **What it does to what we have proven:**
  - **"Nothing patched" holds.** Guest bytes are never modified; the original stays in memory and can always run.
  - **The relocation model is untouched.** The game's own packed loader still relocates the image
    (`docs/re/RESIDENT_LOADER_MAP.md:19-33`).
  - **The Unicorn differential harness is untouched**, because it never involves DOSBox-X.
  - **Timing changes.** A C call consumes no emulated cycles. Anything the game times by counting instructions changes
    speed. The CRT Delay busy loop is the known case (`docs/re/RES_0709_PRINTER_0768_CPU.md`, CRT Delay section).

**B. Patching the guest image at load.** Write DOSBox's callback opcode, or a far jump, over each credited entry. This
breaks "nothing patched" by construction:

- **Overlays.** Overlay code is read from GAME.OVR into heap paragraphs at each load
  (`docs/re/RESIDENT_LOADER_READ_BRANCH_02A1.md:44-51`), so every load would need re-patching. The eviction policy is NOT
  FOUND.
- **Stubs.** The stubs are themselves rewritten at run time, from `CD 3F` to `EA`
  (`docs/re/U9E_ENTRY8_LIVE_STUB_CAPTURE_CONTRACT.md:35-43`).
- **Code read as data.** At least one code segment holds data that is read (`0AF8:1245`,
  `docs/re/U68_ROLL_GATE_0294_CPU.md:57-59`).

Rejected: it adds a correctness surface without buying anything over A.

**C. A trampoline in Unicorn over a whole-game run.** No harness in this tree runs the whole game under Unicorn: all 227
Unicorn harnesses run one routine from a constructed state (`tools/overlay/*_audit.py`). There is no DOS, BIOS or EGA
environment for a full boot. Rejected: building that environment is the larger project.

**Addressing (needed by A whatever else is chosen):**

- **Resident bodies.** `RES_ssss_oooo` is relative to the expanded image (`docs/re/RESIDENT_LOADER_MAP.md:37-38`). The live
  segment is the image base plus `ssss`. The base is set by the packed loader. One run showed a shift of `+0x824`, and the
  docs warn it is not fixed across runs (`docs/re/U9B_RUNTIME_DISPATCH_0000.md:28`). The hybrid must learn the base at run
  time: the live MZ entry CS minus `0F72` (`docs/re/NATIVE_COLD_START_U17_PICTURE_ROUTE.md:33-35`).
- **Overlay bodies.** `OVL_uuuu_oooo` is a unit and an offset in its code (`docs/re/FUNCTION_CENSUS_QUEUE.md:11-13`). Units
  load at varying segments: u9B at `2CCD`, U17 at `3909`, U19 at `38D3`. Two units can be resident at once
  (`docs/re/U9B_RUNTIME_DISPATCH_0000.md:26-50`). The hybrid must update its hook table on every load. Two places to
  observe this:
  - the overlay loader's read, where the target segment is `ES:[0010]` (`docs/re/RESIDENT_LOADER_READ_BRANCH_02A1.md:44-51`);
  - the stub rewrite to `EA seg:off` (`docs/re/U9E_ENTRY8_LIVE_STUB_CAPTURE_CONTRACT.md:35-43`).
- **Unload and eviction are NOT FOUND.** A stale hook on memory that now holds another unit would run our C on the wrong
  bytes. The guard is to verify the entry's code bytes against the media hash before every dispatch. The audits I read pin each span's sha256 (e.g. `DIGEST` in
  `tools/overlay/res0709_printer_0768_cpu_audit.py`); whether all 270 do is checked when the registry is built.

## 2. The contract each C body assumes

Measured over `include/poolrad/recovered/` (546 headers, 692 prototypes):

| What the model takes | Count | What a hybrid must supply |
|---|---|---|
| raw `uint8_t *ram, size_t ram_size` (exactly 1 MiB, wrapping at 1 MiB: A20 off) | 401 prototypes | `MemBase`. Two caveats: A000-BFFF is not in `MemBase` (VGA page handlers, `src/hardware/vga_memory.cpp`), and DOSBox-X's A20 state must be off or wrap-free |
| `pr_bus *bus` (RAM + the model's own `pr_ega` + `pr_port_env`) | 31 prototypes | an adapter that routes A000 accesses and OUTs to DOSBox-X's real VGA, not to a private `pr_ega`. `pr_bus_read8`/`write8`/`out` are plain functions (`include/poolrad/recovered/memory_bus.h`), so a hybrid build can link its own `memory_bus.c` |
| a 14-word CPU struct | 237 private struct types, plus 52 typedefs | a per-model copy-in and copy-out. There is no shared CPU type |
| value-level C with no CPU struct | 237 prototypes | **cannot be dispatched at a guest address**; these are not instruction-faithful |
| per-model budgets | 79 headers | declared constants per hook. A LIMIT must hand off (below), never hang |
| service callbacks: 18 `*_env` types and about 50 hook typedefs | — | see the table below |

| Service | Type (header) | In the hybrid |
|---|---|---|
| DOS open/close/read/seek/position | `pr_reset_dos_service`, `pr_system_close_service`, `pr_blockread_dos_service`, `pr_seek_dos_service`, `pr_filepos_dos_service`, `pr_r0802_02eb_env` | **The game's real DOS provides it.** The adapter loads registers and calls `CALLBACK_RunRealInt(0x21)` (`include/callback.h:65`) |
| INT 10h video | `pr_dos_intr_service`, `pr_r0802_0040_env` | the same, with `0x10` |
| INT 16h keyboard | `pr_crt_int16_service`, `pr_r0899_key_env` | the same, with `0x16`, under the #1326 contract (AH=01h reports only type-ahead) |
| GetTime | `pr_dos_gettime_service` | INT 21h AH=2Ch through the real DOS |
| ports and EGA | `pr_port_env`, `pr_ega` | DOSBox-X `IO_WriteB`/`IO_ReadB` and the VGA memory handlers, through the bus adapter |
| the 08AE driver's vectors and ports | `pr_r08ae_driver_env` | nothing credited needs it yet; 08AE is barred from credit (`docs/re/DRIVER_INTERNAL_CLASS.md`) |

Every service is backed by the game's own environment. **None has to be emulated.** The adapters have to be written, one
per service type (about 18).

**The handoff rule is what makes A safe.** Every model stops at an instruction boundary with an exact CPU and RAM state;
that is what each differential checks. Examples:

- a named stop leaves `CALL` executed and IP at the callee (`resident_0709_printer_0768_cpu.c:326`);
- ARGUMENT changes nothing;
- RETURNED has executed the RET itself (`resident_0709_printer_0768_cpu.c:348-350`).

So on any status other than RETURNED, the hybrid writes the model's state back and lets DOSBox-X continue **the original
bytes** from there. That is correct, but it also means **the rest of that body ran as original bytes**. The cost is real:

- 212 headers have callee, service or hook stops, and 61 more stop on escapes (TRANSFER, CODE, BOUNDARY).
- Only 20 headers are resumable named stops.
- Only 149 headers return nothing but basic statuses.

A body whose callee is not modelled returns to the guest at its first call, and the remainder executes as the original.

## 3. What breaks first

The cold-start path, from the native captures (`docs/re/NATIVE_COLD_START_SCREEN_GAPS.md:24-33`,
`docs/re/GAMEPLAY_PATH.md:21-24`):

1. The MZ entry and the packed loader.
2. 48 unit initializations, none with credited C. SYSTEM installs INT 00/23/24, CRT installs INT 1Bh, and the overlay
   manager installs INT 3Fh (`docs/re/FUNCTION_CEILING_CENSUS.md:32-34`).
3. The program body: unit 0x36, the font loader `0709:017C` (modelled, not credited), the DAX loader `0802:0716` (modelled,
   not credited), and the loading text through **`RES_0709_0626` (credited)**.
4. The publisher splash and title, then credits through **`RES_03F1_0137`** and 33 calls to **`RES_0709_0626`**. The wait
   loop runs through **`RES_0899_0104`** (`docs/re/COLD_START_CREDITS_SOURCE_CONTRACT.md:56-75`).
5. The code-word prompt (unit 0x19), through **`RES_0709_01DF`** and **`RES_0899_0059`** ReadKey
   (`docs/re/COLD_START_CODEWORD_SOURCE.md:60-66`).

The first three things that will not work, in order:

1. **The first credited body draws through a private EGA.** `RES_0709_0626`, `RES_0709_05EB`, `RES_0709_01DF` and
   `RES_03F1_0137` all take `pr_bus` (their headers). Run as is, they would write planes into the model's own `pr_ega` and
   the screen would not change. **The bus adapter is a prerequisite for the first visible credited body**, not a later
   stage. Of the cold-start credited bodies, only `RES_0899_0104`, a 30-byte idle frame, is pure RAM.
2. **Escapes hand most of the body back.** 0626 has CALLEE, CODE, LIMIT and TRANSFER statuses (its header). Any callee it
   does not compose returns control to the original bytes mid-body. How often 0626 completes RETURNED in a real cold start
   is UNKNOWN until measured. Stage 0 measures it.
3. **No registry maps a credited ID to a C entry point.** `strict_full_c_addresses.csv` maps an ID to a document only.
   Dispatch needs, for each of the 270:
   - the C symbol;
   - its call shape (one of 37);
   - its CPU struct;
   - its env and budgets.
   That table does not exist and has to be built and reviewed before anything is hooked.

Next in line:

- **Overlay load tracking.** The first credited overlay unit on the default path is 0x6C, the party menu with 6 credited
  bodies, after cold start.
- **Timing.** A C call costs no cycles.
- **A20 and wrap conventions.** The models follow DOSBox-X by VP rule i19555, which helps here.

## 4. Staged milestones

| Stage | What runs | Done means (each check named) |
|---|---|---|
| **0. Observer** | the patched DOSBox-X with the hook table and **no substitution**: at each credited entry, count arrivals and log CS:IP, and check the entry bytes against the media hash | One cold start to the code-word prompt produces a per-ID arrival log. This gives the **measured** list of which of the 270 credited IDs a cold start reaches, and how often. The byte check passes at every arrival |
| **(a) Title** | substitution for the cold-start credited bodies: `0899:0104` first, then 0626, 05EB, 01DF and `03F1:0137` behind the bus adapter | The game reaches the code-word prompt. For each hooked ID, the counters `entries`, `returned`, `handoff[status]` and `original_bytes_executed_in_span` (section 5) are logged. At least one body has `returned > 0`. At a fixed debugger stop (the existing `BP` at the code-word prompt, `docs/re/NATIVE_COLD_START_CODEWORD_KEY_TIMING.md:78-84`), the VGA planes and RAM are equal to a run without substitution |
| **(b) Gameplay** | add overlay load tracking, the DOS, INT 10h and INT 16h service adapters, and the generated dispatch table for every registry row with a raw-CPU model | `tools/oracle/capture_save_walk.sh` reaches its OCR'd position `15,4` then `14,4` with substitution on, and the counters show a non-zero `returned` for credited overlay bodies |
| **(c) Differential in the live game** | **shadow mode**: at each hooked entry, snapshot the CPU and RAM, run the C on a copy, let DOSBox-X run the original to the same stop, and compare unmasked. Services are replayed from the original's recorded INT log | every credited ID reached in (b) has shadow comparisons equal over the session, with any exclusion argued. This is the in-game form of "the differential still passes" |

**Reachable in one lane-week:** stage 0, plus (a) for `RES_0899_0104` alone, plus the bus adapter started. Full (a) with
the four EGA bodies is likely in week two.

**What (b) needs:** the registry, the overlay tracking with its eviction question answered, and the service adapters.

**What (c) needs:** a recorded service log. That is new code, and its size is UNKNOWN.

## 5. Proving that "this address executed our C"

Four measured checks, none of them an assumption:

1. **Counters at the dispatch point**, logged per ID:
   - `entries`;
   - `returned`, meaning the C ran to its own RET;
   - `handoff[status]`, meaning the C stopped and the original continued.

   An ID is only "executed by our C" for its `returned` count.
2. **A span detector.** The same per-instruction check flags any instruction executed inside a credited body's span when no
   handoff is pending. A non-zero `original_bytes_executed_in_span` with zero handoffs means the hook was bypassed, for
   example by a jump into the middle of the body or a stale table. It fails the run.
3. **Poison mode (a test mode, never the shipped one).** Overwrite a credited body's bytes, after its entry, with `HLT` or
   `INT 3` in guest memory. If the guest ever executes them, the run faults. This deliberately breaks "nothing patched", so
   it is only valid for IDs whose counters show `handoff = 0`. It is the strongest proof that the original did not run.
4. **Shadow mode (stage c)**: the differential, run on live game state.

**The failure this guards against.** A hook table that is never armed for an overlay, because the unit loaded at an
unexpected segment, produces `entries = 0` for that ID. A run reporting "N bodies substituted" must therefore list `entries`
and `returned` per ID, and never a total alone.

## 6. Price in lane-weeks

| Item | Estimate | Unknowns |
|---|---|---|
| Rebuild the heavy DOSBox-X and add the dispatch hook and counters (stage 0) | 2-4 days | the last local rebuild log ends `Terminated` (`.../resources/dosbox-x-build.log`); the build time is UNKNOWN |
| Registry: ID to C symbol, shape, struct, env and budgets, for 270 IDs | 2-4 days, scripted from the headers plus review | how many of the 270 have a raw-CPU entry model as opposed to only value-level C is **UNKNOWN until the registry exists** |
| Bus adapter, routing `pr_bus` to DOSBox-X VGA and IO | 2-4 days | whether DOSBox-X VGA latch semantics match `pr_ega` for every write mode the bodies use is UNKNOWN; the existing native runs suggest they do |
| Overlay load tracking | 3-5 days | eviction and unload behaviour is NOT FOUND |
| Service adapters, about 18 env types | 3-6 days | whether `CALLBACK_RunRealInt` can be called safely from the dispatch point inside the instruction loop is UNKNOWN (not checked in the source) |
| Shadow mode with service replay (stage c) | UNKNOWN, floor 2 weeks | recording and replaying INT results per hook is new |

**Totals:**

- stage 0 plus narrow (a): **about 1 lane-week**;
- through (b): **about 3-5 lane-weeks**;
- (c): **UNKNOWN**, at least 2 more.

None of this changes the strict-credit evidence. It adds a second kind of evidence: our C running in the game.

## Open questions I could not answer from the tree

- Where and when the overlay manager evicts or unloads a unit, and whether it rewrites `EA` stubs back to `CD 3F`.
- Which of the 48 unit initializations run before the program body, and what their targets are. Most are believed to be
  empty `RETF`s (`docs/re/FUNCTION_CEILING_CENSUS.md:116-122`).
- Whether the timer ISR (`RES_08AE_13EA`) is installed during cold start (`docs/re/NATIVE_COLD_START_SCREEN_GAPS.md:88-96`).
  Our C is not interrupted mid-call, but a guest ISR that expects to see an intermediate state would see none.
- How many credited bodies a cold start reaches at all. Stage 0 measures it, and nothing should be promised before it.
