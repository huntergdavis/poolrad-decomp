# Same-save DOS / hybrid checkpoint receipts (t116)

This harness loads the same slot-A game files in two independent DOSBox-X
processes: observer-only original execution and the hybrid's route mode.
It compares the displayed frame and captured state at a signature-verified
`RES_0A40_02FD` keyboard-poll entry after a named route gate. Identical save
files do **not** establish identical complete machine state at boot.

## Cases and results

The `loaded` case stops after loading slot A at 15,4. The separate `turn`
case loads that save, sends Right once, and captures at 15,4 after the turn.
The second fixture is collected from execution after the harness was written;
it is not constructed from a model's expected output. It does not prove that
no prior session ever reached the same gameplay state. No state is adjusted
or masked to make either case pass.

Measured on 2026-10-25 from capture-tools revision `ff93d1d0` (model sources
unchanged from the fresh `68a0a960` main baseline):

| Case | Captured view | Different RGB pixels | Different registers | Different RAM bytes (1 MiB) | Different VGA bytes (2 MiB) | Raw captured-state result |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| [Loaded save](HYBRID_SAME_SAVE_LOADED_RECEIPT.json) | 15,4 W, 10:50 | 0 | 0 | **111** | 0 | **FAIL** |
| [New Right-turn state](HYBRID_SAME_SAVE_TURN_RECEIPT.json) | 15,4 N, 10:50 | 0 | 0 | **137** | 0 | **FAIL** |

All 16 captured register fields and all captured video metadata match in both
pairs. Each image is 640×400 RGB24. The two cases have different frame hashes,
while the original/replacement hashes within each case are identical:

- Loaded: `8b302d814bda9f7b1f0af9305655ecae7c91491f2e6880d7ba1d689708e17f8b`.
- Turn: `2f9b9f23e84c405cb44ff20fd53131bde26f6ec8923fd015c1d67f570e5c426b`.

Both pairs share save/state ID
`728b1bb7b54988065aa1ac29b538a62538367e9dee28356c2f6ed826d955f25c`,
derived from the input save-file hash lines retained in each receipt. All
four captures are at live `1264:02FD`, before the keyboard-poll instruction.
All 32 signature bytes are checked. Poll ordinals differ and are recorded.

The loaded replacement records **439 routed C returns / 1 hand-back**; the
turn replacement records **483 / 1**. Both originals record **0 / 0**.
The four drivers exited successfully. The report CLI exits 1 for each pair,
preserving the measured FAIL. Thirteen synthetic comparator tests pass.

The RAM differences remain unlocalized. These are independent boots with
different timing and poll counts; the receipt does not diagnose a C defect.
No RAM or flag mask was applied, and the newly collected turn case was not
adjusted after its FAIL. Raw memory and images remain private; the receipts
publish full comparison counts, hashes, and the first 32 differing offsets.

## Capture boundary and failure behavior

The existing hybrid observer checks the requested IP and every nonzero-mask
signature byte before calling the checkpoint. The private registry marks
`RES_0A40_02FD` unique and uses a 32-byte signature. Each receipt includes
the registry SHA-256, signature SHA-256 and checked-byte count; original
signature bytes remain private.

The route driver reuses the screen gates and private-font coordinate decoder
from `r1_driver.py`. After the terminal gate it creates `checkpoint.arm`.
At the next matching arrival the hook consumes the arm file and captures:

- 32-bit general registers, EIP and flags; CS, DS, ES, SS, FS and GS;
- all 1 MiB of `MemBase` backing RAM, without masking;
- all allocated VGA backing memory;
- mode, display start, real start, scan length and display dimensions;
- observer arrival/instruction counters and actual C routed-return/stop counters.

The disposable emulator then stops itself with SIGSTOP. The driver checks
the process's stopped state and captures its window as RGB24. This is the
**last presented frame while the guest is stopped**, not a newly completed
frame fenced to that instruction. The driver terminates and continues only
its own disposable process group so the emulator can exit normally.

A signature mismatch never invokes the checkpoint. After 30 seconds without
a stopped checkpoint the driver fails; missing captures cannot produce a
receipt. The comparator also refuses differing input manifests, differing
builds/registries, nonunique target rows, mismatched case/arrival IDs, missing
registers, truncated captures and a replacement run with zero C routed returns.

`PASS` requires equality of every captured register, video metadata field,
RAM byte, VGA byte and RGB byte. `FAIL` preserves unmasked counts and hashes.
Neither verdict covers omitted state. Signature identity is checked; arrival
ordinals and emulated times are reported rather than required to coincide.

## Build and reproduce

Start with pinned DOSBox-X `5a9c7c53` and the tracked cumulative patch from
[the hybrid bus route](../HYBRID_EGA_ADAPTER_V1_ROUTE.md). Keep source, objects,
original media and captures under ignored `resources/`. Build the dispatch
models from the same Poolrad revision using `tools/hybrid/build_models.py`;
do not reuse another lane's modified dispatch header or model object.

