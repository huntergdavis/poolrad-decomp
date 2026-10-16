# U3C2 +05F3 supplied-state byte comparison

The fixture performs only CMP AL,SS:[BP+0A], then stops at +05F6 before JNE.
The 3-byte source extent is [196933,196936), SHA256
`2ad72c7b85ea62f631525de59646f9e9a5d2e70192384c99207d2d0eb9c8a68c`.

The effective offset wraps to 16 bits before adding SS*16; the physical
address is masked to 20 bits. The mandatory
`PR_U3C2_05F3_AF_CMP_DEFINED=1` policy computes CF/PF/AF/ZF/SF/OF from
the byte subtraction. Every other FLAGS bit, AX including AL, every other CPU
word and all RAM remain unchanged. RAM and the 14-word CPU object must be
distinct, with at least 1 MiB RAM, immutable code and no asynchronous traps.
Null pointers, short RAM, wrong IP and unsupported policies reject atomically.

The synthetic C suite checks all 65536 AL/memory-byte pairs, all 65536 incoming
FLAGS words, and all 65536 BP values coupled with odd-permutation SS/AX/FLAGS
values over nonperiodic RAM. Each case compares the complete CPU; each sweep
checks the entire unchanged RAM image. Eight additional snapshots use literal
offset/A20/code-address boundaries and distinct DS/ES/SS. The independent
reference uses signed range for OF, nibble borrow for AF and bit counting for
PF rather than the implementation's XOR/fold formulas. Invalid policies
0/2/255/-1 and argument failures reject unchanged.
Sanitized runtime on this restack: 0.22 seconds.

## Provenance and scope

Original review parent: `6c22877d1d37f928bf741e88cfe6f0a0f367c1fc`.
Publication restack parent: `a2b1e7344918435fa94e5ee9be67342130f4474b`. PM i6842/i6869 approved this serial scoped restack. PM assignment i6771 and correction i6785
authorize these supplied-state slices on independent branches. The proposed
+0907 target was rejected: it is immediate data within the reserved +0906
RETF instruction [0906,0909). No +0906 return policy is selected.

Owner-supplied GAME.OVR: 206105 bytes, SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3C2 entry4 [057B,0909), file [196813,197723), SHA256
`59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae`.
Original media, instruction bytes, constructed images and logs remain ignored.

Bounded recall used `timeout 12s deja "U3C2 05F0 AL stack load 0907 RETF boundary"`;
exit 124 is an incomplete search, not evidence of no previous work.
The completed ownership screen covered 24 clone roots, 367 available directories
and 2392 permitted U3C2 files, excluding 15 reserved paths before reading.
Its 179 matching references reduced to 16 unique basename/content pairs:
predecessor stops, proposals and the existing entry4 contract. No distinct
implementation or active reservation was found in that available scope.
This does not establish absence outside the searched scope.

The source identity audit reuses the entry4 static map reviewed at
`4313dcbbc6e981477f252e1f7ddacab2c4e9931c`, and the source/loaded-image
guards and synthetic metadata tests from the merged +05E7 load at
`fa5a156a0706421ec64a186ba7844ef2e34fb8c2`.
The explicit AF policy, whole-CPU checks and independent arithmetic-reference
approach follow reviewed +05D4 evidence at
`672b49cf95b56970f0ed223360ebe9a719819ca6`.

## Validation

Each branch passed 11 synthetic metadata tests, original-media and entry4
hash checks, exact single-instruction decode, a constructed 1 MiB code-image
match, and rejection after changing an instruction byte. A code-image match
proves instruction identity only; image origin and frame equality are unknown.

The focused C test passed strict C11 with
`-Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined`.
Commands ran serially under `nice -n 19 timeout 20s`; no emulator, Unicorn,
original-instruction execution, native oracle, configure or broad build ran.
A hash-bound source-membership audit counted 254/254 recovered C sources.
Metadata-only CMake checks counted 297 CTest registrations and 309 source paths,
zero literal DEPENDS setters, and an acyclic graph.
Notice checksums, oracle-script syntax-only checks and diff hygiene passed.

## Remaining gates

These are separate supplied-state fixtures, with no load/compare producer join,
conditional execution, caller-frame parity, natural game input or whole-function
claim. Strict full-C accounting remains unchanged at the reviewed floor of 82;
the whole-game denominator stays open. Native original/C differential evidence
is still missing while the host/oracle hold applies.

The queued source successors are +05F6 JNE (stops +05F8 or +0600) and
+05F8 stack-byte load (stops +05FB). They require their own ownership and byte
gates before implementation. This document does not reserve them.

Exact committed heads go to PM before SSH publication. Do not self-merge.
