# U3C6 +071A: complete slot-step range caller

Author candidate; independent verification and credit remain with the reviewer.
This adds the complete 186-byte / 79-instruction public entry 9, including its
three callees on shared CPU and 1 MiB RAM. No scalar callbacks stand in for a
callee. The historical queue ranked this target first but marked it blocked by
+06E9; PR #919 supplies that dependency. Queue readiness still needs the profile
qualification below.

## Identity and provenance

Owner-supplied START.EXE is 65,231 bytes, SHA256
 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd;
GAME.OVR is 206,105 bytes, SHA256
 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit 03C6 starts at GAME.OVR file offset 198458, public stub 03E0:004D.
The caller occupies [071A,07D4), file [200276,200462), RETF 6 at +07D1.

| Body | Bytes | Instructions | SHA256 |
| --- | ---: | ---: | --- |
| +071A caller | 186 | 79 | 97c632304df8ce07a32c07045300b149df14dbd48c88d52a96c51f61dfbb9b2c |
| +0000 pair | 104 | 41 | 05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207 |
| +06E9 range | 49 | 18 | 1495986874fc88753b5419d14380e9f7846b537a2b7015c57f7e24a1a761a7b9 |
| +0C5F lookup | 90 | 32 | c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41 |

The closed chain has 170 instruction starts. Existing +0000 and +06E9 public
raw CPU APIs are called directly. The folded +0C5F implementation and local
arithmetic helpers are adapted from overlay_u5e_direction_263d.c (PR #860),
which cites the earlier overlay_u3c6_1298_cpu.c recovery. The body itself is
newly recovered from the pinned original. Prior-session recall was attempted
with deja "U3C6 071A full CPU composition 06E9"; it returned no matches and
searched zero sessions. No prior-session result is claimed as evidence.

The audit pins the three PUSH-CS/near-call sites +072B -> +0C5F,
+0772 -> +0000 and +07A4 -> +06E9. It also verifies six aligned incoming near
calls (+027A, +0345, +0A39, +0B15, +0BA7, +0E9D), their PUSH CS and OR AL,AL
neighbors, and three incoming far fixups: U5E:+18BB/+1AD3 and U86:+0E4C.
All four callee/caller extents contain no relocation or unresolved call.

## Behavior and contract

At entry, SS:SP contains the far return pair. The low byte at SP+4 selects the
mode; words at SP+6 and SP+8 identify an exact offset/segment pair in the slot
table. The original +0C5F search includes the zero-count 256-slot traversal.
A zero direction byte returns false. Otherwise the caller tries four table
pairs, adds signed byte coordinates, and uses +06E9 to test each result against
0..6. Mode zero stops at the first in-range result; nonzero stops at the first
out-of-range result. Negative first table bytes skip the range predicate. If
all four are skipped, the initial true result survives. These details are
preserved rather than replaced by a simplified geometric predicate.

All argument, table, pointer and local reads are live. DS/SS aliases can change
a direction between its first check and its later load. The audit exercises
that case: SS=DS and SP=5F32 makes loop-index initialization zero the direction
byte, reaching +0000's otherwise surprising zero-direction path.

The API checks null arguments, exact 1 MiB size, entry IP, and conservative
stack/code overlap before mutation. CPU storage must be outside RAM; original
code is immutable. The overlap screen covers the deepest 30-byte frame and
word-extension bytes. A caller budget counts +0C5F and +0753 loop heads;
LIMIT retains all earlier effects and identifies the unexecuted head. It is
not a resumable continuation. Unexpected callee returns preserve CPU/RAM and
return TRANSFER. Final return consumes the actual live frame and six argument
bytes. Every normal exit ends with defined CMP flags; no FLAGS mask is needed.

### Profile boundary that affects composition

Word accesses use contiguous physical bytes after 16-bit effective-address
calculation, A20 off. RETF pops IP and CS as separate wrapped SS words. The
+0000 API uses a contiguous four-byte LES operand. At operand offsets
FFFD..FFFF, the pinned DOSBox-X profile documented in
[U3C6 +0CB9](U3C6_0CB9_SCAN.md) reads the segment word differently. This candidate
inherits the public +0000 profile explicitly; it does not claim native parity
for those LES operands. Its seam vectors are Unicorn evidence only.

Unicorn 2.1.4 reads RETF CS contiguously at SP=FFFE/FFFF. The audit executes the
original RETF, then explicitly substitutes the wrapped CS for those returns
and reports every affected case and substitution. Those are hybrid comparisons,
not exact unmodified-Unicorn CPU matches. Native full-chain replay establishes
the wrapped entry returns separately. Native LES segment seams remain outside
this evidence domain. Keep this qualification in queue readiness and any
future composition; a generic raw-CPU label alone does not resolve it.

## Reproduction and evidence

The completed audit passes 1,499 cases: 1,481 exact Unicorn comparisons and
18 hybrid wrapped-return comparisons (42 explicit CS substitutions). It covers
170/170 instruction starts and 34/34 conditional outcomes, with seven budget
stops and 18 contiguous-LES seam cases reported separately. It compares all
1,048,576 RAM bytes and all 14 CPU words,
including FLAGS, against the real four-function instruction chain. Fixtures
cover coordinate and signed-byte boundaries, modes, four-step paths, zero and
255-slot searches, unequal pointer words, low/high stack seams, A20 wrap,
DS/SS aliases, seeded table cases, and budget stops. Conditional edge and
instruction coverage are asserted, not inferred from case counts.

Native replay executes all original function bodies and their real calls,
using synthetic entry frames with separate loaded code, data and stack
segments. It compares 14 CPU words and 240 stack/physical-extension bytes in
26 cases; ten enter with SP=FFFE/FFFF. Four additional cases wrap a callee
return: two +0C5F returns at entry SP=0010/0011 and two +0000 returns at
entry SP=0018/0019. No FLAGS bits are masked. It does not
measure natural game invocation, the overlay loader, or LES segment seams.
Each capture digest is retained in the local native report.

Run with owner media available at ignored resources/DOS/POOLRAD:

    python3 tools/overlay/u3c6_step_071a_audit.py --output resources/u3c6-071a/original.json
    python3 tools/overlay/u3c6_step_071a_native.py
    cmake -S . -B resources/u3c6-071a/build
    cmake --build resources/u3c6-071a/build --target poolrad_overlay_u3c6_step_071a_tests
    ctest --test-dir resources/u3c6-071a/build -R poolrad_overlay_u3c6_step_071a_tests --output-on-failure

The focused C test covers mode exits, negative-pair skipping, frame cleanup,
budget stops and atomic argument rejection; it also runs under ASan/UBSan with
strict C11 warning checks. Original bytes, disassembly, native programs and
captures stay under ignored resources. Acceptance ledgers are unchanged.

## Author parity assessment

- target_function: OVL_03C6_071A.
- source_candidates: overlay_u3c6_step_071a.c, composed public +0000/+06E9.
- signals: pinned source identity, complete call graph, aligned incoming ABI,
  live-memory alias behavior, CPU/RAM oracle, and native original-chain replay.
- score: strong evidence within the stated profile; incomplete native seam domain.
- verdict: likely_match, pending independent review.
- review_notes: do not treat hybrid RETF cases as exact Unicorn matches or
  contiguous LES seam cases as native parity. No author credit or gameplay
  reachability claim accompanies this change.

## Checker registration (2026-10-30)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven in a worktree WITHOUT `resources/` (what CI has) with

```sh
python3 tools/overlay/check_checker_builds.py OVL_03C6_071A --require-current-main
```
