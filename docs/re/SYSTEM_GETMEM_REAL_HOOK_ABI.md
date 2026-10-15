# Actual DS:[4414] allocator-hook ABI and first unjoined callback context

The saved-A allocator's actual installed hook is **083A:0025**, a loaded
far-JMP thunk to **398A:0042**. Two explicitly controlled native cases
measure the real hook pointer, original far-call frames, zero-argument
return and nonzero prefix. The new C adapter matches those instructions
and stops at wrapper +0248 or callback +005F before SYSTEM B0B.
No naturally reached hook, nonzero hook return, retry/error outcome,
continuous allocator route or full-core/count credit is claimed.

This branch starts at main `7d319e2b7a2d73b713ade29069693d9de581d848`,
separate from [PR #474](https://github.com/huntergdavis/poolrad-decomp/pull/474),
which was still under review when this task began. It requires no #474
source changes. Bounded `deja "Poolrad SYSTEM DS 4414 allocator far hook
03B7 03C3 AL retry error 03C7"` expired without reusable output before
implementation. Reused leads are [the #465 frame audit](SYSTEM_GETMEM_23F_NATIVE_ABI.md),
the #474 selection measurements, and [existing SYSTEM B0B evidence](SLOT_A_NATIVE_U6F_1965_SYSTEM_COPY.md).
Original code, actual pointer and new callback frames are independently pinned.

Before review the branch is restacked onto main
`98a4409516bea5720fa037c5c199e909e5500d3e` after #488. Tested source,
checks and reused headers/oracle helpers retain their recorded hashes;
the focused audit is rerun on the final restacked head.

## Original and loaded identity

| Input/span | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| SAVGAMA.DAT | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original callback +0042..00AB at GAME file +004A | 106 | `af7dd1cc3545d9c6e1b0cae16887cc5c47d2568912840a1fa7543c288ef38875` |
| Loaded callback 398A:0042..00AB | 106 | `b8e2acf3e162161244199147a586b0ed81882a0a30d2de4261a42acf5ef7c0f3` |
| Loaded thunk 083A:0025..0029 | 5 | `a5378317872433ef1fc55344812aa7de6aeb930049cac07533a62dc14c044e79` |
| Complete raw 36-row transcript | — | `63bbc99da636f5ea4ad2cf593937ee76c26a28d203eb7a51a3e2d189dc48113b` |

The callback's six far-call segment words at +0062, +0067, +0073,
+0091, +0096 and +00A1 relocate by **+0824**. No other callback byte
differs. The audit also pins SYSTEM +0308..03D2 and +0491..04A1 against
START and every loaded RAM image using the #465 hashes. The thunk is
pinned in its already loaded far-JMP state; no identity with its earlier
overlay-loader state is assumed.

## Native cases and real frames

Ignored root: `resources/oracle-runs/slot-a-getmem-real-hook-abi-20261014`.
All 36 rows pin sixteen CPU fields and all 655360 RAM bytes. The first
row is untouched saved-A +0308 after the confirmed +19B5 call, with
AX=011D, DS=14A0, SS:SP=1D16:37AE and BP=37B0. The real installed
DS:[4414] pointer stays 083A:0025 in every capture.

The other 35 rows are controlled inputs/execution. Growth redirects only
the free-list head to the measured heap end 6652:0008; two RAM bytes
change and all CPU fields stay identical. Original core execution then
reaches +03B7. Exhaustion independently restores the incoming core CPU,
sets end/head/limit to 6652:0008 and runs the original core to +03C3.
All heap/register edits precede original execution. No returned AX/AL or
hook outcome is written into a running replay.

At growth +03B7, SP=37A8 and the hook argument is zero. The actual far
frame at thunk entry SS:37A4 begins `03BB, 131C, 0000, 0008, 6652, 0248`.
The thunk changes only CS:IP. PUSH BP and MOV BP,SP establish BP=37A2;
SUB SP,28 reserves locals. CMP [BP+6],0 / JBE selects +00A3.

The zero-argument branch reads **AX=196A (AL=6A)** from **SS:37A0**,
the uninitialized local `[BP-2]`. That word is 196A in the untouched
baseline and remains unchanged throughout the measured growth case.
MOV SP,BP / POP BP / RETF 2 return to +03BB with SP=37AA and BP=37B0.
Actual POP AX / POP DX then restore **DX:AX=6652:0008** from the
allocator's own stack; CLC / RET stop at +0248 with SP=37B0.
The whole new growth fragment changes six RAM bytes.

At exhaustion +03C3, SP=37AC and the argument is 011D. Thunk-entry
SS:37A8 begins `03C7, 131C, 011D, 0248`. The callback establishes
BP=37A6, clears its local byte, prepares arguments and stops before
+005F. The measured argument stack is **SS:3774**,
containing `0000, 398A, 3786, 1D16, 000E` (source pointer, local
destination pointer and retained word). The source Pascal length is 28;
no private string contents are published. This fragment changes seventeen
RAM bytes. The native run closes before the SYSTEM B0B call executes.

## Parity, gates and reproduction

The new fragments execute 15 growth and 16 exhaustion instructions:
**31 C steps with 31 post-input native full-state checkpoints**. Including
independent synthetic inputs, the complete audit passes **129 full CPU/RAM
C-versus-original-byte instruction comparisons**. Synthetic cases check
incoming AH, zero/FFFF argument boundaries, stale flags, wrapped SP,
far-call pointer/frame aliasing, SUB/CMP flags and single-instruction reads
of the local word. Those inputs establish operand semantics and add no
native hook-outcome or saved-game reachability claim.

Two original-only core executions (38 growth and 30 exhaustion instructions)
match their captured native pre-hook endpoints. These 68 instructions are
excluded from C totals and never bridge a blocked C state to a later snapshot.
Strict C11/Wconversion, media-free tests and ASan/UBSan pass, including the
complete differential audit. The runner enforces 100 instructions/ten
seconds per case and 120 seconds plus five-second kill grace per process.
It accepts private inputs only inside this clone's ignored `resources/`.

SYSTEM B0B already has a generic C adapter in `slot_a_saved_copy_1965`.
The new hook adapter gates its unjoined callback-context call at +005F;
this capture does not measure that copy's actual native entry/return or
local writes. The next slice can bind that existing helper and stop at
callback +0064 before the printer call. Printer/input/error effects remain
outside this change.

SYSTEM +03C7 remains blocked for all 256 AL values in strict tests. Original
bytes supply static leads: AL=0 branches to +0382, AL=1 to +0383, and
AL>1 reloads DS:[8754] and retries at +030F. These are unmeasured post-hook
leads, with no C implementation or fabricated native outcome in this slice.
The measured zero-argument return resumes +03BB and consumes its own real
allocated-pointer frame.

Run `bash tests/replay/check_system_getmem_real_hook_abi.sh` without media.
With pinned captures and Unicorn 2.1.4, run
`POOLRAD_UNICORN_PYTHON=/path/to/python bash tests/replay/check_system_getmem_real_hook_abi.sh resources/oracle-runs/slot-a-getmem-real-hook-abi-20261014`.
Keep Python assertions enabled. Native startup uses the documented
[title/prompt/menu sequence](SLOT_A_AFTER_ROW9_FULL_PAGE_SEQUENCE.md):
Return at the title, Return at a visible codeword prompt, then `l` and `a`.
Arm caller +19B5 before core +0308 to exclude earlier allocations. Take the
untouched baseline before applying the explicitly controlled heap inputs.

## Adversarial review and disjoint follow-ons

- target_function: actual DS:[4414] hook, loaded 083A:0025 -> 398A:0042,
  zero-argument return and nonzero prefix through explicit gates.
- source_candidates: `system_getmem_real_hook_abi.c` addressed adapter.
- signals: original/loaded identity; real far/near frames; unchanged local
  return word; exact native stack/local writers; full-state differential
  checks and all-AL blocking tests.
- score: 5/5 checked categories above, ordinal evidence coverage rather
  than a probability or complete allocator score.
- verdict: match for the measured bounded ABI fragments; nonzero return,
  full hook semantics and whole-core parity remain unclear.
- review_notes: controlled cases stay labeled; zero-argument AX is read
  from RAM rather than treated as a status. No source-symbol/count credit,
  synthetic AL outcome, continuous replay bridge or caller +19BA edit.

Follow-on A is the real B0B callback copy context through the +0064 printer
gate, with later nonzero return/retry/error effects still requiring native
evidence. Follow-on B remains caller +19BA pointer stores/setup through
+19D7's near CALL +0AEC, stopping before the loader. Runtime/callback and
caller files remain disjoint. Prior gates, immutable #410 map, CMake and
key/printer lane files are untouched; six new uniquely named files only.
