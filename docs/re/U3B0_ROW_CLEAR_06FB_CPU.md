# U3B0 +06FB raw row-clear caller

This adds the missing raw CPU body for U3B0 entry 3, composing the unchanged
resident space renderer at 0709:05EB, glyph renderer at 0709:01DF, and SYSTEM
UpCase at 0AF8:182A. It requests 40 space glyphs at x=0, y=24, color=0.
AH is preserved in all four argument words. No callback substitutes for a
callee. No classification, acceptance, accounting or credit change is included.

This is a callable-body addition only. Existing high-level gameplay/ECL
callbacks are not migrated or connected to this API by this change. It does
not establish a playable route or live entry-to-display integration.

## Selection and ownership

Selected from the current schema successor of the schema-4 queue, generated
with the pinned inventory/callgraph/near-call inputs and current main using
--no-git. U3B0 +06FB is rank 164, score 50, a 22-byte composable caller with
19 fixup-backed callers and one resident dependency. A strict/readiness label
alone does not prove a raw ABI; the merged 05EB source/header were inspected.

VP approval i18697 covers this caller only and prohibits self-credit.
The checked-in ledger/classification remains
game_or_other_library_unresolved/unresolved. The approval used
game_evidenced/provisional wording; that discrepancy is reported for the
independent reviewer to resolve, without author-side metadata edits.

A case-insensitive 06FB/1787 scan over every main-tree recovered source/header
found no U3B0 caller implementation. Other matches are U25's classifier and
unrelated instruction sites. Existing U3B0 space-scan bodies are different
entries; this PR introduces one source/header pair, not a duplicate of them.
The VP separately checked remote ownership. The ignored receipt retains
the source scan, exact base and selected queue row.

Dunking Sheep recall: query "poolrad U3B0 06FB space 0709 05EB ownership".
Historical ownership clearances were reused from Codex sessions
01a12b3e-db20-70e1-95f9-d3fe44b8d309 and
01a1386e-bea7-7883-8ddd-06fd2be8db9b; they are coordination context, not
execution evidence. Reused code is the unchanged merged
resident_0709_space_05eb_cpu and its glyph/SYSTEM/bus dependencies.
The original/native audit mechanics extend the 05EB harness.

## Original identity

Pinned START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Pinned GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Body | Half-open span | Bytes | Instructions | SHA-256 |
| --- | --- | ---: | ---: | --- |
| U3B0 entry 3 | 06FB–0711, GAME file offset 184266 | 22 | 13 | b306a980ebf183c4dd57a62414c019d47d68787e6f6577d61da8bec63e9fc09b |
| Space renderer | 0709:05EB–0626 | 59 | 27 | a41af7187ac3e0bf5a09311bad9fae220c498f0f9aad9492438661381d4b1171 |
| Glyph renderer | 0709:01DF–05EB | 1036 | 403 | a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840 |
| UpCase | 0AF8:182A–183D | 19 | 8 | 738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c |

Complete static closure: 1,136 bytes / 451 instructions across four bodies.
CALL at 070A targets resident 0709:05EB, returns at 070F; caller RETF0 is
0710. The resident near call at 061F targets 01DF after PUSH CS; glyph
far-calls UpCase at 0207. Media and extracted bodies remain ignored.

## Raw contract

Entry IP=06FB, exactly 1 MiB A20-off RAM, attached EGA model,
SP=003A..FFFC, separate CPU/bus/EGA/diagnostic host objects outside RAM.
DS and SS full segment windows exclude EGA; code and bindings are immutable
and outside EGA, writes avoid code. All reached far-return frames fit
SS:0000..FFFF. Other data/stack/font aliases remain live. Word operands use
physically consecutive bytes with 20-bit wrap; effective offsets wrap at
16 bits. No arbitrary-PC resume, interrupts, faults or self-modification.

The wrapper saves BP, pushes x0/y24/color0/count40, retains all caller
FLAGS before the call, and guards CALL opcode/offset before pushing its
return frame. The live segment operand selects the loaded resident body.
Diagnostics retain the space status and both nested diagnostic levels.
Any nonzero callee status preserves partial CPU/RAM/EGA effects. A successful
callee returning to a different CS:IP produces TRANSFER with that state.
Invalid host arguments reject before CPU/RAM/EGA mutation and reset
diagnostics to -1; a null diagnostic is supported.

The existing renderer owns the loop budget, OUT refusal semantics,
undefined-FLAGS conventions and hardware restrictions. This PR adds no
FLAGS normalization. Port hooks may observe/refuse without state mutation.
Hardware scope is EGA; mode0/2 ordinary-memory observations do not establish
CGA/Tandy closure.

## Validation and reproduction

Run with a Python environment containing Capstone and Unicorn:

    python tools/overlay/u3b0_row_clear_06fb_cpu_audit.py --unicorn --output resources/original.json
    python tools/overlay/u3b0_row_clear_06fb_cpu_audit.py --native --output resources/native.json

The original-byte audit executes all four bodies, compares CPU14 unmasked,
all 983,040 ordinary RAM bytes outside the EGA aperture, ordered OUT events
including CS:IP, and all VRAM write operands. Unicorn is not an EGA plane
oracle. Case-specific guard/budget/refusal hooks stop immediately before the
corresponding instruction; no function effects are substituted.

