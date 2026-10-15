# U3C2 +05E4 supplied-state gate

CBW in explicitly pinned 16-bit mode sign-extends supplied AL into AX.
AL<128 gives AH=0, otherwise AH=255; only AX/IP change. All FLAGS, other CPU
words and RAM retained. Stop before +05E5 MOV DX,AX, without successor effects.

    pr_u3c2_cbw_05e4_status pr_u3c2_cbw_05e4_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05e4_af_policy af_policy);

Mandatory PR_U3C2_05E4_AF_PRESERVE_SUPPLIED=1 explicitly retains supplied AF;
no native value inferred. Null/short RAM/CPU, wrong IP and unsupported policy
reject unchanged. The audit canonicalizes one-byte opcode 98 as CBW in mode 16,
even if Capstone labels it CWDE; no 32-bit widening is implemented.

PASS all 65,536 FLAGS and all 65,536 AX with complete 14-word CPU expectations;
AF=0/1 retained in 32,768 FLAGS cases each. Eight full 1 MiB RAM/CPU snapshots,
unchanged loop RAM, policies 0/2/255/-1 and invalid arguments pass. ASan/UBSan
elapsed 0.16s, user 0.12s, max RSS 11,324 KiB.

Slice file [196918,196919), 1 bytes, SHA256
79bec7ff3e69d1b470eb8684f2005a867a7980222474cbc1afd26a9a4f206d4a.

## Domain, byte evidence and reuse

RAM supplies at least 1 MiB; CPU is the distinct shared 14-word
pr_u6c_3f31_cpu. Code immutable; asynchronous/debug exceptions excluded.
Independent supplied states do not assert predecessor consistency, original
frames, successor execution or a result. No gap join, native/original/RETF/
full-C credit, accounting or denominator changes. Reserved entry1/+002E,
U3C6 and +05BE untouched; RETF address candidates remain unresolved.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
The audit pins media/body/exact instruction and 16-bit decode shape, optionally
comparing instruction bytes in a supplied 1 MiB image at explicit CS:IP with
A20 physical wrapping. Image/CS must be paired; invalid image/CS/IP/width or
instruction offset crossing reject. No other RAM equality, image provenance,
producer reach or original-frame join is inferred. Validation uses ignored
constructed images containing only the bounded instruction bytes; no actual
game/original frame or CPU producer was captured, loaded or executed.
One flipped loaded byte rejects with CLI exit1. Eleven synthetic metadata
tests pass: exact/no mutation, each mismatch, wrong position, A20 crossing,
invalid sizes/CS/IP/width, paired inputs and unrelated RAM differences.
Media, RAM images, decoded evidence, binaries and logs stay under ignored
resources/. Strict C11 warnings-as-errors ASan/UBSan, byte locks and hash-bound
source membership 210/210 pass. Serialized nice19/20-second caps; no heavy
build, CMake configure/full CTest, emulator/original/native/oracle execution,
push, PR or merge during this local preparation. PM i5785 reports owner #491
restoring GitHub flow with PM exact-head approval and required CI; exact
local heads must be sent before any push. Publication is pending that review;
heavy/native holds remain.

PM i5785 assigns both gates; milestones i5792/i5793 record interfaces and
parent 217e809448be096fcc48b2413714856f5b165284 (fixed local main; no outgoing
refresh). Before edits: 592 enumerated /292 available directories, 264 permitted
files, 83 predecessor/lead endpoint refs, zero errors. Next-lead scan: 598
enumerated /298 available directories, 272 permitted files, 14 current/
predecessor endpoint refs only, zero errors. Reservations excluded before
content search. No duplicate implementation/reservation in available local
scope; hidden external ownership not inferred, entry4 remains PM-owned.
All 83 pre-edit matches were classified as 27 reviewed documentation
endpoint/lead refs and 56 stop/target/test metadata; no implementation entry,
entry-guard or old reservation identified. Next-lead 14 matches are current/
predecessor endpoints only, without a duplicate implementation/reservation
in that available scope. Full classification is retained privately.
Bounded deja query "U3C2 05B1 zero store CBW supplied AF" completed exit 0
in 2.255s, no matches across zero indexed sessions; no indexed finding reused.
Reused reviewed byte-store raw addressing/reference tests
7d52ee6897a38a5b6d5d353361150d55c7fe6960, adapted to BP-1 and immediate zero
with destination contents independent of AX; CBW source/FLAGS/AX tests and
16-bit opcode 98 decode policy 96d0b0fe74e9dbf720012463e07518f36360f899;
predecessor JLE 6fdc1f388bbe266aad6968ff0752046b88305e2a and table load
df4f969b1c264241fad4bd209945eec874324031; byte-audit helper
4ef6fb67332cef20ba547e45a44622faae602cdc; owned map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c; shared CPU ABI and approved
membership checker 67b8b3e20801db497703cbf446eab424ed858a96.
Upstream XOR AF remains undefined; explicit synthetic preservation convention
is reviewed at 6d8bd10018747ddaecf0d9e8911198a153b44a1e. These gates retain the
supplied bit without selecting a native AF result or joining a producer.
Held heads and prepared-only unexecuted 446-case manifest remain unchanged.

