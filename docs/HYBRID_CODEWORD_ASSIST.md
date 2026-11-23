# Code-word prompt: deterministic exact-state reader and assisted input (ASSIST, emulator side)

**Label: ASSIST.** Everything here is an assisted-input feature. It is not parity evidence, not a function
identity, not a credit, and not part of any manual-path claim. The manual path with the assist off is the
baseline, and it is unchanged by this change.

## What exists in this change

- `tools/hybrid/codeword_assist.py`: a pure reader and gate. It takes an exact snapshot and returns either a
  recognised prompt occurrence or a named refusal, and a gate that emits at most one answer plan per occurrence.
  It imports no host typing, OCR, imaging or memory-writing module (checked by a test).
- `tests/hybrid/test_codeword_assist.py`: 21 reader tests on **synthetic fixtures only** (section "Fixtures").
- `tests/hybrid/test_codeword_assist_conformance.py`: 1 test. It compiles the C++ driver with `-Werror`, runs it,
  and compares its transcript with the Python transcript. The scenarios include an independent-rows case (see
  "Conformance").
- **22 tests in total**, all passing in this round (`python3 -m unittest discover -s tests/hybrid -p 'test_codeword_assist*.py'`,
  22 OK).
- This note.
- `tools/hybrid/dosbox-x/pr_codeword_assist.h` and `tools/hybrid/dosbox-x/conformance_main.cpp`: the C++ reader and gate,
  with the same transcripts as the Python reader.
- `tools/hybrid/dosbox-x-b6abbd5-codeword-assist.patch`: the emulator hook, a patch of three files: two guarded
  edits (`src/cpu/core_normal.cpp`, `src/ints/bios_keyboard.cpp`) and the new header `src/cpu/pr_codeword_assist.h`.
  It is built and run in a scratch copy only; it is not merged anywhere.
  - **Clean-apply claim (corrected):** the patch applies with zero fuzz (`patch -p1 -F0`) to pristine files exported
    from the commit `b6abbd5`, checked for the three files, and a full pristine tree was built with it. An earlier
    patch (commit `ec81e80a`) was generated against the uncommitted working tree of the shared checkout, including
    its dispatch hook and a `parity_checkpoint` include. Its `core_normal.cpp` hunks failed on pristine `b6abbd5`.
    That patch is superseded and is not a reproducible base.

Nothing in the game was changed. The shared DOSBox-X checkout and the shared controller were not modified.

## Reproducibility record

| Item | Identity |
| --- | --- |
| Base commit | `b6abbd5980a885f5f310a4088c59a8688d1b116c` (tree `d1a93618c882a4803b39e2cc7c446308a3332fae`) |
| Patch | `tools/hybrid/dosbox-x-b6abbd5-codeword-assist.patch`, 17611 bytes, sha256 `8de068ac4413c0e7ceca1e1c28a56262da80a545b878ff94cc89771c5c4868c1` (comment-only change to two lines; applies with zero fuzz) |
| New header | `tools/hybrid/dosbox-x/pr_codeword_assist.h`, sha256 `4e6e5ce2ee4de90fd99dca13775b4a35ee2206acc7c8b26c21b7f43e0816ebf5` (equals the copy the patch applies) |
| Patched `src/cpu/core_normal.cpp` | sha256 `03ff58a8913e356498beeb3d6012da7bc39864b404a03f81cdfa0b288ba2e81c` (byte-equal to a fresh zero-fuzz patch result) |
| Patched `src/ints/bios_keyboard.cpp` | sha256 `d5bf1cf225fa529990fbbfcf89bdfaede85673f7b03d6682c5817b541997fa06` (byte-equal to a fresh zero-fuzz patch result) |
| Built binary (scratch; ignored from this commit) | `scratch/build-b6abbd5/src/dosbox-x`, sha256 `502ffe33da0439d244bba5160fe7a557cdcc2673faaca2ec5c1fa2c1fd1e9871` |

The binary was built from the pristine export plus the patch as it was before the comment-only fix to two lines (full build, 543 objects, 0 errors). Its sha16
`502ffe33da0439d2` is recorded in the action logs of runs 052 to 056.

Repository hygiene (this commit): `scratch/` (builds, copies and run caches, 3.1 GB by `du -sh`, including a 132 MB (126 MiB)
binary) is now listed in `.gitignore`. Before this commit it was untracked and not ignored, so `git add -A` must not be used.

