# U3C2 +05ED supplied-state AL byte-store gate

Only SS:[BP-3]=AL is evaluated. The effective offset wraps to 16 bits before
adding SS*16 and applying A20-off physical wrapping. Exactly the destination
byte is assigned; if it already equals AL, RAM stays equal. Every other RAM
byte and CPU word except IP, including AX and every FLAGS bit, stays unchanged.
IP advances from +05ED to +05F0, before the separate AL load.
There is no ADD producer consistency, original frame join or successor effect.

    pr_u3c2_byte_store_05ed_fixture(ram, ram_size, cpu, af_policy)

Mandatory PR_U3C2_05ED_AF_PRESERVE_SUPPLIED=1 retains supplied synthetic AF
along with all other FLAGS. No upstream native AF value is inferred.
RAM supplies at least 1 MiB and is disjoint from the shared 14-word CPU;
code is immutable and async/debug traps are excluded. Null/short RAM/CPU,
wrong IP and policies 0/2/255/-1 reject atomically before mutation.

Synthetic tests pass 550 complete 1 MiB expected-RAM snapshots with full CPU
comparisons: all 256 AL values under two FLAGS patterns, including both
equal-byte and changing writes, 30 SS/BP boundary cases and eight destination
offset cases. Literal reference addresses cover BP subtraction wrapping and
physical A20 crossing. DS and ES differ from SS in the snapshot cases.
Separate sweeps cover all 65,536 incoming FLAGS, all 65,536 AL/old-byte pairs,
and all 65,536 BP and AX values in coupled supplied states. AX uses an odd
multiplier to permute all 16-bit values. Each sweep compares complete CPU
state and target bytes; aggregate RAM equals an independently updated
reference, including a nonperiodic RAM pattern for the BP/AX sweep.
These are bounded synthetic cases, not Cartesian full-state coverage.

Eleven synthetic loaded-code metadata tests, source/constructed-image
identity, flipped-byte rejection and strict C11 warnings-as-errors
ASan/UBSan pass. Sanitizer: elapsed=0.69 user=0.55 system=0.03 maxrss_KiB=10460 exit=0.
Hash-bound source membership is 246/246 (parent 245 plus this source).
CMake adds this fixture and target only, retaining parent registrations.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
Instruction file [196927,196930), 3 bytes, SHA256
be7e3c08b70995be17c8c4a7c48e719b5756bc9b2bdad5c36d3cb07320ea26ba.
Static audit pins media/body/instruction identity and 16-bit decode.
Optional paired --loaded-image IMAGE --loaded-cs CS checks instruction bytes
only; it infers no image provenance, complete RAM equality or frame join.
Original media, constructed images, decoded evidence, binaries and logs
remain under ignored resources/.

Parent 020f90d55ca50451769f9e69e4e3e740a03f449e; PM i6457/i6459 assign the
independent store contract. Reused reviewed guard/reference-test/audit
structure 75dd378198d0d2039eabd12d74d63f8f710daf62, owned source map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c and hash-bound source checker
67b8b3e20801db497703cbf446eab424ed858a96. Bounded deja
"U3C2 05ED AL byte store fixture" completed exit 0 with no matches across
zero indexed sessions; no indexed finding reused.
PM accepted the prior permitted ownership/source screen: 24 repository
roots, 662 enumerated /358 available directories, 1785 files and 53 inspected
predecessor references. No implementation/reservation found in that scope;
hidden/external ownership is not proven absent. Reserved entry1/+002E and
U3C6 paths excluded before content reads. +0906 remains reserved untouched;
no POP/return effects or native-wrap policy selection. A separate +05F0
source lead is screened separately; no successor implementation or reservation.

Checks run serially at nice 19 with 20-second bounds. No configure, broad
build/full CTest, emulator or original/native/oracle execution. No native,
RETF or strict-full-C credit; no accounting or denominator change.
Prepared for exact local PM review before SSH/PR; no publication claimed.

Merged static QA passes 286 CTest registrations, 295 source paths, zero
literal DEPENDS setters and an acyclic graph. Checker sources match the
parent; script-mode QA uses no configure or broad CTest. Public hygiene,
notices and oracle shell syntax pass.
Second lead +05F0: file [196930,196933), 3 bytes, SHA256
44f61bf8b40ff071a8d65dcf735d020732542851b4c74c6a1ccbf6425c4f1223.
Proposed independent AL=SS:[BP+0x0E] only, stop before +05F3.
Fresh permitted ownership screen: 24 clone roots, 666 enumerated /362
available directories, 2003 files and 29 inspected references, zero errors.
References describe predecessor stops and the owned entry4 plan; no distinct
implementation or active reservation found in that scope. Hidden/external
ownership is not proven absent. Bounded deja for this lead completed exit 0
with no matches across zero indexed sessions. Original source identity and
16-bit decode pass without execution. This is an unimplemented candidate,
with no new reservation, producer join, runtime claim or full-C credit.