Next lead, unimplemented/unreserved: +05E5 MOV DX,AX only;
input independent supplied post-CBW AX/FLAGS; explicit synthetic supplied AF. Stops before 05E7.
File [196919,196921), SHA256
646806b9f5f977d0d079c0450de6b2810d4bb4c647dd287306b9eaf3a4b05573.

Run tools/overlay/u3c2_cbw_05e4_static_audit.py --root OWNER_CLONE
with Capstone 5. Optional --loaded-image IMAGE --loaded-cs 0x1404 compares
only instruction bytes; --output writes JSON. Focused metadata tests:
python3 -m unittest discover -s tests/overlay -p test_u3c2_cbw_05e4_static_audit.py.

## Additive restack after PR #498

Original reviewed local head da3cc3a79b5ab37e63d422145f77cf77647f7d7b remains unchanged.
This separate branch starts from c80185b0220c2627487eea35906f54a73d3272ff.
Header, C source, focused C test, static audit and eleven metadata tests are
byte-identical to that reviewed head. CMake adds this leaf while preserving
merged +05AF source and target. Fresh focused gates pass: eleven metadata
tests, exact source/constructed loaded-image check, flipped-byte rejection,
strict C11 ASan/UBSan (elapsed=0.20 user=0.12 system=0.04 maxrss_KiB=11384 exit=0) and hash-bound source membership 211/211.
Earlier 210/210 results above describe the original 217e base only.
PM i5877 asks that this c801 head be preserved and restacked separately on
e337afc before publication. Exact-head review and required CI remain pending.
No native/full-C credit or count changes.

## Current-main restack after PR #496

PM i5877 requires publication from e337afcce347b453c5379a1930f179a8c8caa0e0.
This separate branch uses that parent. Preserved c801 review head:
f237f9f5d60c4672e9ce2142e41c779c89d463e1. Header, C source, C test, static audit and
eleven metadata tests remain byte-identical. Only additive registration
and this provenance note differ. CMake retains PR #498 +05AF source/target
and PR #496 resident first-call test registration. Fresh focused checks pass:
eleven metadata tests, exact source/constructed-image gate, flipped-byte
rejection, strict C11 ASan/UBSan (elapsed=0.19 user=0.13 system=0.04 maxrss_KiB=11324 exit=0), and
hash-bound source membership 211/211. Local scope evidence is unchanged
and reused per i5877. Exact-head review before push and required CI are
pending. No publication, native/full-C credit or count change.

## One-commit publication restack after PR #511

PM i5947/i5969 approve this focused restack on c82c3d049d016361e1b95490e81bdbf3a07adb24.
Header, C source, C test, static audit and eleven metadata tests remain
byte-identical to 6278c1c13ba9fdb325d4fe4291c67bf80616e7c0; prior heads are preserved.
CMake retains every main source/target and adds only this fixture.
Fresh focused checks pass: eleven metadata tests, source/constructed-image
identity and flipped-byte rejection, strict C11 ASan/UBSan (elapsed=0.16 user=0.11 system=0.03 maxrss_KiB=11612 exit=0),
hash-bound source membership 219/219, public-tree hygiene,
notices and oracle shell syntax. Earlier membership results describe their
earlier bases. Current main contains 218 recovered C files; this fixture
adds one.
Bounded deja query "U3C2 CBW 05E4 restack" completed exit 0 in 4.262 seconds,
no matches across zero indexed sessions; no indexed finding reused. The
approved gate and additive registration are reused directly. No broad/native
execution, behavior, native/full-C credit, accounting or denominator change.
+0900/+05E7 local review heads remain separate.
