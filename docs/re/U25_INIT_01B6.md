# U25 +01B6: full-state script initializer

Author evidence for OVL_0025_01B6; independent verification and credit are
separate. The new entry is pr_u25_script_init_01b6, in
[overlay_u25_init_01b6.h](../../include/poolrad/recovered/overlay_u25_init_01b6.h)
and its corresponding C source. It replaces no existing semantic API and
changes no classifier, acceptance record or credit ledger.

## Identity and closed chain

Owner-supplied START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR SHA-256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

U25 public entry 3 is +01B6, runtime stub +002F in segment 002B.
The callable body is [01B6,0322), file [07689,077F5), **364 bytes and
135 instructions**, ending in RETF. Its SHA-256 is
b1bf2753d995d35feb5b4586b09a6dd9c6231183220062f790ef5ff4501b4eb0.
The schema-8 queue used for selection reports 395 bytes / 394 decoded bytes /
147 instructions, extending to the next public entry +0341. The bytes after
+0321 RETF are outside this callable-body claim. This counting discrepancy
was relayed to the VP. The old semantic initializer documentation already
records 364 bytes / 135 instructions.

The caller has two fixup-backed SYSTEM FillChar calls, +01EB and +021C;
only their segment words (+01EE/+021F) are relocated. Five PUSH CS / near CALL
pairs at +0251/+0267/+027D/+0293/+02A9 invoke the full U25 +0000 loader;
five more at +025D/+0273/+0289/+029F/+02B5 invoke +06AF. Real stack arguments,
far-return frames, all fourteen CPU words, and the same RAM pass through
these calls. No result-only callback supplies an operand or runtime effect.

The closed chain contains 2,003 instruction bytes and 771 instruction sites.
The loader includes +1153/+12FB, local +06D3/+06FB, and SYSTEM
+0B0B/+0B25/+0B98/+0C3B. The source auditor reuses the accepted hash checks in
u25_loader_0000_audit.py and u25_string_cores_audit.py, checks the
initializer entry and both relocation slots, and hashes FillChar
[16A8,16BC) (20 bytes / 7 instructions):
f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7.

Implementation reuse is explicit: the arithmetic/memory primitives and the
36-byte +06AF instruction translation come from
overlay_u25_loader_0000.c; the complete public loader and string APIs
remain linked dependencies. FillChar uses pr_system_fillchar_16a8 with
field-wise CPU copies. See [U25_LOADER_0000.md](U25_LOADER_0000.md) and
[U25_STRING_CORES.md](U25_STRING_CORES.md) for the inherited contracts.
Recall was attempted before implementation; indexing stalled and the health
probe timed out. The deja-health repair command started a background
rebuild. No prior-session recall findings were reused.

## Behavior

The initializer writes the original DS flags, clears two and six bytes using
real FillChar, sets the script PC to 9900, and updates live window pointers.
It loads five operands and writes their converted words to DS:49E3..49EB.
The loader's extra PC advance at each return is retained, including all
string-tag side effects and writes to shared temporary buffers.

After setting the window-0 state word to one, it reads the live restored byte
DS:49F8. A nonzero byte is cleared and both clear loops are skipped. Otherwise
it clears 32 window-0 words and ten window-1 words, using the original odd
stack-local offset BP-3, 16-bit arithmetic, repeated LES loads, and live loop
counter comparisons. Aliasing can change pointers, the restored decision,
loop counters, output slots, saved registers, or return frames. None of those
values is snapshotted into a semantic host structure.

## API boundary

Enter after far CALL with IP=01B6 and SS:SP containing return IP/CS; no
arguments. The final RETF reads its frame after all effects. Initial
SP must be in [0236,FFFC]: six caller bytes and the six-byte loader frame
leave +0000 its required lower bound 022A, including the deepest string
frame. SP=FFFC may wrap to zero after RETF; the frame's bytes do not wrap.
The model does not admit initially wrapped frames.

Effective offsets wrap at 16 bits. Word and LES bytes are contiguous physical
20-bit accesses. A20 is disabled. The loaded code and SYSTEM binding must be
immutable, and all caller/callee/stack writes must avoid code. This is an
explicit precondition inherited from the loader, **not a dynamic code-write
rejection guarantee**. The original-code audit verifies code immutability
for every accepted fixture. Data and stack aliases are otherwise live.

All budgets must be positive. max_clear_iterations is shared between
caller headers +02D7 and +02FC. max_operands resets for each +0000 call;
max_string_iterations resets for each nested string call at headers
+1172/+1327/+13D3/+1453/+14D2. Limits retain reached CPU/RAM immediately
before the declined instruction. They do not manufacture a DOS return or
prove termination for cyclic aliases.

