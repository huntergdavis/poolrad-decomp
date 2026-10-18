# 0709 +01DF: raw glyph renderer over the memory bus

pr_r0709_glyph_01df_cpu implements the full resident caller [01DF,05EB),
composing the accepted SYSTEM UpCase leaf. It preserves live stack/data
operands and sends every memory access through pr_bus. EGA mode-1 rendering
has separate native plane evidence; modes 0 and 2 have original-code
ordinary-memory evidence. The latter does not establish CGA/Tandy hardware
closure. No accounting or credit change is made here.

## Identity and reused evidence

- Resident 0709:[01DF,05EB), unpacked load-image [726F,767B):
  **1,036 bytes, 403 instruction starts**, terminal RETF 12 at +05E8.
- Body SHA-256:
  a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840.
- Sole callee: far CALL at +0207 to SYSTEM 0AF8:182A.
  The accepted UpCase body [182A,183D) is **19 bytes / 8 instructions**;
  SHA-256 738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c.
- Owner START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

Memory/flag primitives and the original/native oracle structure reuse
[raw rectangle +00B7](RES_0709_RECT_FILL_00B7_CPU.md). The hardware behavior
comes from the accepted [EGA model](EGA_HW_MODEL.md) and memory bus from
PRs #1045/#1047. The sole dependency uses the raw UpCase ABI documented in
[SYSTEM runtime leaves](SYSTEM_RUNTIME_LEAVES.md). No callback replaces
its behavior. The native guest executes the extracted original bodies
with the one far segment rebound to the explicit loaded SYSTEM segment.
Original media, extracted bodies and captures stay in ignored resources/.

The required recall query timed out; deja-health reported a healthy index
with its recall probe skipped. No unusable recall result is cited as proof.
The implementation and checks use the pinned original bytes and the public
source/API evidence above.

## Behavior

The six argument words after the far return pair are transform, character,
repeat count, color, y and x. Only low bytes are used. Initial x must be
at most 39, y at most 24, and DS:5E20/5E22 must form a nonzero font pointer.
A nonzero transform flag applies UpCase, then reduces the result modulo64;
zero uses the full character byte. A zero repeat count returns.

The mode byte DS:4A1E is reread for every repetition:

| Mode | Original memory/port behavior | Evidence scope |
| --- | --- | --- |
|0|Remap color through DS:25D5, build packed pixel words using DS:25CD/25E5, write four row pairs through B800/BA00.|Original CPU/ordinary-RAM parity; no CGA hardware claim.|
|1|Select sequencer map masks through DS:25C9, write eight glyph rows or zeroes to both live segments DS:4A16/4A18, restore mask0F.|Original CPU/OUT/write operands and native EGA planes.|
|2|Pack pairs of font bits into color nibbles, write two groups of four banks B800/BA00/BC00/BE00.|Original CPU/ordinary-RAM parity; no Tandy hardware claim.|
|other|Skip rendering, then advance x and the repetition loop.|Original and native control-state cases.|

The color remap changes the argument byte itself, so later repetitions
read the changed value. X increments as a byte after each repetition;
there is no repeated x/y bounds check. Font addresses wrap as 16-bit
offsets, and font bytes are reread for the second page and each plane.
The implementation preserves those reads even when source/destination or
video memory/stack data overlap. It does not cache scalar arguments.

## API and stop contract

- Exactly 1 MiB A20-off RAM, attached EGA, initial IP01DF and
  SP[001A,FFF0]. The lower bound leaves 26 bytes for the local frame,
  UpCase argument and far return. DS/SS segment windows exclude the EGA
  aperture because the reused UpCase API reads ordinary stack RAM.
- Original code and bindings are immutable and outside the EGA aperture;
  writes avoid code, reached return frames do not wrap. Other aliases
  remain live. CPU/bus/EGA/diagnostic storage is distinct from RAM and each
  other. Port/write callbacks observe only their supplied operands or
  refuse OUT; they cannot change execution state.
- The reached +0207 binding must have opcode9A and offset182A. Its live
  segment selects the loaded UpCase body. CODE stops before that CALL.
  CALLEE and TRANSFER preserve an unexpected dependency stop/return;
  neither is reached by this corpus. A normal outer RETF uses its actual
  frame. Diagnostic callee0 means UpCase, -1 means none.
- A shared budget counts taken backward transfers at 02CA,0351,0381,
  045B,0468,05BC,05C8,05D4,05E2. LIMIT stops before the exhausted transfer.
  Zero permits execution up to the first taken back-edge. This also bounds
  loops whose counters are modified by allowed aliases.
- Refused OUTs return PORT at the pre-OUT CPU state without updating EGA.
  ARGUMENT preserves CPU/RAM/EGA and resets the optional diagnostic.