Attribute scope: `.gitattributes` gives `-whitespace` to **one file only**, `tools/hybrid/dosbox-x-b6abbd5-codeword-assist.patch`.
`git check-attr whitespace` reports `unset` for that file and `unspecified` for the other tracked patches
(`tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`, `tools/hybrid/dosbox-x-5a9c7c53-stage0-observer.patch`,
`tools/hybrid/dosbox-x-b6abbd59-dispatch.patch`) and for this doc.

Whitespace count, default rules, pre-attribute blob `98b4b7e2`, whole file: **eleven hits**, at lines 4, 6, 11, 12, 13, 14, 15,
17, 18, 19 and 32 (four `blank-at-eol`, seven `space-before-tab`). Reproduce with
`git --attr-source=4b825dc642cb6eb9a060e54bf8d69288fbee4904 diff --check 4b825dc642cb6eb9a060e54bf8d69288fbee4904 98b4b7e2 -- tools/hybrid/dosbox-x-b6abbd5-codeword-assist.patch`.
The empty tree as attribute source turns the repository attribute off. The blank-at-eof rule is checked by the tail test: the patch's last line is `+#endif` followed by a single newline, not a blank line, so that rule adds no hits. A second scan of blank-at-eol and space-before-tab only (kept in the ignored scratch directory, not committed) gives the same eleven lines, rerun on 2026-11-22. With the repository attribute active, the same whole-file command gives no hits.

Commit-level count (the seven-hit figure): `git --attr-source=98b4b7e20bf0dc7b7857e52ee898a74cc561b507 diff --check 98b4b7e2~1 98b4b7e2 -- tools/hybrid/dosbox-x-b6abbd5-codeword-assist.patch`
gives seven hits, at lines 4, 6, 11, 12, 13, 14 and 19. It reports only the lines that commit added or changed. The bare form
`git diff --check 98b4b7e2~1 98b4b7e2 -- <patch>` gives no hits at HEAD, because the repository `.gitattributes` (`-whitespace`) is active,
so the attribute-source override is required for the count to reproduce. `git cat-file -e 98b4b7e2:.gitattributes` fails (the file is
absent at that commit), so default rules apply under that override.

Identical-code basis for the header: two header comments (lines 101 and 270) were corrected after the build. This is a comment-only
diff, checked with `git diff` against `98b4b7e2`, which changes only those two lines. It is also a comment-stripped comparison: with
block comments removed (23 in each copy) the build-tree copy and the tools copy are equal. The comment-stripped text has SHA-256 `610ecc37b5b78702c6c85ad1f4981fac5ad1aeb72de3e505bafde19b9aae911a` in both copies (prefix `610ecc37b5b78702`); no line comments are present. This is not a rebuild; the statement that
the compiled code is identical rests on those two checks.

## Exact prompt state (what the reader reads)

The reader decides only from these, read at the INT 16h keyboard poll of the input loop:

| Field | Source (observed) | Why |
| --- | --- | --- |
| `keyboard_poll` | the poll site of the input loop | the decision happens only when the game asks for a key |
| BP chain on SS | the frame whose return is `UNIT_RET_IP` (`0302`) and whose child returns into resident `0709` (`0F2D`) | the input routine `0709:09E3` called from unit `0x19` at `+02FD` |
| unit frame bytes | selected index at `BP-2Ah`, attempt at `BP-2Bh` | the game's own selection (`0AF8:11C0`, argument 13) and attempt counter |
| `DS:0000`, 512 bytes | 13 records of 21 bytes at `2 + 21*i`: length byte `06`, six letters, 14 bytes of tail | the game's own record table (checked by digest, never printed) |
| BIOS head and tail | `0040:001A` and `0040:001C` | keyboard buffer must be empty before any key is sent |
| printer sites | the return offsets (unit 0x19) of the calls to `0709:0626` recorded since the last clearing poll (see hook item 2) | the six fixed-row calls sit at `+0157`, `+0174`, `+0191`, `+0202`, `+0286` and `+02D8` (5-byte `lcall`s), so the recorded return offsets are `015C`, `0179`, `0196`, `0207`, `028B` and `02DD`; these must all be present |