Statuses are OK, ARGUMENT, LOOP_LIMIT, OPERAND_LIMIT, STRING_LIMIT, CALLEE
and TRANSFER. Invalid initial arguments are atomic. CALLEE retains a rejected
nested entry; TRANSFER retains an unexpected internal return. All successful
outer return targets, including targets changed by live data aliases, are
reported as actual returned CPU state. Logical/shift AF is zero and undefined
multi-bit shift OF follows the reused loader convention.

## Validation

The focused C test performs 72 semantic checks: the five entry words and PC
advance, fixed state fields, both clear ranges, restored preservation, two
exact loop-stop states, a write that changes the restored branch, a write
that changes the outer return target, and atomic rejection. Strict C11
warnings plus ASan/UBSan passed. The complete recovered library and the
focused CTest target passed.

The main differential suite passed **800 cases**: 748 complete returns,
five caller loop limits, 45 nested string limits, and two unexpected internal
returns retained as TRANSFER. Caller coverage is **135/135 instruction sites
and 6/6 conditional outcomes**. Transitive coverage is **767/771 sites and
78/82 conditional outcomes**. The four unvisited sites are SYSTEM +0B3B and
U25 +0017/+001F/+0129; complete transitive branch coverage is not claimed.
The initializer normally requests exactly one operand per loader call.
All five declared string-limit headers were exercised. No FLAGS bits were
masked and no emulator CPU values were substituted.

Long packed cases can take over 45 seconds in the Python instruction hook.
The tool emits diagnostic stack traces at that interval and continues; these
are diagnostic timers, not execution failures or relaxed comparisons. The
complete run exited zero with the above measured totals.

The differential oracle executes the closed original-code chain with actual
calls and runtime bodies. It compares CPU14 and all 1,048,576 RAM bytes,
including partial stops. RETF is single-stepped with the code hook kept
installed; no CPU substitutions or FLAGS masks are used. Directed fixtures
cover all 256 tags, string classes, packed lengths/values, both restored
paths, stack limits, live aliases, and budget exhaustion; seeded cases mix
five operands. The report distinguishes caller coverage from transitive
callee coverage rather than claiming unexecuted helper paths.

Two separately recorded whole-memory probes also matched: a window store
that repeatedly resets the live BP-3 loop counter (LOOP_LIMIT at +02D7),
and a window store into the outer return frame (actual return IP=00FF).
Their fixtures and CPU results are recorded in the private alias.json receipt;
they are not silently folded into the main differential-suite count.

The native harness has **30 DOSBox-X normal/386 captures: 29 complete returns
and one partial stop before +02FC**. Each compares all fourteen CPU words,
all 65,536 DS bytes, and 1,808 stack bytes, with zero FLAGS masks. Stack
windows are [EDC0,F010), [0000,0240), and [FD80,10000). Captures include
SP=0236,0237,FFFB,FFFC, all operand shapes, packed lengths through 255,
string-address classes, and two live data aliases. IRQs are masked during
execution. PUSHF uses a scratch stack at SS:7FFE outside captured windows.

The partial native case patches the first declined +02FC instruction to a
far-jump observer after 32 first-loop iterations. Its report restores the
logical stopped CS:IP; it is explicitly counted as a partial capture, not a
complete return. Successful captures execute unmodified original instructions
apart from loaded call-segment fixups. No natural saved-game route, overlay
loader, code mutation, indirect LES operand seam, or wrapped return frame is
claimed as native evidence.

Private reports, native binary captures and extracted bodies stay under
ignored resources/u25-01b6/. The exact-head receipt records source/tool
hashes, reports and capture hashes for reviewer reproduction.

## Reproduction

Provide matching owner media under ignored resources/DOS/POOLRAD/. With
Capstone 5.0.7 and Unicorn 2.1.4 available:

    PYTHONPATH=tools/overlay python3 tools/overlay/u25_init_01b6_audit.py --output resources/u25-01b6/audit.json
    PYTHONPATH=tools/overlay python3 tools/overlay/u25_init_01b6_native.py
    cmake -S . -B resources/u25-01b6/build -DBUILD_TESTING=ON
    cmake --build resources/u25-01b6/build --target poolrad_overlay_u25_init_01b6_tests
    ctest --test-dir resources/u25-01b6/build -R '^poolrad_overlay_u25_init_01b6_tests$' --output-on-failure

## Author parity assessment

- target_function: OVL_0025_01B6, callable [01B6,0322).
- source_candidates: new overlay_u25_init_01b6.c, accepted full-state
  loader/string sources and SYSTEM runtime leaf source.
- signals: byte hashes, public entry/fixups, twelve call sites, complete
  caller branch shape, memory/register/FLAGS effects, original-code and
  native comparisons, focused semantic checks.
- score: strong source identity and measured behavior within the explicit
  loaded-code, stack and memory profiles; not a calibrated probability.
- verdict: likely_match, pending independent review.
- review_notes: challenge live aliases and budget transitions; distinguish
  callable extent from linear queue extent, full returns from partial stops,
  and measured native profiles from emulator-only boundaries. No self-credit.
