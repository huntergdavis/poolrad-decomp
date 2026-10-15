# U3C2 +0913 jump from an original-produced entry frame

`pr_u3c2_jump_successor_0913` models only the two-byte near JMP at +0913.
It changes IP to +0918 and stops before the successor's first instruction.
Every other CPU word is unchanged; the C interface has no RAM argument or
access. This is a bounded gate with zero strict full-C credit, not a
whole-caller model or a natural/native-frame claim.

| Artifact | Measured identity |
| --- | --- |
| GAME.OVR | 206105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Caller +0909..+0BD8 | File `[197723,198443)`; SHA-256 `38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed` |
| Original frame producer +0909..+0912 | Four instructions; SHA-256 `fc6542ae7bd0b6f694715bb0984ef7bf2118607c0dff04566c1352b189f8214b` |
| JMP +0913..+0914 | File `[197733,197735)`; 2 bytes / 1 instruction; SHA-256 `c3ce784032215a5bb8028beba705a6f0b97cfeb0ae5541802743e954da13169e` |
| Stop boundary | Before +0918's first instruction |

The audit starts the pinned **original** four-instruction entry prefix with
synthetic initial RAM/CPU, stopping before +0913. It freezes the resulting
1 MiB RAM and all 14 CPU words, gives the C jump a copy of that CPU, and
resumes the same original machine for exactly one JMP. No guest CPU/RAM
edit occurs after capture. These are original-byte-produced frames from
synthetic inputs; they are not captured natural game or native DOS frames.
The previously approved C entry prefix is not executed or linked here.

The original jump's entire RAM must equal the frozen frame's RAM, and the
C CPU must equal all 14 original output words. A five-instruction producer
cap and a two-hook jump cap stop execution at the two boundaries. No
successor instruction or external call executes. Immutable code, no debug
exceptions and no asynchronous traps are preconditions; original inputs
keep TF clear. Null or wrong-IP input is rejected without mutation.

## Reproduction

With measured Capstone 5.0.6 and Unicorn 2.1.4 importable:

```sh
python3 tools/overlay/u3c2_jump_successor_0913_audit.py --quick \
  --capture-frame --output resources/u3c2-jump-0913/quick-parity.json
```

`--capture-frame` saves the first original-produced RAM/CPU snapshot under
ignored `resources/u3c2-jump-0913/`. The JSON report records a digest of the
ordered original input frames. The quick matrix varies SS, SP, saved BP,
initial flags and seeded full RAM. All original media and derived frames
stay under ignored `resources/`; public C tests contain synthetic data.

The quick gate passes 190/190 original-produced frames, covering the one
JMP site and reaching +0918 in every case. All 14 CPU words match, and the
original branch leaves every byte of the captured 1 MiB RAM unchanged.
There are zero post-capture guest edits, masked flags or emulator
substitutions. The ordered original-frame digest is SHA-256
`70364c7e86f9a84dfef119245f882d53f550fb7ff4efefe7565e18b1bc29a06f`.
Both states of CF/PF/ZF/SF/OF occur in the input frames; AF is defined zero
by the original SUB that produced them and remains unchanged by JMP.

The focused C test checks all 65,536 flag words with varied CPU fields,
requiring only IP to change, plus null/wrong-IP rejection. These are C
state-preservation tests, not native execution of every flag combination.
Warning-clean C11 ASan/UBSan validation passes. The cached focused CMake
build and `poolrad_overlay_u3c2_jump_successor_0913_tests` CTest pass (1/1).
Tracked-file media hygiene and the copied research snapshot checksums pass.

## Parity review

- `target_function`: U3C2 +0909 restricted to its +0913 JMP.
- `source_candidates`: `src/recovered/overlay_u3c2_jump_successor_0913.c`.
- `signals`: pinned jump identity, original-produced frame provenance,
  exact source/stop boundaries, all-CPU and whole-RAM invariants.
- `score`: 1/1 jump site and 190/190 original-produced frame comparisons;
  bounded coverage only, with no full-function score inferred.
- `verdict`: `match` for this JMP under the documented preconditions;
  whole-caller parity remains `unclear`.
- `review_notes`: the entry prefix produces frames but is outside the C
  jump implementation. The successor, remaining caller, callee and return
  path are unmodeled. No natural/native or strict-function claim is made.

Bounded `deja "poolrad U3C2 0913 original prefix frame JMP 0918 parity"`
completed with no matches and reported zero sessions searched. Reused
conventions come from reviewed `54c0dd6` original entry boundaries,
`2235fc5` finite-stop/full-state harness, merged `u86_procvars_3413_audit.py`
CPU/A20 helpers and `overlay_u6c_record_table_sum_3f31.h` CPU interface.
This branch has no code dependency on the published entry-prefix or
caller-window branches.