Any failed check is SILENCE with a reason. There is no confidence value and no OCR.

## Gate contract

- **One occurrence, one plan.** An occurrence is `(unit segment, unit BP, input frame BP, index, attempt)`.
  A plan is the letters of the record, then RETURN, sent one key at a time.
- **One key at a time.** A key is sent only when the buffer is empty. After a send the gate waits until the buffer
  tail moves (the key was buffered) and the head catches up (the game read it).
- **Guard break mid-plan aborts** the plan and never resends. A new plan needs a new occurrence.
- **Silence on anything else.** Non-prompt states, a second input frame, a changed table digest, an out-of-range
  index or attempt, missing rows, and a non-empty buffer all produce SILENCE or WAIT. Nothing is typed.
- **Assist off** returns SILENCE before any state is read, so the manual path is exactly as before.
- **No manual fallback in assisted play.** The reader never asks for manual input. Keys the player types are the
  player's own; the gate does not add, replace or retry them.

## Normal keyboard path (observed, except the host step)

Observed in native run `t579-codeword-u19-042` for the key `a`, on the pinned build:

1. The host key event is delivered as IRQ1. The ISR reads port 60 at `F000:E988`: value `1Eh` (make code).
   The first event after my break-in was a stale Alt release `B8h`, so the ISR order is real and must be respected.
2. `INT 15h AH=4Fh` intercept (callback `000D`) runs and returns.
3. The IRQ1 BIOS callback `002F` adds the word `1E61h` at `0040:001Eh`; the tail moves from `1Eh` to `20h`.
4. End of interrupt, `iret`.
5. The game's `INT 16h AH=00` read at `1264:031D` (callback `002E` at `F000:CFC1`) returns `AX=1E61h` at
   `F000:CFC5`; the caller at `1264:031F` tests `AL`.

**Not observed:** the host call `KEYBOARD_AddKey2` (`src/hardware/keyboard.cpp:1067` in `dosbox-x-hybrid-t24`) that
produces step 1. Its position is inferred from the source names only. The assisted path must use this same
keyboard path and must not write the buffer directly.

## Fixtures (synthetic only)

The fixture table is generated by the test module. Its words are letter patterns, not game words, and its digest is
`2ed55a87fe20e87954333be344e7d1f5197e322c9f7b856f91ef92998acb6059`.

| Index | Slot offset | Synthetic letters |
| ---: | ---: | --- |
| 0 | 2 | ADGJMP |
| 1 | 23 | HKNQTW |
| 2 | 44 | ORUXAD |
| 3 | 65 | VYBEHK |
| 4 | 86 | CFILOR |
| 5 | 107 | JMPSVY |
| 6 | 128 | QTWZCF |
| 7 | 149 | XADGJM |
| 8 | 170 | EHKNQT |
| 9 | 191 | LORUXA |
| 10 | 212 | SVYBEH |
| 11 | 233 | ZCFILO |
| 12 | 254 | GJMPSV |

The 21 reader tests cover every index, every refusal (non-poll, index 13 and 255, attempts 0 and 4, digest mismatch, no input
return, two input returns, wrong resident caller, missing fixed rows, missing table, malformed record), assist off,
one plan per occurrence, a new attempt as a new occurrence, waiting on a non-empty buffer, a guard break mid-plan
with no resend, a key that never buffers, the complete keyboard map, and the import boundary.

The runtime table digest the reader pins is `0486c788451ab11470af8d9bcf79a4ff4c677bca4fa479dddc96d2e2e8f27ac7`.
It is a digest of the game's 512-byte table and contains no words. It was identical in all eight runs observed
for this slice.

## Conformance

The conformance test runs the same scenarios through the Python reader and the C++ driver and requires identical
transcripts. It counts 136 `SEND` lines and 19 `completed` lines, which follow from the scenario design.

The **independent-rows** scenario uses six call-site literals typed in the C++ driver and the Python transcript
(`0x0157`, `0x0174`, `0x0191`, `0x0202`, `0x0286`, `0x02D8`, each plus five for the pushed return). They are not read from the
reader's `kFixedRows` / `FIXED_ROW_CALLS` table. The test checks that its plan is the synthetic record's letters then
RETURN. This closes the gap that let the earlier row-offset bug pass: that bug shared one constant on both sides.
Limit: the literals come from the same native observation as the reader constants, so they are independent of the
code constant, not of the observation.

