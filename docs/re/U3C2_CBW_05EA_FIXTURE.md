# U3C2 +05EA supplied-state gate

CBW only in 16-bit mode: AX sign-extends supplied AL; IP advances to +05EB.
Stop before ADD AX,DX. No successor arithmetic or producer reach is established.

    pr_u3c2_cbw_05ea_status pr_u3c2_cbw_05ea_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05ea_af_policy af_policy);

Mandatory PR_U3C2_05EA_AF_PRESERVE_SUPPLIED=1.

RAM supplies at least 1 MiB; the CPU is the distinct shared 14-word
pr_u6c_3f31_cpu. Code is immutable; asynchronous/debug traps are excluded.
Full RAM, all FLAGS and every other CPU word remain unchanged. Mandatory
host AF policy 1 retains the supplied synthetic bit; upstream native AF
remains unknown. Null RAM/CPU, short RAM, wrong IP and policies 0/2/255/-1
reject atomically. Independent supplied states establish no predecessor
reach, original frame, successor effect or return result. Zero native,
original, RETF or strict full-C credit; no accounting/denominator change.
Entry1/+002E, U3C6 and +05BE remain outside scope. Neither RETF wrap candidate
is selected; no popped frame values or stack-read policy are inferred.

PASS all 65,536 FLAGS with full CPU expectations; supplied AF 0/1 each
occurs 32,768 times. Eight full 1 MiB RAM snapshots, aggregate sweep RAM
equality and atomic invalid argument/policy checks pass. These bounded
sweeps do not establish Cartesian full-state coverage.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
The audit locks media/body/exact instruction and the 16-bit decode shape.
Optional paired --loaded-image IMAGE --loaded-cs 0x1404 compares instruction
bytes only in a supplied 1 MiB image with A20 physical wrapping. Invalid
image/CS/IP/width, offset crossing and unpaired inputs reject. No other RAM
equality, image provenance, original frame or producer consistency is inferred.
Eleven synthetic metadata tests pass: exact/no mutation, each mismatch,
wrong position, A20 crossing, invalid sizes/CS/IP/width, paired inputs and
unrelated RAM differences. The CLI accepts a constructed image and rejects
one flipped instruction byte with exit 1. No original producer/frame was
captured or executed. Media, constructed images, decoded evidence, binaries
and logs stay under ignored resources/.

Strict C11 warnings-as-errors ASan/UBSan, byte gates and hash-bound source
membership 227/227 pass on fixed parent
97a1ef0990f5ad6cb570b40a94d866c63452af2b (226 main sources plus this fixture).
CMake retains every main source/target and adds only this fixture source/test.
Checks run serially with nice 19 and 20-second caps. No heavy build, CMake
configure, full CTest, emulator or native/oracle execution. Exact local heads
must be sent before publication; PM exact-head review and required CI remain
pending. PM i6077 assigns the two distinct local gates; i6098/i6099 record
interfaces and the exact base.

Before edits, bounded deja "U3C2 MOV SP BP CBW 0903 05EA" completed exit 0 in
4.762 seconds, no matches across zero indexed sessions; no indexed finding
reused. Reused reviewed MOV word raw guards/full-state reference tests
699ac8593505d5b6563cf57c6906c174384cec63, adapting DX/AX to SP/BP and the
entry/stop; reviewed CBW source/tests/canonical 16-bit decode policy
bf2b15a02a4fe5c6a5857318f1ca6fce85b9d356, adapting entry/stop only; byte helper
4ef6fb67332cef20ba547e45a44622faae602cdc; owned entry4 map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c; membership checker
67b8b3e20801db497703cbf446eab424ed858a96. Reviewed synthetic preservation
6d8bd10018747ddaecf0d9e8911198a153b44a1e retains opaque supplied AF after
upstream XOR without selecting a native outcome or joining a producer.

Before edits: 639 enumerated / 337 available directories, 829 permitted files,
145 inspected predecessor stop/source/test/audit/next-lead references, zero
errors. Next-lead scan: 641 enumerated / 339 available directories, 922 files,
29 inspected next-lead docs/current stop metadata, zero errors. No duplicate
implementation or reservation found in either available permitted local
scope; hidden external ownership is not inferred. Reserved paths excluded
before content search. Earlier reviewed heads and the root/setup/prepared-only
unexecuted 446-case manifest remain unchanged.

All 65,536 AX values also pass full CPU checks. The static audit pins
one-byte opcode 98 to canonical CBW in CS_MODE_16, including decode-mode metadata.
ASan/UBSan: elapsed=0.10 user=0.08 system=0.02 maxrss_KiB=11612 exit=0.
Instruction file [196924,196925), 1 bytes, SHA256
79bec7ff3e69d1b470eb8684f2005a867a7980222474cbc1afd26a9a4f206d4a.

Next disjoint lead (unimplemented/unreserved in screened local scope):
+05EB ADD AX,DX only: proposed independent supplied-state arithmetic, all
six arithmetic flags defined and other FLAGS retained; stop before +05ED
store. No producer/frame join. Not implemented or reserved.
File [196925,196927), SHA256
dc98dcbca7373c462b8ddafeaf7590bc8549185d60dd109e82f5573804afb6e5.

Run tools/overlay/u3c2_cbw_05ea_static_audit.py --root OWNER_CLONE
with Capstone 5; optional paired --loaded-image IMAGE --loaded-cs 0x1404;
--output writes JSON. Metadata tests: python3 -m unittest discover
-s tests/overlay -p test_u3c2_cbw_05ea_static_audit.py.

## Separate restack after merged +0903 and QA foundation

This one-commit branch uses be0c138f03f07531c327eba3888a67ffd7fb4b70, containing merged +0903
and QA foundation 75de529083b0adc8a1950f59bbc0da624d3cb697. Header, C
source, C test, static audit and metadata tests remain byte-identical to
reviewed 7fccd2f5a609bf672386f321c65f8c2bd2ef512e; previous heads are preserved.
CMake keeps every main source/target and QA registration, adding this fixture
only. Fresh eleven metadata/source/constructed-image/mismatch checks, full
CPU/RAM sweeps, strict C11 ASan/UBSan (elapsed=0.22 user=0.16 system=0.03 maxrss_KiB=11628 exit=0),
hash-bound recovered source membership 236/236, full public-tree
hygiene/notices/oracle syntax pass. Unchanged merged static QA chain passes
276 CTest registrations, 285 source paths, zero literal DEPENDS setters and
an acyclic graph. No configure, broad CTest or original/native execution.
Bounded deja "U3C2 05EA CBW restack" completed exit 0 in 4.644 seconds, no
matches across zero indexed sessions; approved gate and QA checks reused
directly. Source counts above describe earlier bases. Independent-state
scope, supplied AF policy, stop before +05EB and zero native/full-C credit
remain unchanged.
