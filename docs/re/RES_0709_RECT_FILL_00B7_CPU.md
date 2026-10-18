# 0709 +00B7: raw rectangle fill over the EGA bus

pr_r0709_rect_fill_00b7_cpu composes the original rectangle body with the
raw page selector +00A0, register reset +0000, and bus-aware SYSTEM FillChar
+16A8. CPU registers, flags, live stack/data operands, port refusals, and
video-write effects are preserved through the shared pr_bus/pr_ega model.

This is an author submission for independent review. It changes no credit,
denominator, or acceptance accounting. It does not close the enclosing
+01DF/+0DD3/+0768 or RES 00BA:16EC callers.

## Identity and reused evidence

The expanded START.EXE span [7147,7201) is resident 0709:[00B7,0171):
**186 bytes, 89 instructions**, RETF 12 at +016E. The old bounded note's
51-instruction count was incorrect; the original-byte audit now pins 89.

| Body | Bytes / instructions | SHA-256 |
| --- | --- | --- |
|0709:00B7|186 / 89|f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5|
|0709:0000|113 / 58|5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5|
|0709:00A0|23 / 10|6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d|
|0AF8:16A8|20 / 7|f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7|

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

Recall queries for the EGA/rectangle work stalled; deja-health --repair
started a background rebuild. No unreturned recall result is cited as
evidence. The direct sources reused here are the
[handoff design](RES_0709_RECT_FILL_00B7_DESIGN.md),
[bounded rectangle evidence](RES_0709_RECT_FILL_00B7.md),
[raw reset and native tracer](RES_0709_EGA_RESET_0000_CPU.md),
[raw page selector](RES_0709_PAGE_SELECT_00A0_CPU.md), and
[EGA model](EGA_HW_MODEL.md). Claude supplied pr_ega in #1045 and the
bus/FillChar stage in #1047. The CPU arithmetic primitives follow the
previous raw +0160 implementation.

## Behavior

The six words after the far return pair are color, page, bottom, right,
top, left. Only the low bytes are used. Coordinates are sign-extended,
while the page helper zero-extends its page byte. The upper AX byte is
preserved whenever the original only loads AL.

The caller saves the active-page byte DS:4A1A at BP-1, computes a wrapped
16-bit starting offset top*320+left and row bound
(bottom-top+1)*8-1, then selects the requested page and resets the EGA.
It selects write mode 2 and calls FillChar once per row, passing the live
width right-left+1 and color. Each next row adds 40 to the local offset.
It restores write mode 0, writes the saved active page back to DS:4A1A,
selects that page, and returns with RETF 12.

Zero width performs no stores. Reversed/signed coordinates retain the
original word wrapping; no rectangle clipping or synthetic successful
return is introduced. A wrapped row bound can cause 65,536 rows, so the
caller has an explicit loop limit.

The three near calls are +00FC -> +00A0, +0100 -> +0000, and
+0168 -> +00A0, each following PUSH CS. The far call at +0140 must be
LCALL offset 16A8; its live segment operand supplies the FillChar code
segment. Native and original-byte probes resolve only this relocation.

## API and boundaries

- The bus must supply exactly 1 MiB RAM and an attached EGA, with initial
  IP00B7 and SP[0020,FFF0]. DS and SS 64 KiB windows must avoid the EGA
  aperture because the reused page/reset helpers access plain RAM.
  CPU, bus, EGA and optional diagnostic storage are distinct from RAM and
  each other. Code resides outside the EGA aperture, remains immutable,
  and writes avoid code. Reached far-return frames do not wrap.
- Other RAM/stack aliases stay live. In particular, a page-global store
  can overwrite the helper's return IP; the caller preserves this as
  TRANSFER rather than substituting the expected return.
- max_loopbacks counts taken JNEs at +014F. Zero permits the initial row.
  LIMIT returns immediately before an exhausted JNE, preserving its flags,
  registers, locals, port effects and completed stores.
- CODE stops before a malformed +0140 call. PORT stops before a refused
  caller OUT; CALLEE retains a helper stop with its identity/status.
  Diagnostic identities are 0 page, 1 reset, 2 FillChar, -1 none.
  The optional diagnostic is reset on entry; ARGUMENT changes no CPU,
  RAM or EGA state.
- Observers may record/refuse but must not mutate modeled state. A refused
  port write does not reach pr_ega. The routine sets supported write modes
  before stores, so pr_bus's unsupported write-mode-3 behavior is not used.