## Emulator hook (built as a patch; run in a scratch copy; not merged)

1. **Snapshot point:** `INT16_Handler` (AH=00 and AH=01 only), the keyboard poll of the input loop. The snapshot is read
   there, `Gate.step` decides, and only SEND acts.
2. **Printer sites (exact rule):** at the entry of `0709:0626` the hook records the pushed return CS:IP, capped at 256
   entries with the oldest dropped (`kPrinterCap`, `pr_codeword_assist.h:34`). At each keyboard poll, after the gate
   decides, the list is cleared **only** when the decision reason is `no-unique-input-return`, `no-input-routine-child`
   or `input-routine-chain-mismatch` (`poll_hook`, `pr_codeword_assist.h:342`). Every other decision keeps the list,
   including a prompt occurrence refused for `fixed-rows-not-printed` or any other guard. The list is the only extra state
   the hook keeps.
   - **Row-survival decision (accepted risk; not a native result):** rows recorded before a refusal that does not clear the list
     can, in principle, survive into a later prompt occurrence, and the completeness check would then not prove that this
     occurrence printed the six rows. The decision accepts this risk on the static basis below. No native run exercised a
     surviving row, and no test covers it. The decision changes no code.
     - **Static basis, and what reproducing it needs:** the private overlay file `GAME.OVR` (supplied game media, under the ignored `resources/` tree, not committed). `find resources -iname GAME.OVR -type f | wc -l`, run in the checkout, gives 127 regular files (`find ... -type l` gives 0 symlinks; a depth-3 search gives 15). All 127 have the same size (206105 bytes) and the same full-file SHA-256, `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` (prefix `872964a3`), checked by hash on 2026-11-22. Unit `0x19` is the 862 bytes at file offset `0x09AC`, SHA-256 prefix `f93ae8c4`. Figures below were measured on 2026-11-22 with capstone 5.0.7 in 16-bit mode (`CS_MODE_16`), with unit offsets as addresses:
       - Linear sweep from `+0000`: 440 instructions, no undecodable bytes, 862/862 bytes tiled; return sites at `+0358` and `+035D`.
       - Recursive descent from `+00FD`, following fall-through, direct jumps and conditional jumps, and treating calls as returning (27 far calls, no near calls): 263 instructions covering 564 of 862 bytes, all within `+00FD` to `+0358` inclusive. No indirect jumps or calls, and no far jumps. The only return reached is `+0358`, so `+035D` is not reachable from the entry. No reached instruction lies below `+00FD`.
       - Dominance over that graph, with each conditional's both outcomes taken as possible and calls falling through: the six print sites (far calls at `+0157`, `+0174`, `+0191`, `+0202`, `+0286` and `+02D8`) each dominate the input call at `+02FD`. Removing any one of them makes `+02FD` unreachable from `+00FD`, so every path from `+00FD` to `+02FD` passes through all six.
       - Entry set. `+00FD` is taken as the entry. This is an assumption: no caller trace is cited here, and a word-table check finds no 16-bit word equal to `00FD` in the first 0xFD bytes, so the entry is not read from a table inside the extent. Measured from `+0000`, for comparison only: the descent covers all 253 preamble bytes by fall-through, and the conditional at `+00E7` targets `+015B`, past the first print call (`+0157`, five bytes). So from `+0000` the site at `+0157` does not dominate `+02FD`; the other five sites do. A dominance result from `+0000` that claims all six is not reproduced here.
       - Preamble. The 253 bytes below `+00FD` are not reached from the entry `+00FD`; the descent shows that nothing reachable from `+00FD` lies below it. Of those 253 bytes, 248 are printable ASCII and one is NUL (counted in this round), which suggests data rather than code. Decoding them from `+0000` as code is not evidence about the unit's behaviour.
     - Clearing on every non-SEND poll is not a safe alternative: the gate runs the row check on every poll, and a WAIT poll
       during a plan is non-SEND, so clearing there would abort a plan in progress.
     - Any narrower rule (for example, clear only on SILENT decisions other than plan-ending ones) is new design work. It needs
       its own conformance tests, a rebuild and a new demo before it could replace the accepted behaviour.
