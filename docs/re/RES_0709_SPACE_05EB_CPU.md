# 0709 +05EB: guarded space-glyph composition

pr_r0709_space_05eb_cpu implements the complete resident [05EB,0626) body,
composing the accepted raw +01DF glyph renderer and SYSTEM UpCase. This
continues the assigned [16EC dependency plan](RES_00BA_16EC_PORT_HARNESS.md)
toward +0BF5 and the line-input path. Existing queue snapshots are not a
fresh readiness ranking. No credit or accounting change is made here.

## Identity and provenance

| Body | Bytes / instructions | SHA-256 |
| --- | ---: | --- |
|0709:[05EB,0626)|59 / 27|a41af7187ac3e0bf5a09311bad9fae220c498f0f9aad9492438661381d4b1171|
|0709:[01DF,05EB)|1036 / 403|a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840|
|SYSTEM 0AF8:[182A,183D)|19 / 8|738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c|

The caller occupies unpacked load-image [0767B,076B6), calls +01DF at +061F,
and ends with RETF 8 at +0623. Owner START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

Reused public evidence: [row14 return path](COLD_START_CODEWORD_ROW14_RETURN_PATH.md),
[row14 boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md),
[glyph renderer](RES_0709_GLYPH_01DF_CPU.md), and
[printer composition](RES_0709_PRINTER_0626_CPU.md). The latter two supply
accepted CPU/flag primitives, bus/EGA dependencies and oracle infrastructure.
The earlier return-path description's “one-cell clear helper” is contextual:
this body actually accepts a caller-supplied count and draws the supplied
space bitmap. It is neither inherently one-cell nor necessarily a zero fill.

The required deja queries for the +0BF5/CRT path and +05EB space helper
returned no matches and reported zero indexed sessions; no prior-session
claim was reused. Original bodies, generated guests and captures remain in
ignored resources/.

## Behavior and contract

Four words after the far return pair are count, color, y and x, using their
low bytes. The wrapper rejects x>39 or y>24. Otherwise it pushes x, y, color,
count, character32 and transform1 while preserving incoming AH, pushes CS,
then calls the raw glyph renderer. Count is not clipped at the right edge.
Count0 still reaches glyph/UpCase before the glyph's zero-count guard.
Arguments, mode, font and page state remain live memory reads.

- Exactly 1 MiB A20-off RAM, attached EGA, initial IP05EB, SP[002C,FFF4].
  The lower bound reserves 18 bytes before the nested glyph frame.
- Whole DS/SS windows exclude A0000..AFFFF. Code/bindings are immutable
  outside EGA; writes avoid code; reached frames do not wrap. Other aliases
  remain live. CPU/bus/EGA/diagnostic objects are distinct from RAM and each
  other. Callbacks only observe/refuse supplied OUT operations.
- CALL061F requires opcode E8 and displacement FBBD. CODE retains the
  preceding PUSH CS and exact pre-call state.
- max_loopbacks is passed to glyph, whose LIMIT stops before the exhausted
  transfer. There is no additional outer loop. CALLEE retains dependency
  status/state. Diagnostic callee0 means glyph, -1 means none; glyph_callee
  and glyph_status retain nested UpCase diagnostics.
- TRANSFER retains an unexpected successful child return; no new independent
  TRANSFER case was measured. ARGUMENT preserves CPU/RAM/EGA and resets the
  optional diagnostic.
- Normal/386 flags; no interrupts, faults or arbitrary-PC resumption.
  All glyph bus/hardware restrictions apply. Native scope is EGA mode0Dh;
  packed modes0/2 ordinary-memory parity does not establish CGA/Tandy closure.

## Validation

**160 original-byte cases passed**, comparing CPU14, **983,040 ordinary RAM
bytes**, ordered OUTs, EGA write operands and four diagnostic values with
**zero flag masks**. Coverage: **27/27 caller starts**, **6/6 feasible caller
conditional outcomes** out of eight syntactic outcomes, and **433/438 composed
starts**. The corpus recorded **14,721 OUTs** and **27,512 EGA write operands**.
Stops: **108 OK, 2 CODE, 50 CALLEE**.

