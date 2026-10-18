# U38 +04BA: list candidate selection and byte-pair replacement

Author candidate for independent verification; no acceptance or credit changes.
The schema-7 successor of the schema-4 queue ranked this caller 30, the highest
composable hard target remaining at selection. Its two raw CPU dependencies
are merged in PRs #916 and #917. This implements the entire 284-byte,
99-instruction public entry 12, with both real U9E callees on shared CPU/RAM.

## Original identity and reuse

START.EXE: 65,231 bytes, SHA256
 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR: 206,105 bytes, SHA256
 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
U38 starts at file offset 60507. The caller occupies [04BA,05D6), file
[61717,62001), with RETF 10 at +05D3. Public entry 12 maps to +04BA.

| Body | Bytes | Instructions | SHA256 |
| --- | ---: | ---: | --- |
| U38 +04BA | 284 | 99 | 0556f1d0f2eb60cc9d8d5e346274a4b6ba13c9e0a0dad054bc2d7eccbbcf010e |
| U9E +10E4 | 41 | 17 | 12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db |
| U9E +110D | 79 | 29 | 63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830 |

The implementation calls the existing public U9E APIs directly. Their provenance:

- +10E4: PR #917, merged implementation commit
  00166e2e43e7213dda6075825a5dd27d5dd70e80.
- +110D: PR #916, implementation commit
  f342b612c183b84ce0bd33b3f7f91478e73d82cc.

The raw-CPU oracle and native harness follow the U3C6 +071A / PR #923 test
structure, adapted for two loaded overlays and relocated far-jump stubs.
No scalar result callbacks or hook stand-ins replace a callee. CPU structs are
copied field by field when crossing the two existing header types, avoiding
incompatible-struct pointer casts.

Recall was attempted before implementation with deja "U38 04BA full state
10E4 110D". It timed out. deja-health --repair reported an interrupted staging
directory and started a background rebuild; a later file-state health check
reported the active rebuild. The hook-context attempt also timed out. No
recalled session result is reused or represented as evidence.

## ABI and behavior

At entry, SS:SP holds the return IP/CS. The word at SP+4 is unused; the far
source pointer is at SP+6 and the far record pointer at SP+10. The source byte
at +3 above 127 exits immediately. Otherwise +110D decodes that byte into the
caller's two live local bytes, which are copied to record fields +10 and +16.

The caller walks the far list rooted at record +7F/+81, following links +5/+7.
Only node kinds 12 and 38 are considered; an exact offset/segment match with
the source is skipped. Each eligible node is decoded by the actual +110D
body. The caller retains a greater primary byte, or a greater secondary byte
when the prior primary byte is 18. The chosen node receives the original
record pair re-encoded by +10E4; the record receives the chosen pair. Empty or
ineligible lists retain the initial decoded record fields. Pointer equality
compares the two words, not normalized physical addresses.

Pinned call sites are +04DD and +0552 to U9E stub 00B0:0075 (+110D), and
+05B0 to 00B0:0070 (+10E4). The audit verifies all three relocation words
(+04E0/+0555/+05B3), both public entry mappings, complete body hashes, and the
absence of callee relocation sites. No incoming aligned call is claimed;
public entry identity does not establish a measured natural game route.

Every table, node, frame and pointer read remains live. The corpus includes
record/frame aliases, source=record, node=record, offset-wrapped node fields,
exact self nodes, unequal segment words, cyclic lists and boundary stacks.
The implementation preserves real pushes, callee frames, FLAGS and RETFs.
The loaded U9E segment is explicit. Transparent relocated JMP stubs have no
CPU/RAM side effects; dynamic overlay loading is outside this API.

## Execution profile and limits

RAM is exactly 1 MiB with A20 off. Effective offsets wrap at 16 bits; bytes
within word and LES operands are physically contiguous. RETF reads IP and CS
as separately wrapped SS words. Logical-operation AF is explicitly zero,
matching the compared Unicorn and DOSBox-X normal-core cases. The full FLAGS
word is compared; no flags are masked.

Both caller and +110D retain the public contiguous LES profile. Operand starts
FFFD..FFFF are covered only by Unicorn, not native parity. Existing native
LES-seam differences documented for other recovered callers must not be erased
by labeling this chain generically raw CPU. Native cases below exclude those
operand seams. The RETF seam is separate: Unicorn 2.1.4 uses a contiguous CS
read at SP=FFFE/FFFF, so the audit executes RETF and then explicitly substitutes
the wrapped CS at those returns, counting every substitution. These hybrid
cases are not described as exact unmodified-Unicorn CPU matches.

The API atomically rejects null pointers, wrong RAM size, wrong entry IP and
conservative stack/code overlap across the deepest 32-byte frame plus word
extensions. CPU storage must be outside RAM. All code must be immutable:
callers must exclude data-pointer writes to code, including +110D stores.
Other data aliases are supported. The audit rejects code-writing fixtures.

One budget unit is consumed before each +0518 loop head, including its final
null-pointer check. LIMIT preserves prior effects and stops before that head;
it is not resumable. Cycles are not converted to successful returns. An
unexpected callee return is exposed as TRANSFER; no fixture claims to have
exercised that defensive status. Final RETF 10 consumes the actual live frame.

## Author validation

- 3,018 full 1 MiB RAM / CPU14 comparisons: 3,000 exact Unicorn comparisons and
  18 hybrid wrapped-return cases, with 30 explicit CS substitutions.
- 145/145 instruction starts and 24/24 conditional outcomes across all three
  bodies. Twelve budget stops; 45 contiguous-LES seam cases counted separately.
  Zero FLAGS masks and zero unreported comparison exclusions.
- 24 DOSBox-X normal/386 original-chain cases compare CPU14, 224 stack bytes,
  and 8,192 data bytes per case. The data capture includes the physical stack
  extension used by offset-FFFF words. Ten cases enter at SP=FFFE/FFFF; two
  enter at SP=001C/001D to wrap +110D's internal return frame. Selected-node,
  empty/ineligible, exact-self, rank/tie and high-bit rejection paths pass.
- Strict C11 warnings-as-errors ASan/UBSan tests cover source rejection,
  decoding, list selection/replacement, frame cleanup, cycle limits and atomic
  rejection. The recovered-library build and focused CTest are also run.

Original disassembly, native programs and captures remain in ignored
resources/u38-04ba. original.json, native/report.json, capture SHA256 values,
build/test logs and evidence.json form the local replay package. No original
media or derived bytes are committed.

With Capstone/Unicorn installed and owner media under resources/DOS/POOLRAD:

    python3 tools/overlay/u38_pair_04ba_audit.py --output resources/u38-04ba/original.json
    python3 tools/overlay/u38_pair_04ba_native.py
    cmake -S . -B resources/u38-04ba/build
    cmake --build resources/u38-04ba/build --target poolrad_overlay_u38_pair_04ba_tests
    ctest --test-dir resources/u38-04ba/build -R poolrad_overlay_u38_pair_04ba_tests --output-on-failure

## Author parity assessment

- target_function: OVL_0038_04BA.
- source_candidates: overlay_u38_pair_04ba.c and the two merged U9E raw APIs.
- signals: pinned original bytes, full dependency closure, actual far-call
  frames/relocations, whole-RAM/CPU oracle, alias fixtures and native replay.
- score: strong within the declared profile; native LES seam domain incomplete.
- verdict: likely_match, pending independent verification.
- review_notes: distinguish hybrid RETF comparisons from exact Unicorn cases;
  retain the contiguous-LES caveat in queue readiness. Natural invocation,
  dynamic loader execution and the defensive TRANSFER branch are unmeasured.
  No author credit is claimed.
