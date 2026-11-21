# The four 08AE C models: `--native` (synthetic DOSBox-X) corroboration

The per-function audits of the four C models used by the saved-slot-A composition work
(`pr_r08ae_table_1742_cpu`, `pr_r08ae_table_17ae_cpu`, `pr_r08ae_vector_13b1_cpu` and `pr_r08ae_dispatch_00dc_cpu`)
each have two parity modes: `--unicorn` (original START.EXE bytes in Unicorn, 14 CPU words and the whole 1 MiB RAM,
no mask) and `--native` (a synthetic `ORACLE.COM` run in DOSBox-X, every case compared with the C model). The
Unicorn mode was re-run and reported earlier in this lane (with the link-composition note of PR #1971, not merged when
this note was written; the composition they serve is described in [SLOT_A_MEASURED_INPUTS_030.md](SLOT_A_MEASURED_INPUTS_030.md));
the `--native` mode had not been run by anyone in this lane. This note records
that run. It adds only an orchestrating audit and a check script: no model code, no `src/` or `include/` change.
**It establishes no parity, no function identity or count and no credit**: the models remain
`credit_claimed: false` / `identity: unresolved` and the native run does not change that.

## What `--native` does and what it compares

Each audit assembles a 16-bit `ORACLE.COM` (GNU `as`/`ld`) that copies the original bytes of the routine to a synthetic
segment, builds synthetic frames, calls the routine for every case and writes the registers and a RAM window to
`RESULT.BIN`; DOSBox-X (normal core, 386, `cycles=max`, SDL dummy video and audio) runs it headless; the audit then runs
the C model on the same case and compares. The native comparison is **narrower** than the Unicorn one:

| Model | Native cases | RAM compared per case | Masked | Notes |
| --- | ---: | --- | --- | --- |
| `1742` | 48 | 65,792 B (a 256-byte stack window plus the 64 KiB data segment) | 982,784 B | 14 CPU words, no flag mask |
| `17AE` | 48 | 65,792 B | 982,784 B | same shape |
| `13B1` | 32 | 264 B (256-byte stack window, the 4-byte slot `CS:1378..137B`, the 4-byte shadow vector entry) | 1,048,312 B | real DOS `INT 21h` with a shadow vector table; SF/ZF/PF masked because `IN 61h` reads live emulated hardware; port writes are **not** observed natively |
| dispatcher `00DC` | 48 | 65,792 B | 982,784 B | six callee stubs (`JMP FAR` into the harness) declared, callees not executed; stops RETURN 12, `CALL_1742` 11, `CALL_006C` 10, `CALL_17AE` 7, `CALL_181A` 4, `CALL_13B1` 3, `CALL_137C` 1 |

## Emulators

* **System build** (what the audits use as written): `/usr/bin/dosbox-x`, DOSBox-X 2026.01.02 SDL2, sha256
  `99d74601…`; the audits record it as a **declared substitution** for the pinned source revision.
* **Pinned debugger build** of this repository: `resources/hybrid/bin/dosbox-x`, DOSBox-X 2026.10.01 SDL2, sha256
  `d85a8563…`, run through a `PATH` shim. The audits still report `pinned_build_used: false` (their provenance helper
  never asserts a pinned build from a hash alone); the binary actually used is recorded by path and sha256.

## Result (`tests/replay/audit_slot_a_native_models_08ae.py`, run by `check_slot_a_native_models_08ae.sh`)

All eight `--native` runs (four models on two emulator builds) **exit 0**: every native case equals the C model in
all 14 CPU words and in the compared RAM (`13B1`: flags with SF/ZF/PF masked). The system pass also re-ran
`--unicorn`, so each report carries both sections.

