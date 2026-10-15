# U3C2 +05B5 supplied-state gate

JMP +0900 only. Only IP changes to +0900. Every other CPU word, all FLAGS and all RAM remain unchanged. Stop before target effects.
Input is independent supplied post-zero-store state; no upstream execution is implied.

    pr_u3c2_jump_only_05b5_status pr_u3c2_jump_only_05b5_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05b5_af_policy af_policy);

Mandatory PR_U3C2_05B5_AF_PRESERVE_SUPPLIED=1 preserves supplied
synthetic AF explicitly; native AF is unknown. Null/short RAM, null CPU,
wrong entry IP and unsupported policies reject atomically.

PASS all 65,536 FLAGS and all 65,536 AX values with full CPU expectations;
supplied AF 0/1 each occurs 32,768 times in the FLAGS sweep. Eight separate
full 1 MiB RAM snapshots and the aggregate sweep RAM comparison pass.
Policies 0/2/255/-1 and invalid arguments preserve CPU/RAM on rejection.
These bounded sweeps do not establish Cartesian full-state coverage.
ASan/UBSan: elapsed=0.19 user=0.10 system=0.03 maxrss_KiB=11428 exit=0.

Slice file [196871,196874), 3 bytes, SHA256
6cc5b91b8132fc426a66c98c6ba9eb2ba5e7f666b8d12718986b52543da7cc26.

RAM supplies at least 1 MiB; CPU is the distinct shared 14-word
pr_u6c_3f31_cpu. Code is immutable; asynchronous/debug exceptions are excluded.
Independent supplied states do not establish predecessor consistency, original
frames, successor effects or a return result. Zero original/native/RETF/strict
full-C credit; no accounting or denominator change. Entry1/+002E, U3C6 and
+05BE are outside scope; RETF wrap candidates remain unresolved.

GAME.OVR is 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Static audit pins media/body/exact instruction and 16-bit decode shape.
Optional paired --loaded-image IMAGE --loaded-cs 0x1404 compares only the
instruction bytes at explicit CS:IP with A20 physical wrapping. Invalid
sizes, CS/IP/width, offset crossing or unpaired inputs reject. No other RAM
equality, image provenance, producer reach or original-frame join is inferred.
Validation uses ignored constructed images; no original frame or producer was
captured or executed. One flipped instruction byte rejects with CLI exit 1.
Eleven synthetic metadata tests pass: exact/no mutation, each mismatch,
wrong position, A20 crossing, invalid sizes/CS/IP/width, paired inputs and
unrelated RAM differences. Media, constructed RAM, decoded evidence, binaries
and logs remain under ignored resources/.

Strict C11 warnings-as-errors ASan/UBSan, exact byte gates and hash-bound
source membership 211/211 pass on c80185b0220c2627487eea35906f54a73d3272ff
(after PR #498). CMake additions preserve the merged +05AF source and target.
Checks run serially with nice 19 and 20-second caps. No heavy build, CMake
configure, full CTest, emulator or native/oracle execution. This is local
preparation: PM i5785 requires exact-head approval before push and required
CI. PM i5877 subsequently requires separate e337afc restacks before publication;
the c801 review heads are preserved. CI and publication remain pending.

Before edits, bounded deja query "U3C2 JMP 0900 MOV DX AX supplied AF"
completed exit 0 in 2.873 seconds, with no matches across zero indexed
sessions; no indexed finding reused. Reused reviewed raw argument guards,
full CPU/FLAGS/AX tests and explicit supplied AF convention from MOV DX
8c8b53879dc49743ec20173459ff5295cdd6bee5; bounded JMP semantics from
 af598bd3603dd2a73508ea076d583279dabb4b3f (adapted from CPU-only API to
required full RAM plus explicit AF policy); byte audit and loaded-image helper
4ef6fb67332cef20ba547e45a44622faae602cdc; owned entry4 map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c; membership checker
67b8b3e20801db497703cbf446eab424ed858a96. Upstream XOR AF is undefined;
reviewed synthetic preservation 6d8bd10018747ddaecf0d9e8911198a153b44a1e
does not select a native AF value.

Initial ownership screen: 604 enumerated / 304 available directories,
274 permitted files, 18 predecessor endpoint references, zero errors.
Next-lead screen: 615 enumerated / 313 available directories, 317 files,
25 matches, zero errors. Matches are closure-map docs/numeric count false
positives, predecessor lead docs, current stops/branch destinations/test/audit
metadata. No duplicate implementation or reservation found in that local
scope; hidden external ownership is not inferred. Reservations excluded
before content search. Held heads and prepared-only unexecuted 446-case
manifest are preserved.

PM i5836 assigned this gate; i5848/i5849 record the first interfaces.
PM i5851 prompted the c801 base update; i5872/i5873 and i5881 report restacks.

Proposed next lead, unimplemented/unreserved in the screened local scope:
+0900 MOV AL,SS:[BP-1] only, independent supplied state;
stop before +0903. No restore, RETF or predecessor/frame join.
File [197714,197717), SHA256
3df97494d0295807e9fbf23dfd0176776213e1873f02336bedcfca6037a850de.

Run tools/overlay/u3c2_jump_only_05b5_static_audit.py --root OWNER_CLONE
with Capstone 5; optional paired --loaded-image IMAGE --loaded-cs 0x1404;
--output writes JSON. Metadata tests: python3 -m unittest discover
-s tests/overlay -p test_u3c2_jump_only_05b5_static_audit.py.

## Current-main restack after PR #496

PM i5877 requires publication from e337afcce347b453c5379a1930f179a8c8caa0e0.
This separate branch uses that parent. Preserved c801 review head:
8662b6b392fb844e871e8b77db973cd3f1057d2f. Header, C source, C test, static audit and
eleven metadata tests remain byte-identical. Only additive registration
and this provenance note differ. CMake retains PR #498 +05AF source/target
and PR #496 resident first-call test registration. Fresh focused checks pass:
eleven metadata tests, exact source/constructed-image gate, flipped-byte
rejection, strict C11 ASan/UBSan (elapsed=0.26 user=0.14 system=0.05 maxrss_KiB=11448 exit=0), and
hash-bound source membership 211/211. Local scope evidence is unchanged
and reused per i5877. Exact-head review before push and required CI are
pending. No publication, native/full-C credit or count change.