- The normal/386 flag profile is used; interrupts, faults and unsupported
  EGA configurations are outside the contract. MUL/shift/IDIV result flags
  are dead before an observable stop, conditional branch, OUT or carry use.
  The implementation is not a resumable arbitrary-PC CPU interpreter.

pr_bus routes A0000..AFFFF through the planar EGA model and treats other
addresses as ordinary RAM. Its mode-0/2 memory effects therefore cannot
alone justify other video hardware. Native setup below uses EGA mode0Dh
with graphics write/read mode0 and full bit mask. Controller/latch state
is not independently dumped; written OUT values and all four planes are
checked, including cases whose later reads depend on live EGA data.

## Validation

**183 original-byte cases passed** with exact CPU14, **983,040 ordinary
RAM bytes**, ordered OUTs and EGA byte-write operands, with **zero flag
masks**. All **403/403 caller starts, 52/52 conditional outcomes and
411/411 composed starts** were reached. The report contains **3,305 OUTs
and 6,088 EGA write operands**. Results: **132 returns, two CODE,
34 PORT and 15 LIMIT stops**.

The corpus covers every mode branch, six flag profiles, color combinations,
UpCase boundaries, empty/full/mixed font rows, initial coordinate guards,
nil font, repeat255, stack endpoints/odd stacks/A20, shared DS/SS,
font-offset wrap, shared pages, ordinary-RAM pages, mask aliases, malformed
bindings, refused OUT positions, budgets and deterministic random cases.
A deliberate B800 stack alias changes the mode-2 bank byte while rendering,
reaching +0595's otherwise unobserved taken branch. Another case overlaps
font input and ordinary output; live reloads remain visible.

**64 native cases passed** under DOSBox-X **machine=ega, mode0Dh,
core=normal, cputype=386**. Each compares **CPU14, SS256 bytes, DS64KiB,
ordinary font-buffer64KiB and all four 64KiB planes**, with **zero flag masks**.
A separate guest INT1 run compares **3,060 ordered OUT events**. Plain
runs supply CPU/RAM/plane evidence; traced runs are used only for OUTs.

Native cases cover colors/pages, repeats, transform/character boundaries,
flags, guards, zero/combined masks and shared pages. Two cases read font
bytes from the EGA aperture; one overlaps the font and destination. One
malformed-binding checkpoint and all three static OUT checkpoints are
captured before the unexecuted instruction via a far-JMP trap. The trap
changes no general register, flag or stack byte; reported CS:IP names the
original boundary. Its five overwritten bytes are restored between cases.
These are synthetic direct executions, not saved-game reachability proof.

The first native run exposed four stale bytes left in the capture window
by fixture DOS file I/O. Moving the window initialization after setup I/O
fixed the fixture; no C behavior or comparison mask changed to hide it.

Native RESULT.BIN SHA-256:
25009fbb4c17593017cae26fa2f9845e450e20a224b957eb983679756529769a.

Strict C11 ASan/UBSan passed **3,205 semantic checks**, including direct
expected-pixel assertions for EGA and packed ordinary-memory outputs,
repeated columns, flag preservation, exact budget/port stops and rejection.
Focused CTest passed **1/1** on integration base
aff11e3b16a9276290a6b5f84a899c2bf0618880.
All 16 source/header/audit dependency pins were unchanged when main was
refreshed before publication. No rebase or repeated oracle run was needed.

The private receipt resources/01df-evidence.json pins 18 public files,
18 private artifacts, owner media and tool binaries. It includes original/
native reports, capture inputs/output, harness/configuration and sanitizer/
CTest logs. Documentation is excluded to avoid a self-hash cycle.
Receipt SHA-256:
fdfbb622868dc0af83ed271ae1efcb28931676b3c89441cf174b4b3c35e41e71.

## Reproduction

Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler are required.
Owner START.EXE stays under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_glyph_01df_cpu_audit.py --unicorn --native --output resources/01df-run.json
    cmake -S . -B resources/01df-build
    cmake --build resources/01df-build --target poolrad_resident_0709_glyph_01df_cpu_tests
    ctest --test-dir resources/01df-build -R '^poolrad_resident_0709_glyph_01df_cpu_tests$' --output-on-failure

## Author review

- target_function: RES_0709_01DF complete [01DF,05EB), one raw UpCase call.
- source_candidates: resident_0709_glyph_01df_cpu.c, accepted runtime
  leaf, memory bus and EGA implementation.
- signals: pinned identities, exact source-order operands, complete caller
  starts/outcomes, composed-start coverage, original full ordinary-RAM and
  ordered-I/O comparison, independent native original plane captures.
- score: strong within the stated CPU/bus contract; native hardware evidence
  is EGA-only, and no natural-game reachability is established.
- verdict: likely_match within contract; mode-0/2 hardware closure open.
- review_notes: no self-credit or accounting edits. Independent reviewer/VP
  controls acceptance and any credit.
