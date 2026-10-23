# U33 +1403 evidence refresh (2026-10-18)

This evidence-only change reruns the existing complete model and its existing
oracles at main 883ad93a79684a4d665e56fd7e211a706bd63eb7. No C, header, test,
audit/native tool, classifier, ledger, or acceptance file changes. Independent
review decides identity and credit; this PR does not self-credit.

| Existing check | Fresh result |
| --- | --- |
| Original-byte differential | 4,084 cases; CPU14 and all 1,048,576 RAM bytes; 18/18 sites, 8/8 conditional outcomes |
| DOSBox-X normal/386 caller-prefix capture | 1,024 cases; CPU14 and 64 frame bytes, preserved AH, no FLAGS mask |
| DOSBox-X wrapped RETF proof | Two cases at SP FFFE/FFFF; CPU14, 96 stack/sentinel bytes and 16 auxiliary bytes; distinct linear-CS decoy rejected, no masks |
| Existing strict-C11 ASan+UBSan test | Pass, including all 65,536 signed-byte pairs, exact flag vectors, guards, and three wrapped-frame regressions |

The predicate remains exactly [1403,1434), 49 bytes / 18 instructions,
GAME.OVR file [55764,55813), SHA-256
 a749aec8ac16739928c52f412b1bd4447e3bb668500b56bd76b1a5140288d62b.
Aligned incoming calls +17C8 and +18C8 in U33 +1577 and their original
argument/result instructions are checked by the unchanged audit.

The native result hashes reproduce the existing evidence:

- Caller-prefix RESULT.BIN: 07a3f5291f0e5187ad4e4a0481ea5d2478d24a113770e7d4148aad4c6b4f35a7.
- Wrap RESULT.BIN: f468e5a6e06f0db384ac7d22d21cfcbe897d1f81c88d1a80f446772850e392ee.
- DOSBox-X executable: 99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.

The six Unicorn cases at SP FFFE/FFFF across three SS choices remain
excluded because that engine reads the second RETF word linearly. The two
separate native cases prove the implemented wrapped-word behavior at those
SP boundaries; no emulator result is silently repaired. Natural gameplay
reach, loader dispatch and caller-local meanings remain unmeasured. Native
captures compare the stated windows, not the whole RAM image.

START.EXE SHA-256:
 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The reused model/audit revision is 62065e747ff4ceee7e7c9b61361cf4b2b82b8a4d.
See [the existing ABI and native-repair note](U33_BOTH_OUTSIDE_1403.md).
Recall query U33 1403 U6C 3FB3 408C U6A 0824 evidence returned Claude
session 281c8700-5ac; its lane assignment is reused only for coordination,
not as parity evidence.

Reproduce with owner media in ignored resources/DOS/POOLRAD:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u33_both_outside_1403_audit.py --output resources/evidence-1403/audit.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u33_both_outside_1403_native.py
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u33_both_outside_1403_wrap_native.py
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -g -I include tests/recovered/test_overlay_u33_both_outside_1403.c src/recovered/overlay_u33_both_outside_1403.c -o resources/evidence-1403/test
    resources/evidence-1403/test

Ignored reports, logs and a commit-bound receipt are retained at
resources/evidence-1403/evidence.json in the author worktree. Media and
original-derived captures remain private. target_function: OVL_0033_1403;
source_candidates: the existing raw predicate; signals: pinned identity,
all-site/all-edge differential and native wrap proof; score: measured
18/18 and 8/8; verdict: candidate for independent acceptance;
review_notes: engine wrap discrepancy and natural-route gap retained.

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0033_1403 --require-current-main
```