For this measurement, the private pinned emulator build was copied into this
lane. The dispatch header was restored to the tracked cumulative patch, the
model object was rebuilt from this checkout, and the CPU core was rebuilt
with the checkpoint hook. Unchanged base-emulator objects were reused; an
independent clean rebuild of every DOSBox-X object was not performed.
The normal CPU core uses `-O1 -g0`; linked C models use `-O2`. The private
`-O2` CPU compile was stopped after more than ten minutes without an error,
following the private-build fallback recorded in Dunking Sheep session
`01a12b3e-db20-70e1-95f9-d3fe44b8d309`. An initial `-O0` trial then exhausted
the inherited 300-second startup-idle budget before the code-word gate:
`t116-turn-original` ended with `unknown screen; expected code` and no
checkpoint. The other three initial trials were stopped or cancelled.
The retry uses `-O1` and an explicit 900-second gate / 3600-second total
budget. This changes capture infrastructure, not the saved-state fixtures.
Both comparison sides use the same resulting binary; timing equivalence
is not claimed. The failed startup produced no parity verdict.

Install the additional opt-in hook before compiling the normal CPU core:

```sh
python3 tools/hybrid/install_parity_checkpoint.py resources/hybrid/dosbox-x-src
```

Build the model closure and the source-hash manifest consumed by the runner:

```sh
poolrad-heavy.sh python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import re
import subprocess
import sys

hybrid = Path('resources/hybrid')
header = (hybrid / 'dosbox-x-src/src/cpu/pr_hybrid_dispatch.h').read_text()
entries = sorted(set(re.findall(r'\b(pr_\w+)\(', header)) -
                 {'pr_bus_read8', 'pr_bus_write8', 'pr_bus_out'})
subprocess.run([sys.executable, 'tools/hybrid/build_models.py',
                '--entries', ','.join(entries)], check=True)
names = (hybrid / 'pr_models.sources').read_text().splitlines()
(hybrid / 'model-source-sha256').write_text(''.join(
    sha256((Path('src/recovered') / name).read_bytes()).hexdigest() +
    '  src/recovered/' + name + '\n' for name in names))
PY
```

Use a fresh object cache when model headers change: the existing model
builder's cache key covers the `.c` file, not transitive headers. This run
started with an empty private model cache. Compile `src/cpu/libcpu.a` with
this checkout's `include/` in `CPPFLAGS`, then link `src/dosbox-x` with the
rebuilt `resources/hybrid/pr_models.o` appended to its `LIBS` (as in the
parent hybrid build instructions). Preserve the emulator's configured
include and linker flags.

The compiled private executable belongs at `resources/hybrid/bin/dosbox-x`.
The all-ledger registry belongs at `resources/hybrid/registry_all.tsv`.
Use the shared heavy-job wrapper for builds and native runs:

```sh
poolrad-heavy.sh tools/hybrid/run_parity.sh t116-loaded-original-o1 original loaded
poolrad-heavy.sh tools/hybrid/run_parity.sh t116-loaded-replacement-o1 replacement loaded
poolrad-heavy.sh tools/hybrid/run_parity.sh t116-turn-original-o1 original turn
poolrad-heavy.sh tools/hybrid/run_parity.sh t116-turn-replacement-o1 replacement turn
python3 tools/hybrid/parity_report.py \
  resources/oracle-runs/t116-loaded-original-o1 \
  resources/oracle-runs/t116-loaded-replacement-o1 --output resources/loaded-receipt.json
python3 tools/hybrid/parity_report.py \
  resources/oracle-runs/t116-turn-original-o1 \
  resources/oracle-runs/t116-turn-replacement-o1 --output resources/turn-receipt.json
```

The report command writes the receipt and exits 1 for a measured FAIL. It
raises an error without a receipt for invalid/missing input. Run names must
be new; the existing preparation script refuses to overwrite runs. Both
branches use the same checkpoint-enabled executable. `original` leaves
dispatch unset; `replacement` uses all IDs in that build's tracked dispatch
table and `PR_HYBRID_BUS_MODE=route`. Run manifests pin the binary, model
object, driver, capture header and cumulative patch.

Synthetic negative controls:

```sh
python3 -m unittest discover -s tools/hybrid -p test_parity_report.py
```

## NOT verified

- Identical complete machine state before the two boots; identical polling
  ordinal or emulated timing at capture.
- A new completed host frame corresponding to the captured guest instruction.
- Full VGA controller/palette state, PIC state, DOS handles, timer state or a
  restorable machine snapshot. `MemBase` is backing RAM, not device-mapped RAM.
- Whole-game replacement. Service-bound dispatch still uses original DOS
  effects, and unlisted functions remain original code.
- Other saves, combat, full-game parity, or any accounting/credit change.

## Reused evidence

The implementation builds on [HYBRID_STAGE_B_OBSERVER](../HYBRID_STAGE_B_OBSERVER.md)
for the slot-A route and input provenance, [HYBRID_EGA_ADAPTER_V1_ROUTE](../HYBRID_EGA_ADAPTER_V1_ROUTE.md)
for route versus replay semantics, and `r1_driver.py` for gated startup and
private-font coordinate reads. Dunking Sheep memory was queried before
implementation; its broad hits supplied no t116 measurement reused here.
VP inbox i24339 requires a separately reported new-state fixture; i24355
accepts unmasked differences and requires signature failure to fail closed.
