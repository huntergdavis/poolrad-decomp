# U3C2 entry 1: signed-word signum

`pr_u3c2_signum_0000` translates the complete U3C2:+0000..002D leaf:
AX returns FFFF, zero or one for a negative, zero or positive signed word.
This is a raw-state translation with live stack/RAM and original flag effects.
The current classification is helper_candidate/unknown; verified game identity,
strict-credit eligibility, natural game reach and merge remain separate PM
gates. No accounting or classifier files change.

## Source and callers

Owner inputs are START.EXE (65,231 bytes), SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
and GAME.OVR (206,105 bytes), SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3C2 code file base is 195410, stub segment 03DBh. Public entry slot 1
points to +0000 via stub 03DB:0025. Exact body file extent is
[195410,195456), 46 bytes / 17 instructions, SHA256
`5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf`.
Terminal +002B is RETF 2. All body bytes decode; two conditional sites
have four edges. There are no calls, interrupts or body relocations.

The audit validates two aligned near calls inside U3C2:+01A0:
+01F9 and +0213, each with PUSH AX; PUSH CS before CALL +0000.
Their inputs subtract record words +4 minus +0 and +6 minus +2,
respectively. AL is stored into record bytes +12h and +13h after the
return. This supports sign-of-difference byte behavior without assigning
a broader game meaning to the record. Two additional aligned,
fixup-confirmed far calls are U86:+30D9 sites +3150 and +316D through
03DB:0025. The source audit checks the complete raw near-target set,
aligned argument/result instructions and all matching far fixups.

## Raw-state behavior

At entry after CALL, the signed word argument is SS:SP+4. PUSH BP and
SUB SP,2 create a saved BP and one local word. Negative input takes the
first CMP branch; positive input takes the second CMP branch; zero executes
XOR AX,AX. The local word receives FFFF, one or zero and is read back into
AX, so AH is overwritten. MOV SP,BP, POP BP and RETF 2 read live RAM;
the model reports the actual popped CS:IP and final SP = entry SP + 6.
Each RETF word pop wraps SP independently; CS uses SS:(entry SP+2)
modulo 65536.
Other general and segment registers remain unchanged.

For nonzero input all six arithmetic flags come from CMP input,0:
CF/AF/OF/ZF are zero, SF is the input sign, and PF reflects the low input
byte. Flags must not be derived from the returned AX. For zero, XOR defines
CF/OF/SF=0 and PF/ZF=1; AF is undefined and the C model chooses zero.
The oracle masks only AF (0010h), only when the original executes +0020;
all other flag bits and all RAM bytes are compared exactly. The native
ordinary probe uses the same sole mask on its three zero inputs; the
boundary probe has two zero inputs, described below.

Effective offsets and SP wrap at 16 bits; contiguous operands and A20-off
RAM wrap at 20 bits. Code must remain immutable and disjoint from writes.
Null pointers or RAM shorter than 1 MiB are rejected before mutation.
No dependency callback, loader or natural game route is modeled.