Branches +05F2->0606 and +05FE->0606 follow unsigned byte CMP against zero.
That comparison always clears CF, so those JB outcomes are impossible under
the uninterrupted contract. They are explicitly reported rather than counted
as covered. Unreached child starts are glyph +021A/+021D and SYSTEM
+1834/+1836/+1838: ordinary calls supply transform1 and character32. This is
not complete transitive branch coverage or a proof about every data alias.

Cases cover modes0..3, all16 colors across modes0/1/2, counts0/1/2/3/40/255,
coordinate guards, six flag seeds, glyph budgets, 35 refused-OUT positions,
bad near opcode/displacement, nested UpCase failure, stack endpoints/odd
stack/A20, SS=DS, mode-stack alias, AH=FF, font offset wrapping/missing font,
zero/ones bitmaps, shared pages and ordinary-memory pages. The flat original
oracle uses ordinary-memory fonts; planar font reads are native-only.

**58 native DOSBox-X cases passed** with machine=ega, mode0Dh, core=normal,
cputype=386. Each compares **CPU14, SS1024 bytes, DS64KiB, font64KiB and all
four64KiB EGA planes**, with **zero flag masks**. Separate INT1 executions
compare **4,930 ordered OUTs**. Two cases read font bytes through EGA. Two
pre-CALL checkpoints (+061F/+0207) and three pre-OUT checkpoints
(+03AD/+03D5/+0470) capture exact stopped state. Native counts are0/1/3/5;
the original-byte corpus covers40/255. These are synthetic direct executions,
not natural-game captures. Final controller/latch state is not independently
dumped.

Native RESULT.BIN SHA-256:
502f7a76b2ef46529534c087cd7d136faf5df34eb62c0f9e9c19ab669800e1e9.

Strict C11 ASan/UBSan passed **4,215 semantic assertions**, including nonzero
space pixels in all16 colors, both pages and neighboring pixels; no clipping
of a second character at x39; count0 reaching UpCase without drawing; exact
port/budget/code frames; nested failure; live SS=DS mode alias; and rejected
input immutability. Focused CTest passed **1/1** on integration base
5438ae2f027244b0a258e9ecca5a1826de3c5a02. Refreshed origin/main matched that
tested base; all15 reused dependency pins were unchanged.

Private receipt resources/05eb-evidence.json pins **21 public files and
19 private artifacts**, media and tool binaries. It includes all five compiled
C sources, relevant headers/imports, original/native reports, guest
input/output/configuration, and sanitizer/CTest logs. Documentation is
excluded to avoid a self-hash cycle. Capstone5.0.9 and Unicorn2.1.4 were used.
Receipt SHA-256:
dddf933333485495a9e1dea400edee4e56dd38ccc2b2b827dc4fe4d8c457dc84.

## Reproduction

Requires Capstone, Unicorn, DOSBox-X, GNU as/ld and a C compiler; owner media
must stay under resources/DOS/POOLRAD.

    python3 tools/overlay/res0709_space_05eb_cpu_audit.py --unicorn --native --output resources/05eb-run.json
    cmake -S . -B resources/05eb-build
    cmake --build resources/05eb-build --target poolrad_resident_0709_space_05eb_cpu_tests
    ctest --test-dir resources/05eb-build -R '^poolrad_resident_0709_space_05eb_cpu_tests$' --output-on-failure

## Author review and remaining readiness

- target_function: RES_0709_05EB complete [05EB,0626), raw glyph/UpCase chain.
- source_candidates: resident_0709_space_05eb_cpu.c and accepted glyph,
  SYSTEM leaves, memory bus and EGA implementations.
- signals: pinned identity; live source-order operands; full caller starts
  and feasible outcomes; original RAM/I/O parity; independent native original
  execution; expected pixels and exact stopped-state assertions.
- score: strong within the stated CPU/bus contract.
- verdict: likely_match within contract; native hardware evidence is EGA only.
- review_notes: independent reviewer/VP decides verification and credit.
  No self-credit or accounting edits; no natural-game reachability claim.

+0BF5 still depends on 0899:0059 and its CRT/BIOS/DOS/sound closure. This
wrapper alone does not make that chain ready. Queue readiness must preserve
that caveat along with the unmeasured transitive outcomes and packed-mode
hardware limits above.
