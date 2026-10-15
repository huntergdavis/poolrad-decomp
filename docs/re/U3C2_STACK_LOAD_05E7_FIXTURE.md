# U3C2 +05E7 supplied-state load gate

MOV AL,SS:[BP+0C] only; effective offset wraps to 16 bits before
SS base and A20 addressing. Stop before +05EA. This load does not execute the following CBW or establish predecessor reach.

    pr_u3c2_stack_load_05e7_status pr_u3c2_stack_load_05e7_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05e7_af_policy af_policy);

Mandatory PR_U3C2_05E7_AF_PRESERVE_SUPPLIED=1.

RAM supplies at least 1 MiB; the CPU is the distinct shared 14-word
pr_u6c_3f31_cpu. Code is immutable; asynchronous/debug traps are excluded.
AH, all other CPU words, all FLAGS and full RAM remain unchanged. Only AL
and IP change. Invalid RAM/CPU pointers, short RAM, entry IP or policies
0/2/255/-1 reject before mutation. Mandatory AF preservation policy 1
retains the supplied synthetic bit; upstream native AF remains unknown.
Independent supplied states do not establish producer reach, original-frame
consistency, a result or successor effects. Zero native/original/RETF/strict
full-C credit; no accounting or denominator change. Entry1/+002E, U3C6 and
+05BE remain outside scope; RETF wrap candidates remain unresolved.

PASS 550 full 14-word CPU/1 MiB RAM expectations: all 256 loaded byte values
with two supplied FLAGS patterns (512), 30 BP/SS boundary cases, and eight
synthetic data/code/stack address cases. The adjacent byte differs and all
RAM stays unchanged. A further 65,536 BP sweep uses nonperiodic RAM and
coupled SS, AX and FLAGS, covering every AX/FLAGS word and every AH value;
full CPU expectations and aggregate full RAM equality pass. This is bounded
coupled coverage, not Cartesian full-state coverage. Invalid argument and
policy cases preserve CPU/RAM on rejection.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
The audit locks media/body/exact instruction and the 16-bit decode shape.
Optional paired --loaded-image IMAGE --loaded-cs 0x1404 checks instruction
bytes only in a supplied 1 MiB image, with A20 physical wrapping. Invalid
image/CS/IP/width, offset crossing and unpaired inputs reject. No other RAM
equality, provenance, original-frame join or producer consistency is inferred.
Eleven synthetic metadata tests pass: exact/no mutation, each mismatch,
wrong position, A20 crossing, invalid sizes/CS/IP/width, paired inputs and
unrelated RAM differences. The CLI accepts a constructed image and rejects
one flipped instruction byte with exit 1. No original frame/producer was
captured or executed. Media, constructed RAM, decoded evidence, binaries and
logs stay under ignored resources/.

Strict C11 warnings-as-errors ASan/UBSan, exact byte gates and hash-bound
source membership 214/214 pass on fixed parent
73c4c0aac5203d34e61a7a134d5fc84c65b7f257. CMake adds only this source/test,
retaining merged +05B5, +05AF and all resident/U3B0 registrations. Serialized
nice 19 processes use 20-second caps; no heavy build, CMake configure, full
CTest, emulator or native/oracle execution. Local review heads must be sent
before any publication; exact-head approval and required CI remain pending.
PM i5906 assigns these next local prefixes after ownership screening.

Bounded deja query "U3C2 0900 05E7 load AL supplied AF" timed out at
20.139 seconds (exit 124), supplying no usable indexed finding. Reused
reviewed stack-load raw guards, CPU expectations, byte/wrap/reference tests
24e801184e107564df6e13473d9875eb0e811293, adapted only to the assigned
address/displacement/stop and boundary cases; byte/loaded-image helper
4ef6fb67332cef20ba547e45a44622faae602cdc; owned entry4 map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c; membership checker
67b8b3e20801db497703cbf446eab424ed858a96. Upstream XOR AF is undefined;
reviewed synthetic preservation 6d8bd10018747ddaecf0d9e8911198a153b44a1e
does not select a native AF value or establish a producer/frame join.

Before edits, ownership screen: 619 enumerated / 317 available directories,
379 permitted files, 52 inspected predecessor stop/target/test/audit,
closure-map/numeric byte-count and next-lead refs, zero errors. Next-lead
screen: 624 enumerated / 322 available directories, 427 permitted files,
21 inspected proposed-next docs/current stop metadata, zero errors.
No duplicate implementation or reservation found in either permitted local
scope; hidden external ownership is not inferred. Reserved paths excluded
before content search. Earlier reviewed heads, root/setup and the prepared-only
unexecuted 446-case manifest are preserved.

BP=FFF3/FFF4/FFF5 covers offsets FFFF/0000/0001; SS=FFFF with BP=3/4/5
covers physical FFFFF/00000/00001.
ASan/UBSan: elapsed=0.70 user=0.46 system=0.04 maxrss_KiB=10428 exit=0.
Instruction file [196921,196924), 3 bytes, SHA256
74d3cf8eaa53746dbad6b7bd6f2fb6131a6354699c407927f5a84c56f7d061b3.

Proposed next lead, unimplemented/unreserved in screened local scope:
+05EA CBW only (opcode 98 in 16-bit mode), independent supplied AX/FLAGS;
stop before +05EB. No producer/native AF consistency is inferred.
File [196924,196925), SHA256
79bec7ff3e69d1b470eb8684f2005a867a7980222474cbc1afd26a9a4f206d4a.

Run tools/overlay/u3c2_stack_load_05e7_static_audit.py --root OWNER_CLONE
with Capstone 5; optional paired --loaded-image IMAGE --loaded-cs 0x1404;
--output writes JSON. Metadata tests: python3 -m unittest discover
-s tests/overlay -p test_u3c2_stack_load_05e7_static_audit.py.

## Current-main focused restack after PR #518

This separate one-commit branch uses 1d3ffc8be575923eea5e54da2375f56dda0c472f, after +0900
integration. Header, C source, C test, static audit and metadata tests remain
byte-identical to reviewed 38960e111f292257d46f4be89ec7e2c4ddb85e61; prior heads
are preserved. CMake retains every main source/target and adds this fixture
only. Fresh eleven metadata tests, source/constructed-image identity,
flipped-byte rejection, strict C11 ASan/UBSan (elapsed=0.47 user=0.43 system=0.03 maxrss_KiB=10612 exit=0),
hash-bound membership 224/224, full public-tree hygiene, notices
and oracle shell syntax pass. Earlier membership results describe earlier
bases; current main has 223 recovered C files plus this fixture.
Bounded deja "U3C2 05E7 restack" completed exit 0 in 3.153 seconds, no matches
across zero indexed sessions; no indexed finding reused. The reviewed gate
and additive registration are reused directly. Independent supplied-state
scope, synthetic AF policy and stop before +05EA are unchanged. No CBW or
successor effect, producer/frame join, native/full-C credit, accounting or
denominator change. No broad/native execution.
