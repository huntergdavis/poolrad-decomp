# U7D +03A3: DOSBox-X synthetic oracle for the existing raw-CPU model

[`U7D_UPDATE_03A3.md`](U7D_UPDATE_03A3.md) documents `pr_u7d_update_03a3`
(PR #606). It is a complete raw-CPU, full-state model of GAME.OVR unit
`0x7D` `+03A3..+03D4` (50 bytes, 19 instructions) and composes the U7D
`+02F2` carry model, which is now strict. Its evidence is:

- a focused C test under ASan/UBSan;
- an original-byte Unicorn differential: 48 cases with the whole 1 MiB RAM
  and all 14 CPU words, 19/19 instruction starts and 4/4 conditional
  outcomes.

That document records that no DOSBox-X run was made. This note adds that run
against the existing model, unchanged. The VP approved it as the `+03A3`
strict-upgrade evidence on 2026-10-18 (i17904). No new C body is added.

## Oracle

`tools/overlay/u7d_update_03a3_native_audit.py` first checks the media
hashes. It then checks the `+03A3` body
(`3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad`) and
the `+02F2` body
(`8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9`).
It loads the whole unit `0x7D` into a DOS COM at paragraph `8800`.

**Each case rebuilds one 64 KiB data segment (DS) with:**

- the seven thresholds at `DS:363A`;
- the seven-word array at `DS:6DC2`;
- a non-cyclic party list of 0 to 6 nodes: head at `DS:5D96`, links at
  `+104`/`+106`, counters at `+30`.

The COM calls `+03A3` from a single call site with IRQs masked. It records
all 14 CPU words, a 128-byte stack window and the whole data segment. Each
record is compared with `pr_u7d_update_03a3` run from the same state, with
no mask.

**Result:** **128 cases** pass on the normal core with cputype 386:

- 64 use carry-mask threshold patterns, with word 5 at 0, 1, 2 or kept;
- 64 use random thresholds and values;
- 64 end with word 4 at the `63h` cap.

The result SHA-256 is
`743f223be74649ed49b118a2772b25a7ad8c1cd50435f278701ceb00e7907b10`, with COM
segment `0814`. On current main the existing Unicorn checker
(`tests/recovered/check_overlay_u7d_update_03a3.py`) still passes all 48
cases.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
  python3 tools/overlay/u7d_update_03a3_native_audit.py --output resources/u7d-03a3/native-report.json
```

Cyclic party lists are excluded on purpose: `+02F2` has no cap and would not
return, in DOS or in C. Original bytes and generated images stay under
ignored `resources/`. No classifier, queue or count ledger is changed.
Whether this completes the strict gate for `+03A3` is the verifying lane's
call; the original note also asked for caller and identity review.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_007D_03A3 --require-current-main
```