3. **Sending:** `KEYBOARD_AddKey` (the public entry; it dispatches to `KEYBOARD_AddKey2` in the controller mode the build
   uses) with a press and then a release for each key. The buffer is never written, and no host tool types. The log
   records each SEND but not which dispatch branch ran; the branch is inferred from the source, not observed.
4. **Off switch:** the environment variable `POOLRAD_CODEWORD_ASSIST` must be exactly `1` for the hook to act. Unset or any
   other value is off, and no state is read. `POOLRAD_CODEWORD_ASSIST_LOG` names an optional decision log.
5. **Build cost (measured, scratch copy of the checkout, 8 cores).** Every build attempt is listed, including failures.
   Builds 1 to 4 used the earlier dirty base, so they are not reproducible from `b6abbd5`.

   | Build (log) | Wall | Result |
   | --- | ---: | --- |
   | build-1 (dirty base) | 17 min 34 s | core compiled; failed at the `bios_keyboard` include of the header (fixed with the `cpu/` path) |
   | build-2 (dirty base) | 12.8 s | failed: `mem_readb_inline` not declared in the `bios_keyboard` unit (`paging.h` added to the glue) |
   | build-3 (dirty base) | 19 min 04 s | success; still had the row-offset bug |
   | build-4 (dirty base) | 24 min 16 s (16 min 19 s CPU) | success; row-offset fix |
   | pristine full (`b6abbd5` export + patch) | 37 min 51 s (84 min 06 s user, 5 min 16 s sys) | success; 543 objects, 0 errors; binary `502ffe33…` |

   The bios_keyboard object alone takes seconds; `core_normal.o` dominates an incremental rebuild.

## Removal (recorded separately, when it happens)

1. Turn the flag off (no state read, no key sent).
2. Remove the hook registration in the DOSBox-X patch and rebuild.
3. Delete `tools/hybrid/codeword_assist.py`, `tests/hybrid/test_codeword_assist.py` and this note's section
   on the hook.

Removing the assist must not change any manual-path output. That is checked by running the manual path with the
flag off before and after removal.

## Native coverage of the selected index (read-only cold starts)

Cold starts with no key input, one break-in each, index read from the unit frame byte `BP-2Ah`. The unit
segment was `38D3` in every valid run, and the record table digest was identical in every run that dumped it
(`0486c788451ab114…`, 38 of 38).

| Index | Observed (run count) | Source |
| ---: | ---: | --- |
| 0 | 2 | coverage 003, 037 |
| 1 | 2 | coverage 009, 016 |
| 2 | 2 | coverage 001, 022 |
| 3 | 2 | coverage 035, run 046 |
| 4 | 3 | coverage 004, 026, 032 |
| 5 | 2 | run 047, coverage 018 |
| 6 | 1 | coverage 027 |
| 7 | 4 | coverage 011, 012, 024, 031 |
| 8 | 7 | coverage 002, 007, 010, 025, 030, 033, 036 |
| 9 | 4 | run 043, coverage 006, 034, 039 |
| 10 | 2 | coverage 028, 029 |
| 11 | 6 | runs 045, 049; coverage 008, 015, 020, 023 |
| 12 | 7 | runs 042, 044, 048; coverage 005, 019, 021, 038 |

Totals: 44 valid cold starts (8 from runs 042 to 049, 36 from the coverage loop). **All 13 indices are observed at
least once.** Index 6 has a single observation. Four coverage runs (013, 014, 017, 040) gave no input-return frame at
the break-in, so they are excluded; their cause is not established here.

What this does **not** establish: the observation is of the selected index byte only. It does not show that each
index's stored word is the accepted word. The only typed answers tested were a wrong answer (index 12) and the stored
word for index 11, and both reached the same menu. Per-index acceptance is not device-proven for any index.
(The table is a count of observations, not a claim about acceptance.)

## Emulator runs (this slice, scratch builds, not parity evidence)

Binary identity: runs 052 to 056 record sha16 `502ffe33da0439d2` in their action logs (the pristine build). Runs 050
and 051 record no sha; they are tied to builds by launch time only (launch 20:50 matches the build-3 finish; launches at 21:19 and 21:22
follow the build-4 finish at 21:19). Screenshots are cited by file name; the screenshot hash is the first 16 hex digits of the PNG's SHA-256.