| Model | Body SHA-256 | Original-byte starts | Unicorn cases (fresh) | Native result file, system vs pinned |
| --- | --- | --- | --- | --- |
| `1742` | `6128c512…` | 43 of 43 | 607 (603 return, 4 argument), 275 reference-checked, conditional 6 of 6 | identical (`541945f9…`) |
| `17AE` | `8bb73768…` | 43 of 43 | 1,023 (1,019 return, 4 argument), 275, 6 of 6 | identical (`96d0db18…`) |
| `13B1` | `0fa5df2c…` | 27 of 27 | 480 (4,320 events, all 256 `IN` values) | differs, see below |
| dispatcher `00DC` | `aae16730…` | 77 of 77 | 1,788 + 5 argument, 1,050 reference-checked, 16 of 16 | identical (`21b766ad…`) |

Ownership fields in the reports: `1742`, `17AE` and the dispatcher say `credit_claimed: false`, `identity: unresolved`;
the `13B1` audit has **no ownership record** (the same as in the Unicorn report). Unicorn masked RAM bytes: 0 in all four.

## Divergences from the filed reports

* **None in substance:** body SHAs, the expanded-image SHA, original-byte starts, Unicorn case counts, reference-checked
  counts, conditional outcomes, stop mixes and masked-RAM figures of the fresh `--unicorn` pass equal the filed values
  (the audit pins them and fails on any change).
* **Coverage differs by construction, not by result:** native mode compares 65,792 bytes per case (264 for `13B1`) of the
  1 MiB the Unicorn mode compares; it observes no port writes; the dispatcher's callees are stubs.
* **`13B1` across the two emulator builds:** the native result files differ, in a **run-dependent** number of records
  (9 of 32 in one run, 16 of 32 in the collected run), **only in the PF bit of the flags word** (xor `0x0004`, inside the
  audit's SF/ZF/PF mask), and in no other byte. The C model's environment supplies `IN 61h = 0`; the emulator reads live
  hardware, so the parity of that value differs between builds. The audit therefore pins the invariant (only masked
  flag bits, nothing outside the flags word), not the count.

## Mutation probes

Eleven mutants, **all killed**. Seven of the verifier's branches (wrong native case count, wrong body SHA, an ownership
record claiming credit, a wrong masked-RAM figure, a wrong emulator sha256, a perturbed Unicorn count, a flag mask without
PF; the last depends on at least one PF difference occurring between the builds, which both observed runs had). Four
mutations of the C model sources, applied in a scratch copy of the tree under the work directory (no source changed in
this tree), each caught by that model's own `--native` run with a specific mismatch: `1742` and `17AE` with the row stride
`0x30` changed to `0x31` (6 bytes differ in case 0), `13B1` with the AX high byte set to `FE` (`C=feff`, `DOS=ffff`), and the
dispatcher with `MOV AL,[5372]` reading `[5373]` (`C=000a`, `DOS=0004`). So the native corroboration has teeth.

## Status, stated plainly

The four models pass their own `--unicorn` and `--native` differentials on synthetic frames, on two emulator builds. The
native mode is synthetic: it does not run the models against the real game state, and it does not resolve the routines'
identity. The `credit_claimed: false` / `identity: unresolved` fields are unchanged and nothing here supports a parity,
function-count or credit claim; the composition notes (PRs #1968 to #1971, not merged when this note was written) are
separate same-trace checks that use these models.

## Limits

Manual CPU transcriptions and breakpoint-only receipts are not used in this slice, but every note that cites these models
carries them; the halted guest and `LOGL` timing do not apply here (headless synthetic runs, no `LOGL`); the upper 384 KiB
is not observed (the native comparison covers 65,792 bytes, `13B1` 264 bytes, of 1 MiB); host-time noise appears only
through `IN 61h` live hardware reads, which the `13B1` audit masks; the emulators run headless with SDL dummy drivers; the
system emulator is a declared substitution and `pinned_build_used` stays `false` in every report.

## Next step

Identity is the open question: an ownership record for `13B1` (it has none) and evidence that links these routines to
named functions would be needed before any claim beyond "differentials pass"; that is outside this slice.
