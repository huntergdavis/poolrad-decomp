# U6C +3FB3 evidence refresh (2026-10-18)

This evidence-only change reruns the existing complete tier caller and
its existing differential/native tools at main 883ad93a79684a4d665e56fd7e211a706bd63eb7.
No model, header, test, tool, classifier, ledger, or acceptance file changes.
The merged +3F31 callee is composed unchanged. Independent review owns
identity and credit; this PR does not self-credit.

## Explicit comparison profile

**After a successful IDIV, the six architecturally undefined arithmetic
FLAGS bits 0x08D5 (CF/PF/AF/ZF/SF/OF) are excluded from comparison.** The
remaining FLAGS bits are compared exactly (mask 0xF72A). The C model keeps
the preceding ADD's arithmetic flags. Clamp and pre-fault boundaries
compare all FLAGS bits. This is declared-undefined behavior evidence,
not unmasked CPU14 parity. Carry this qualification into any independent
acceptance/checkpoint record.

**Native divide-fault captures normalize three words of the exception
frame** back to their known pre-vector values, reconstructing the C
DIVIDE_ERROR boundary: caller CS, record offset and record segment. The
original-byte differential stops before the vector transition and needs no
such normalization. The game's exception handler is not implemented or
claimed. Normal native returns are not fault-frame-normalized. These
qualifications remain part of the evidence, not proposed source changes.

## Fresh results

| Existing check | Fresh measurement |
| --- | --- |
| Original full-state differential | 3,088 cases; all 1,048,576 RAM bytes and CPU14 under the flag profile above |
| Original outcomes | 2,946 returns (2,656 divisions, 290 clamps), 136 divide boundaries, 6 changed internal returns |
| Instruction/branch coverage | 132/132 caller+callee sites; 24/24 static conditional outcomes |
| DOSBox-X normal/386 | 1,538 cases: 1,536 returns (1,400 divisions, 136 clamps), 2 actual divide faults |
| Native comparison windows | CPU14 under the stated profile; 64 frame bytes, 256 DS table bytes, 288 record-region bytes |
| Existing strict-C11 ASan+UBSan focused test | Pass, assertions enabled; guards, division/clamp/fault and changed-return cases |

Complete caller [3FB3,408C), 217 bytes / 88 instructions,
GAME.OVR file [121138,121355), SHA-256
 bf1e24f3fbf0660c08259b756b918b0685b1488c3c530599e55e80778ebd397f.
The existing audit pins the original caller body, +3F31 dependency and
aligned incoming active-character call sites. Native setup supplies
synthetic data and runs the original incoming prefix/caller/callee; earlier
game conditions, field names, natural saved-game reach and loader dispatch
remain unmeasured. Native RAM claims are limited to the listed windows.

Native RESULT.BIN SHA-256:
 b3fb207324dbc8755d0ed6cbc5207e8bca511e9c21504173839daf305d3b5cad.
The result header contains allocation segments; another environment may
produce a different checksum while passing the same comparisons.
DOSBox-X executable SHA-256:
 99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3.
START.EXE SHA-256:
 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

## Provenance and reproduction

Existing caller/audit revision: 4b48f4950a6abafe63a52ef1ad29531d4e53690a.
Existing +3F31 revision: 52b1cb8be3de3005264fcdeca551c83ca11248ea.
See [the existing complete ABI note](U6C_RECORD_TIER_3FB3.md).
Recall query U33 1403 U6C 3FB3 408C U6A 0824 evidence returned Claude
session 281c8700-5ac; its assignment is coordination only, not parity proof.
VP/reviewer correction i17730 explicitly requested this declared-undefined
IDIV profile. Acceptance and checkpoint edits remain with that lane.

With owner media under ignored resources/DOS/POOLRAD:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u6c_record_tier_3fb3_audit.py --output resources/evidence-3fb3/audit.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay python3 tools/overlay/u6c_record_tier_3fb3_native.py
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -g -I include tests/recovered/test_overlay_u6c_record_tier_3fb3.c src/recovered/overlay_u6c_record_tier_3fb3.c src/recovered/overlay_u6c_record_table_sum_3f31.c -o resources/evidence-3fb3/test
    resources/evidence-3fb3/test

Logs, reports and a commit-bound receipt remain at
resources/evidence-3fb3/evidence.json in the author worktree. Media,
original-derived code and native captures stay private. target_function:
OVL_006C_3FB3; source_candidates: existing caller plus +3F31; signals:
pinned identity, complete sites and feasible edges, original/native returns
and faults; score: 132/132 sites and 24/24 static outcomes;
verdict: candidate for independent acceptance under the declared profile;
review_notes: IDIV FLAGS exception, normalized native fault frames, and
natural-route gap must remain visible.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_006C_3FB3 --require-current-main
```


## Division flags after #1433 (recorded by the acceptor, 2026-10-22)

#1433 (par-d) corrected this model's flags after successful `DIV`/`IDIV` to the pinned DOSBox-X rule. The earlier
Unicorn oracle masked all six flags at those exits, and there was no native run, so the PM marked the flag claim at
the division exits unproven (i21495). par-d2 re-ran both checkers at `23acc5bf` (#1433 plus the 4235 audit import
fix), with FLAGS unmasked in the native run (i21533).

**Per-credit sentence:** `OVL_006C_3FB3`'s evidence did not depend on the division flags. The credit stands.

Native 1,538/1,538 cases (1,536 returns plus 2 deliberate divide-error exits; 1,400 divisions), `flags_masked = 0`. Unicorn audit rc 0 on 3,088 cases (2,946 returns, 136 divide errors, 6 transfers, 2,656 divisions) under the declared division split.

Provenance, stated plainly: par-e checked par-d2's run logs (`*_audit.log` and `*_native.log` per body) (kept in par-d2's worktree at `resources/evidence-par-d2/flags2/`, a lane-local, git-ignored path; moved there from `/tmp` on 2026-10-23) for every
number above. par-e also confirmed that this body's C source, header, audit and native script, and the SYSTEM sources,
are byte-identical between `23acc5bf` and main `5af0fb2e`. par-e did not re-run these checkers. The logs' `source_sha256`
is the original body's hash, not the C's, so the source identity above comes from the git diff.
