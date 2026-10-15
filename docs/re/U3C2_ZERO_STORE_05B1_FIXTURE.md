# U3C2 +05B1 supplied-state gate

MOV byte SS:[BP-1],0 only. Effective BP-1 wraps16 before SS/A20 addressing.
Only the destination byte/IP change; AX, all other CPU words and all FLAGS
retained. Stop before +05B5 JMP; no restore, RETF or return result.

    pr_u3c2_zero_store_05b1_status pr_u3c2_zero_store_05b1_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05b1_af_policy af_policy);

Mandatory PR_U3C2_05B1_AF_PRESERVE_SUPPLIED=1 retains supplied synthetic AF
explicitly; native value unknown. Null/short RAM/CPU, wrong IP and unsupported
policy reject before mutation. Independent supplied range-reject state only.

PASS 550 full 14-word CPU/1 MiB RAM expectations: all 256 initial destination
bytes with two fixed AX/FLAGS patterns (512), 30 distinct BP/SS wrap states and
eight data/stack address cases. Destination contents vary independently of AX;
boundary/alias destinations start nonzero, neighboring bytes remain untouched.
BP=0/1 covers underflow/zero; SS=FFFF with BP=15/16/17 covers A20 offsets 14/15/16.
All 65,536 FLAGS and all 65,536 BP full-CPU cases pass; coupled BP sweep covers
all AX words over nonperiodic RAM. Full loop RAM matches accumulated expected
zero stores, without Cartesian state coverage. Policies0/2/255/-1 and invalid
arguments pass atomic checks. ASan/UBSan elapsed 0.48s, user 0.45s, RSS 10,568 KiB.

Slice file [196867,196871), 4 bytes, SHA256
7d66dcc7f938e8e81df61c6f4c59d23074f9b887db8844849e126dda0eb3c357.

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

Next lead, unimplemented/unreserved: +05B5 JMP +0900 only; stop before target effects;
input independent supplied post-zero-store state; all FLAGS retained. Stops before 0900.
File [196871,196874), SHA256
6cc5b91b8132fc426a66c98c6ba9eb2ba5e7f666b8d12718986b52543da7cc26.

Run tools/overlay/u3c2_zero_store_05b1_static_audit.py --root OWNER_CLONE
with Capstone 5. Optional --loaded-image IMAGE --loaded-cs 0x1404 compares
only instruction bytes; --output writes JSON. Focused metadata tests:
python3 -m unittest discover -s tests/overlay -p test_u3c2_zero_store_05b1_static_audit.py.

## Additive restack after PR #498

Original reviewed local head 42157893d34e2c6d4e7049579ec8a1b79bd6c5fe remains unchanged.
This separate branch starts from c80185b0220c2627487eea35906f54a73d3272ff.
Header, C source, focused C test, static audit and eleven metadata tests are
byte-identical to that reviewed head. CMake adds this leaf while preserving
merged +05AF source and target. Fresh focused gates pass: eleven metadata
tests, exact source/constructed loaded-image check, flipped-byte rejection,
strict C11 ASan/UBSan (elapsed=0.53 user=0.48 system=0.03 maxrss_KiB=10584 exit=0) and hash-bound source membership 211/211.
Earlier 210/210 results above describe the original 217e base only.
PM i5877 asks that this c801 head be preserved and restacked separately on
e337afc before publication. Exact-head review and required CI remain pending.
No native/full-C credit or count changes.

## Current-main restack after PR #496

PM i5877 requires publication from e337afcce347b453c5379a1930f179a8c8caa0e0.
This separate branch uses that parent. Preserved c801 review head:
f37d578208af05821d15862d3991889c12c76f41. Header, C source, C test, static audit and
eleven metadata tests remain byte-identical. Only additive registration
and this provenance note differ. CMake retains PR #498 +05AF source/target
and PR #496 resident first-call test registration. Fresh focused checks pass:
eleven metadata tests, exact source/constructed-image gate, flipped-byte
rejection, strict C11 ASan/UBSan (elapsed=0.73 user=0.55 system=0.03 maxrss_KiB=10344 exit=0), and
hash-bound source membership 211/211. Local scope evidence is unchanged
and reused per i5877. Exact-head review before push and required CI are
pending. No publication, native/full-C credit or count change.

## One-commit publication restack

PM i5947 approves this focused restack on c0096a98ce724b21b07adfb61426e4d92da122b3.
The reviewed header, C source, C test, static audit and metadata tests are
byte-identical to c1e9d21718186e5c3525ceb7b1887ce5901be870; previous heads remain
preserved. CMake adds only this source/test, retaining every main registration.
Fresh focused checks pass: eleven metadata tests, source/constructed-image
byte identity and flipped-byte rejection, strict C11 ASan/UBSan
(elapsed=0.53 user=0.50 system=0.03 maxrss_KiB=10468 exit=0), hash-bound membership 217/217,
public-tree hygiene, notices and oracle shell syntax. Earlier membership
results above describe their earlier bases. No broad/native execution or
behavior, native/full-C credit, accounting or denominator change. The bounded
additive-restack recall attempt timed out at 12.074 seconds and supplied no
finding; reviewed additive registration and implementation are reused directly.