## Reproduction and measured evidence

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c2_signum_0000_audit.py \
  --output resources/u3c2-0000-parity/full.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c2_signum_0000_native.py
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u3c2_signum_0000_native.py --wrap-returns
```

Capstone 5.0.7 / Unicorn 2.1.4: 2,774 original-byte cases compare the
complete 1 MiB and fourteen CPU words. Six FFFE/FFFF return cases use
explicitly reported native-supported CS expectations for Unicorn 2.1.4
which reads that CS linearly. Complete RAM and the other CPU words use
raw original execution; this does not claim raw-Unicorn CS agreement at
those six returns. All 17 instruction starts and all
four conditional edges are covered. The matrix sweeps every high/low input
byte against representative opposite bytes, sign boundaries, 66 stack/
physical-wrap cases, initial flags and 128 deterministic random whole-RAM
backgrounds. Seven zero-path cases mask undefined AF; nonzero flags have
no mask. Quick mode passes 254 cases. Each original invocation reaches
RETF within 32 instructions, with no budget exclusions.

DOSBox-X normal-core 386 passes 776 synthetic cases executing original
+01F7..01FB (PUSH AX; PUSH CS; CALL +0000) and the complete leaf in a
separately allocated code segment. A transparent far jump captures the
original +01FC return before its LES. The input matrix covers all 256
paired high/low-byte values, all 256 small nonnegative values, 256 values
at/above 8000h, and eight boundary values. Fourteen CPU words and 64 frame
bytes match C; defined flags are exact. Native RESULT.BIN SHA256:
`d06864c383f78c2dc917627ba6749a399e8ff332220f9b4658f6cf3a37ee0f20`.
The capture compares these frame bytes rather than the entire native RAM.
Original loader dispatch and natural game invocation remain unmeasured.

The separate native RETF2 boundary gate passes 16 original-prefix cases:
entry SP=FFFE and FFFF across eight signum inputs each. A 128 KiB private
program allocation keeps the linear shadow above SS:FFFF separate from the
wrapped CS word. All fourteen CPU words and 104 frame bytes match C:
SS:FFB0..FFFF (80), SS:0000..0007 (8), and physical shadow at SS base+
10000h..1000Fh (16). Only AF on the two zero-input XOR cases is masked;
all other flag bits are exact. Native RESULT.BIN SHA256:
`145afb3a1e76a17b5884be471d43d527093f36401f65ff23d598e2c8a5ea09a0`.
The quarantined 857d1c3 candidate read CS=A5A5 in all 16 cases while
native returned the actual loaded segment; the other CPU/frame observations
matched. The corrected CS pop, separately wrapped IP/CS fixture words and
two independent C regressions close this edge. The previous wrap claim was
insufficient because fixture setup and Unicorn shared a linear-CS assumption.

The focused synthetic C test exhausts all 65,536 input words against the
mathematical signum, with independent flag vectors, guards, local/saved
frame effects, return cleanup and unchanged-register checks. All 246 CTests
pass after a strict -Wall/-Wextra/-Wpedantic/-Werror build. Shell syntax,
research checksums, staged whitespace and tracked-media hygiene pass.
Original media,
extracted bytes/disassembly, generated COM files and captures stay under
ignored resources/; the public tree contains source, synthetic tests,
address/hash evidence and extraction tooling.

- `target_function`: U3C2 public entry 1, +0000..002D.
- `source_candidates`: new uniquely named raw-state signum implementation.
- `signals`: exact source/extent/public entry; aligned near and far caller
  evidence; whole-RAM/CPU original parity; exhaustive C inputs; independent
  native original-prefix execution; path-specific undefined-flag handling.
- `score`: 17/17 instruction starts and 4/4 edges; no whole-game denominator.
- `verdict`: match within the immutable-code/raw-frame contract.
- `review_notes`: natural reach, broader record meaning, verified identity,
  strict-credit eligibility and publication/merge require their separate gates.

The bounded deja query returned no completed finding. Reused findings are
[merged CPU and offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), measured
ranked-ledger selection (rank 445, grade A), and the independently reviewed
[U33 separate-code native capture design](https://github.com/huntergdavis/poolrad-decomp/blob/3986c34c00232eb12c662ea3bd39767b69888d98/docs/re/U33_BOTH_OUTSIDE_1403.md).
This branch requires no unmerged symbol. Its CMake registration shares an
insertion area with seven other pending registrations; integration must preserve each entry.

The RETF repair bounded deja query returned no completed finding. The
native boundary method reuses the PM-reviewed
[U3C6 explicit wrap gate](https://github.com/huntergdavis/poolrad-decomp/blob/6dd20497198f3940e7a82343d60adb1275236af3/docs/re/U3C6_IN_RANGE_06E9.md),
with this function's own RETF2 prefix, eight signed-word inputs, AF rule,
CPU/frame comparison and native capture above. This standalone current-main
candidate replaces the quarantined 857d1c3 head.