- **`t579-assist-on-050`, pre-fix (build-3 by launch time):** `on-150.png` shows the prompt with no SEND. The refusal reason
  `fixed-rows-not-printed` is inferred from the commit message of `056d035f` (row-offset correction), not from a log: the
  directory holds only the post-fix `assist.log`, so no decision log of the pre-fix launch is retained. This is the pre-fix record only.
- **`t579-assist-on-050`, post-fix (build-4 by launch time):** `on2-150.png` and `assist.log` show one silent poll
  (`no-unique-input-return`), exactly one plan (six letters and RETURN, seven SEND lines), a `completed` line, and then the
  main menu. The one-plan claim cites these files only.
- **`t579-assist-off-051`, assist unset (build-4 by launch time, dirty base):** no decision log; the prompt stays through 150 s
  (`off-150.png`). An xdotool answer with RETURN advanced the screen (`off-manual.png`). That screenshot's full SHA-256 (`0e42ba8f630eb448f601d2733f2f72baba6bca2c6c7d11a5c2ad9219fb3c8690`) is identical to the main-menu screenshot `t579-assist-off-056/off-manual2.png`, which was viewed, so the bytes are that menu. This is a hash match, not a new view.
- **`t579-assist-on-052`, assist on (pristine, recorded):** stalled on the credits page (duration not verified: the run directory has a launch time only, so the earlier figure of about 7 minutes is not measured) (`on-150.png`,
  `on-poll.png`); killed. The cause is not established. It was not reproduced in 054 or 055.
- **`t579-assist-off-053`, assist off (pristine, recorded):** the prompt is still shown at 180 s (`off-180.png`); no decision log.
- **`t579-assist-on-054`, assist on (pristine, recorded):** `on-60.png` and `on-180.png` show the main menu (screenshot hash
  `0e42ba8f630eb448`); `assist.log` holds one plan (six letters and RETURN) and then silence.
- **`t579-assist-on-055`, assist on (pristine, recorded):** the same result as 054: the main menu at 60 s and 180 s, one plan in `assist.log`.
- **`t579-assist-off-056`, assist off (pristine, recorded):** the prompt is in `off-prompt.png` (screenshot hash
  `a8f5ada7ecf76010`). The first manual send (xdotool to the root display) left the screen at that prompt (`off-manual.png`).
  After a second send to window `0x60000a`, `off-manual2.png` shows the main menu (hash `0e42ba8f630eb448`), which was viewed.
  The attribution to the second send rests on the sequence, not on a repeated control run.

The send log holds the record letters. It is written only into the ignored run directory and is not committed.

## Limits (not claims)

- The reader is unit-tested on synthetic fixtures (22 tests, with the C++ transcript conformance). The emulator hook has
  run in a scratch build, in the runs listed above; those runs are not parity evidence.
- The keyboard chain is observed for one key on one run. The host step is not observed.
- The game accepted both a wrong answer and the stored word for the same prompt; the accepted word is therefore not
  distinguishable at the unit boundary. The assist types the stored word, which is the game's own record, not a
  verified answer.
- Retry and quit paths were not reached natively.
- No parity, function identity, function count or credit follows from any of this. ASSIST labels every output.
- Pull requests remain behind REVIEW_REQUIRED. Nothing here is approved, merged or self-approved.

## Limits (emulator hook)

- Two post-fix assisted runs on the pristine build (054 and 055) each produced one plan. Run 052 stalled and was killed.
  One post-fix run (050) used the earlier build. No attempt-two, no retry, and no quit path was exercised by the hook.
- The decision log records SEND and reason changes, not every poll. Silence is shown by the absence of SEND and by the
  screen, not by a per-poll count.
- The game accepts any typed answer at this prompt (both wrong and stored words reached the menu). The hook therefore
  demonstrates the mechanism, not a validated answer.
- Printer-row clearing: the row-survival decision (accepted risk; hook item 2) rests on the static basis stated there, not on a native run. No run exercised a non-clearing refusal followed by a prompt.
- Manual keys typed during an assisted plan are not suppressed or detected; the gate waits for the buffer and would
  interleave them. This is not covered by any test.
- The hook is built from `b6abbd5` (pristine export plus the patch). It is not built from the pinned `5a9c7c53` source,
  and this note does not check whether the native captures used the same build.