The native harness uses DOSBox-X machine=ega, video mode0Dh, core=normal,
cputype=386. It loads caller/resident/SYSTEM at 4000/8800/6000 and patches only
the two far-call segment bindings for return cases. It compares CPU14
unmasked, a 1,024-byte SS window, full 64 KiB DS and font windows, and all
four 64 KiB EGA planes. Selected pre-call/pre-OUT cases use a flags-neutral
far jump at the stopped instruction; normal cases are uninstrumented.
No native OUT trace, budget-stop, A20, wrapped-frame or gameplay claim.

The C semantic test independently checks the requested row is cleared in
all four planes on both pages and every other plane byte is unchanged.
It also checks AH-bearing stack words, cleanup, nested diagnostics, code
guards, OUT refusal, loop budget, a saved-BP/video-mode alias, and atomic
host rejection. The sanitizer run and focused CTest include the unchanged
space and glyph tests. Exact results and pins are recorded below.

## Parity review

target_function: U3B0 public entry3 [06FB,0711).
source_candidates: overlay_u3b0_row_clear_06fb_cpu.c, composed with the
unchanged space/glyph/SYSTEM sources.
signals: pinned body hash and entry table; 13-instruction straight-line
shape; x/y/color/count constants; AH-preserving pushes; exact call/return
frame; original CPU/RAM/OUT/VRAM comparisons; independent native EGA
planes and synthetic whole-plane checks.
score: seven independent signal categories; scoped match, not an identity
or universal-hardware probability.
verdict: match within the documented entry, memory, device and stop profile.
review_notes: nested dependency paths are not all reachable from this fixed
caller; static closure size is not executed coverage. Unexpected successful
return handling is defensive unless listed as a measured stop. Existing
callbacks remain integration work, and unresolved game/library identity is
left to independent review.

## Measured result

- Original-byte differential: 63 cases; stop statuses
  {'0': 29, '2': 2, '3': 32} (0=returned, 2=caller code guard, 3=nested renderer stop).
  All 13/13 caller instruction starts executed; the caller has no conditional
  branch. The closure reached 411/451 starts; the ignored JSON
  explicitly lists the other 40 dependency starts.
  Compared 36,492 OUT events and 66,124
  VRAM write operands, plus CPU14 and 983,040 ordinary RAM bytes per case,
  with no FLAGS masks.
- Native EGA: 20 cases, 14 returns, three pre-call stops and three pre-OUT
  stops. CPU14 and 394,240 memory/plane bytes per case, unmasked.
  RESULT.BIN SHA-256:
  7c972791edc33446f23c6e5dc7a9d500c522f5e7d28f6157a8d531d1bed24556.
- Strict C11 warnings-as-errors with ASan/UBSan: 4,194,459 checks passed.
- Focused CTest: 3/3 passed, including unchanged space and glyph tests.

Execution was measured on base 6fe1e713a48342dcd4e215a0514c195d49326440.
Integrated main 07e0ff436325853a92794893b6d298d894bf87ea, which adds the
independent dice caller from PR #1114; all execution dependency hashes are
unchanged. Focused CTest was rebuilt and passed on that integration base.
Ignored resources/evidence.json pins public inputs, unchanged dependencies,
tool versions, media, native input/output, original reports and sanitizer/
CTest logs. Native input is independently reconstructed before receipt
creation. Independent review and credit remain with the VP/reviewer.


## Manifest declaration and checker handoff

The declaration of `OVL_03B0_06FB` as `full` names this already implemented
raw entry under the memory/device/stop profile above. It does not assert
full feasible-path coverage of the renderer closure: the measured caller
has 13/13 starts and no conditional outcomes, while its dependencies reach
411/451 starts. It does not turn the 983,040-byte ordinary-RAM comparison
plus VRAM operands into a full 1 MiB flat-RAM comparison. Native separately
corroborates EGA planes and the stated capture windows. No masking, new
parity measurement, independent credit, or gameplay integration is implied
by the manifest word. Readiness and game/library identity are separate:
identity remains `game_or_other_library_unresolved`, as recorded when
PR #1116 accepted the body without credit. Declaring composition readiness
does not resolve that classification or grant strict credit.

The actual audit now registers this row and supports a media-free
`--compile-only` mode. It uses the same source list, compiler command,
shared-library load and symbol binding as the original/native audit:

    python tools/overlay/check_checker_builds.py OVL_03B0_06FB --require-current-main --output resources/checker-builds.json

This declaration follows VP instruction i19236. Dunking Sheep context
recalled Codex session 01a1386e-bea7-7883-8ddd-06fd2be8db9b's public entry
mapping (3C5:002F -> U3B0 entry 3), also checked by the existing original-byte
checker; that recalled mapping is coordination context, not new parity.
The original evidence and implementation above remain the authority for
the scoped declaration; acceptance and any credit belong to the reviewer.

Declaration validation on main `940ffc70`: the full existing audit reran
63 original-byte cases and 20 native EGA captures with the same counts,
coverage and native RESULT.BIN hash reported above. The current-main gate
compiled, loaded and bound all nine registered checkers for nine rows.
The declaration changes no C body, header, classification or accounting.
