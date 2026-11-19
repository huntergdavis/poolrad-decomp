# Slot-A Up: measured inputs for the `00F4 → 0165 → 13C6 → 0747` window (run 030)

This note replaces the synthetic inputs of the run003 composition replay
([SLOT_A_DOS35_COMPOSITION_RUN003.md](SLOT_A_DOS35_COMPOSITION_RUN003.md): scripted time `0` then `10`, a
declared `IN 61h` of `03`, an `OUT 61h` of `00`) with values measured in instruction-level `LOGL`
traces of one fresh native run, and attributes the remaining RAM differences to named writers.
It is bounded evidence for that window only. It claims no complete one-Up frame, no same-invocation
C route, and no function count or credit. Media, RAM images and traces stay in ignored `resources/`;
this note holds hashes, addresses and numbers only.

## Runs and pins

| Run (ignored `resources/oracle-runs/`) | Role |
| --- | --- |
| `t4492-slot4-logc-030` | main run: 00F4/0165/13C6/0747 anchors (ten segments + CPU14) and the traces below |
| `t4492-slot4-logc-029` | `LOGL` calibration run (its 00F4→0165 window was lost to the open-log quirk) |
| `t4492-slot4-logc-028` | **discarded**: keys were typed into the code-word prompt after a slow load |

Inputs unchanged: `GAME.OVR` `872964a3…`, `START.EXE` `d825df2b…`, `SAVGAMA.DAT` `abca2483…`.

| Run-030 file | SHA-256 |
| --- | --- |
| `RAM-10D2-00F4.BIN` | `dbbc1e3e4abd68aad0de729be534bace84ebb6c8cd074a4263096af194f73b37` |
| `RAM-10D2-0165.BIN` | `b227e2c1fadb3b1b8d3dd00a1020d83edddcdc193e16acbe26c93f181c5890c8` |
| `RAM-10D2-13C6.BIN` | `342882515c7dd4f852a17a15bc78c4d7bf147b2cb287e3e7169abbcaee02e270` |
| `RAM-2E60-0747.BIN` | `99d632bb8349102c6f41a495ce212367e07a4d35b32206b9654703e2e065e270` |
| `CPU-10D2-00F4.TXT` | `a1be6e91ab4f9386a3c495cfdb3c776e2758f6e802de20c56b30877918b301bb` |
| `CPU-10D2-0165.TXT` | `be674fba164d5b5bde0407d34d6cfd3f6f4515a05247c80bd227a2c5d3f73218` |
| `CPU-10D2-13C6.TXT` | `c5403b94eb3b242d82c7646dff8c566bcd54412e02af6e3d4e0112c97bf77c35` |
| `CPU-2E60-0747.TXT` | `c4b279854701fd67463ae3f7cb58a1d9eb805c21891786d10175bddbcfa56396` |
| `LOGL-A-13A3-to-00F4.TXT` (9 lines) | `be5611e0cf08bca6516b9827750a65275273d123ea2548f6a65b4d804074d854` |
| `LOGL-W1-00F4-to-0165.TXT` (411,601 lines, 132,123,921 B) | `0ec136c9104063de9448ef956719b0b73607db06492686748eb174bbf5722a6d` |
| `LOGL-W3-13C6-to-0747.TXT` (26 lines) | `4960c5b94fe3c09bf487a1e71069a1297c0648bab9927d07c9409835b1432fc0` |

## Measured inputs