- Interrupts, faults, self-modifying code, and other video hardware outside
  A0000..AFFFF are outside this model. Ordinary out-of-aperture accesses
  use RAM; that is not evidence of CGA/Tandy device behavior.

MUL/SHL intermediate undefined flags are dead before every observable
return/stop: the page helper's final ADD overwrites them before the first
port/refusal, and the row's arithmetic defines flags before FillChar and
its malformed-call stop. No comparator masks flags.

## Validation

The original differential checks CPU14, **983,040 RAM bytes** outside the
64 KiB EGA aperture, stop/diagnostic, ordered OUT events, and every ordered
video-write (offset,value) operand. Unicorn has no planar EGA device; its
flat bytes in the aperture are deliberately not treated as hardware planes.

The final corpus includes all 16 colors on all eight pages, incoming flag
profiles, random rectangles, signed coordinates, zero/wrapped widths, odd
and endpoint stacks, A20 stack wrapping, shared DS/SS, the changed-return
alias, all 22 OUT refusal sites, malformed far calls and exhausted row
limits. **202 cases passed**: 174 returns, two CODE stops, 18 CALLEE
refusals, four own PORT refusals, one TRANSFER and three LIMIT stops.
The comparison checked **4,159 OUT events and 98,347 video-write operands**,
with no flags, CPU or ordinary-RAM masks.

All **89/89 caller starts** and **164/164 composed starts** are reached.
Three conditional outcomes are reachable. The fourth, JA taken at +011A,
is impossible: XOR AX,AX immediately precedes CMP AX,word, so unsigned
zero cannot be above the compared value.

Native DOSBox-X uses **machine=ega, mode 0Dh, normal/386**. Its **50 cases**
compare CPU14, a 256-byte stack window, the complete 64 KiB data segment,
and **all four 64 KiB planes**. A separate single-step run compares
**1,100 OUT events**, including their CS:IP, port, value and width.
The plain run provides the stack evidence because single stepping writes
interrupt frames below SP. Both comparisons use **no masks**.

Planes begin with distinct per-plane byte patterns. Native cases include
all colors, all pages, six flag profiles, full-screen/edge/single-cell
rectangles and zero width. The capture does not independently compare
unwritten EGA registers or final latch contents; written register values
are checked through the traced OUT sequence. These are direct synthetic
runs, not new saved-game reachability measurements.

Native result SHA-256:
fa0db1c025982f9ceba13ef16d63cc86c75598ca5671c12048461a50cdce1669.

The focused C11 ASan/UBSan test passed **2,168 checks**, including
hand-predicted plane bytes, unchanged neighbors/page, restoration, port
refusal, loop limits and rejected arguments.

The focused CTest passed **1/1** after rebasing onto
0cc45a36769dc93f391dda9e72451940a399c8e8; all six compiled source files match the parity-tested versions.
Private resources/00b7-evidence.json pins code/header/audit hashes, reports,
native artifacts, media, DOSBox-X binary and sanitizer/CTest logs.
Receipt SHA-256: fbcac163d2008bcd7b16bd8fa14c421fb3efe42b7a469b4148f78cfd045a4b14.

## Reproduction

Owner media, extracted bytes, native binaries and captures remain under
ignored resources/. The native oracle invokes DOSBox-X externally and
includes no emulator source.

    python3 tools/overlay/res0709_rect_fill_00b7_cpu_audit.py --unicorn --native --output resources/00b7-run.json
    cmake -S . -B resources/00b7-build
    cmake --build resources/00b7-build --target poolrad_resident_0709_rect_fill_00b7_cpu_tests
    ctest --test-dir resources/00b7-build -R '^poolrad_resident_0709_rect_fill_00b7_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_00B7, original [00B7,0171).
- source_candidates: resident_0709_rect_fill_00b7_cpu.c with the merged raw
  page/reset helpers, bus-aware FillChar and pr_ega.
- signals: pinned body/helper identities, live operands and return frames,
  complete instruction coverage, all reachable conditional outcomes,
  original CPU/RAM/OUT/write-operand parity, independent native plane bytes
  and traced OUTs, and focused sanitizer checks.
- score: strong within the documented RAM/EGA and bounded-execution contract.
- verdict: likely_match; independent reviewer acceptance remains pending.
- review_notes: no native claim for other video hardware, interrupted runs,
  or natural-game reachability; no accounting or self-credit changes.