* **DOS time.** The delay performs 924 GetTime polls (`INT 21h` at `1256:00A7`, each followed by an
  `INT 1A` inside the emulated DOS callback). The reading changes only when the BIOS tick advances:
  `08:19:44.76` for polls 1–561, `08:19:44.82` for 562–923, `08:19:44.87` for poll 924. The time body
  values (with the original's minute truncation) are 2,932,940, 2,932,946 and 2,932,951 against a target of
  first reading + 10 = 2,932,950, so the loop exits at poll 924. The BIOS tick dword goes 545,918 → 545,920.
  This replaces the scripted `0, 10` (and explains 027's four identical polls: they sit inside the first
  561). `SI = 8` at `0165` is the **hour** field (the time body leaves `SI` = hour), not residue; 027's
  `SI = 7` is its hour.
* **IRQ0.** 26 entries into the installed hook `10D2:13EA`, exactly 4.225 ms apart (PIT count `0x13B1` =
  5041 at 1.193182 MHz, measured emulator behaviour). The hook chains to the BIOS vector at IRQ0 #13 and #26
  (two `INT 1C` / clock callbacks), which is the +2 tick. 21 of the 26 interrupted the emulated-DOS delay
  loop at `F000:D10B`.
* **`IN 61h`.** `AL = 0x30` after the `IN` (so `AND AL,FCh` leaves `0x30`, ZF = 0, PF = 1, and `OUT 61h`
  writes `0x30`). This is the explanation of native `FLAGS = 7206` against the declared `03`'s `7246`.
  Bits 0–1 are not constrained by the flags alone (any value `0x30..0x33` gives the same stop); the trace
  gives `0x30` for this run. The other writes are as declared: `40h←FF`, `40h←FF`, then `C0h←9F`, `BF`, `DF`, `FF`.
* **Ports before `00F4`** (`13A3..13B0`): `43h←B6`, `40h←B1`, `40h←13`, with emulated stamps (`LOGL-A`).

## Replay result (`tests/replay/check_slot_a_measured_030.sh`)

Stage 1 starts at the native `00F4` image/CPU and runs the existing dispatcher, `1742`, delay and time
models with the measured readings (GetTime still supplies `CX:DX` only). It stops at `0165` with **CPU14
equal to the native transcription** (`AX=C0D7 BX=0000 CX=000A DX=002C SI=0008 DI=0000 BP=SP=3DB6 DS=14A0
ES=SS=1D16 FLAGS=7202`; the scripted run reached `7246`). The RAM differs from native `0165` in exactly 83
bytes: tick 1, PSP stack pointer 1, resident data `10F00..110FF` 38, stack 34, other 9, FNV-1a over
(address, model, native) `0fb46f0fef1bcf63`. That is the pinned residue: this replay has no IRQ0 hook and no
DOS/BIOS callback internals.

Stage 2 starts at the native `0165` image/CPU. The existing `17AE`, `1742` and `13B1` models reach `13C6` with
a RAM that differs from the native pre-`INT` image only by the six interrupt-frame bytes the model pushes (`SP`
and registers match). With the declared AH=25 binding (vector write, PSP pointer and nine saved registers, as the
run003 binding; the trace shows the callback returning straight to the stub `iret`) and `IN 61h = 0x30`, the
return to `2E60:0747` reproduces the native CPU14 (`FLAGS=7206`) and **all 655,360 RAM bytes (0 differences)**.
With the old declared `03` the RAM is identical but `FLAGS` is `7246` and `OUT 61h` is `00`, so that value is
refuted. This also means the previously unlogged `0165→13C6` window (54 native bytes) is explained by the
existing models without a trace.

## Store replay and attribution (`tests/replay/audit_slot_a_logl_030.py`)

The audit hash-locks the files above, re-derives every measured input from the traces, and replays all memory
stores of the `00F4→0165` window: 41,846 stores decoded from the logged instruction bytes (capstone) and addressed
with the logged registers, over the native `00F4` image. Two host callbacks are **declared, not traced**: the DOS
`INT 21h` callback (nine saved registers below the interrupt frame and the PSP stack pointer `0816E`, the run003
binding) and the BIOS IRQ0 clock callback (tick dword + 1). Result: the predicted image equals native `0165` in
every byte, i.e. all **131** native-delta bytes (tick 1, `0816E` 1, resident data 35, stack 85, other 9) are
reproduced with **0 unattributed**. Omitting the writer class that last wrote each byte leaves bytes unexplained
for the DOS callback (13), BIOS callback (1), IRQ0 hook (51) and ROM stubs (8); the IRQ0 frames themselves
write no byte that survives to `0165`. Every one of the 83 stage-1 residue bytes has a writer in an unmodeled class
(0 unattributed): IRQ0-hook 55, hook+ROM stub 2, DOS+hook+ROM 12, DOS+frame+hook 2, DOS+frame+hook+ROM 4,
frame-only 4, frame+hook 2, DOS 1, BIOS 1.

The attribution rests on equality of a replayed image with an independently captured one; the hidden callbacks'
semantics are hypotheses supported by that equality and by the run003 binding, not read from callback code.

## Mutation probes

Of 16 source mutants of the C test, 13 are killed (reading counts and values, hour, last-reading position, `IN 61h`
value `20`, the old declared `03` accepted as native, pinned count and hash, PSP pointer, register-save order, vector
segment). Three survive and are equivalent in this window: reading 2 plus one (it only has to stay below the target),
`IN 61h = 31h` (same `AND` result as `30h`), and the flags returned by the AH=25 service (overwritten by the following
`AND`, so not observable here).

## Missing windows: plan only (nothing run)

* **`0165 → 13C6`** (54 native bytes): explained by the models (above), so a trace is optional. If wanted: the first
  `LOGL` of a fresh run, count 5000 (the `1742` call alone is 373 instructions; `17AE` and the dispatcher steps are
  smaller), ended by the `13C6` breakpoint.
* **`0747 → 3492`** (6,165 native bytes; 6,201 for `348D→3492`): includes the rest of the coordinate writer, unit `0x3BD`
  entries, unit `0x7D` entry 1 with the overlay loader's DOS file calls (each emulated DOS call is about 330 trace lines),
  `0xBA:0x290B` and the two `0x464:0x2595` releases. Calibrate before any full window: (1) in a fresh run, `LOGC`
  (cs:ip only) with a very large count from `0747` to the `3492` breakpoint, measure the line count and bytes per line on the
  first 1000 lines, and check free space (100 GB free at this note; `LOGL` is about 320 B per instruction, so each million
  instructions is about 320 MB); (2) split the window with breakpoints at the boundaries (`07A1`, `07ED`, the near `0000`
  call, `0x3D0:2F`, `0x464:2595`, `3492`) so every `LOGL` stays small; (3) every `LOGL` must be the first log of the run
  or follow an exhausted one, because a log still open at a breakpoint stop is only closed by the next `LOGL`.

## Limits carried

CPU files are manual `EV` transcriptions; stop receipts are the debugger's breakpoint display only; the debugger halts
the guest at every stop and `LOGL`'s effect on timing is not excluded; the upper 384 KiB was never observed (the models
run on a 1 MiB image with the upper part zero, never read here); the trace has quirks: the first instruction at a
stop is logged twice, the `FLG` column is lazily evaluated (the audit rebuilds flags from the per-flag columns) and
timestamps have 1/3 µs granularity; `INT 21h`/`INT 1A` and the BIOS clock are emulated callbacks whose internals are
not instruction-traced. Cleanup of runs 029/030: outer and observer records 0, `post-audit.txt` present, inner record
143, full command line checked before every signal; run 029 needed no SIGKILL, run 030 had two SIGKILL reaps; 028 got
the same cleanup. The merged post-EXIT audit and the other run notes are unaffected.
