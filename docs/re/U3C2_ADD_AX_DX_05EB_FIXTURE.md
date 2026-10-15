# U3C2 +05EB supplied-state ADD AX,DX gate

ADD AX,DX only: AX receives the 16-bit sum; CF/PF/AF/ZF/SF/OF are defined
by addition, other FLAGS and CPU words/full RAM remain unchanged. IP advances
to +05ED; stop before the byte store. Independent supplied AX/DX/FLAGS may
include wider synthetic states; no CBW producer or original frame is inferred.

    pr_u3c2_add_ax_dx_05eb_status pr_u3c2_add_ax_dx_05eb_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu,
        pr_u3c2_05eb_af_policy af_policy);

Mandatory PR_U3C2_05EB_AF_ADD_DEFINED=1. RAM supplies at least 1 MiB; CPU
is the distinct shared 14-word pr_u6c_3f31_cpu. Code immutable; async/debug
traps excluded. Null/short RAM/CPU, wrong IP or policy 0/2/255/-1 reject
atomically. AF is defined by addition; upstream opaque AF is not joined to
any producer evidence. No successor store, frame join or return is supplied.

Reviewed independent reference uses signed 32-bit range overflow, low-nibble
carry and bit-count parity, distinct from implementation formulas. Full CPU
expectations pass: 524,288 AX-by-eight-DX-edge cases; 524,288 DX-by-eight-
CBW-shaped-AX cases; 2,048 all-CBW-shaped-AX-by-eight-DX-edge cases; all
65,536 FLAGS at FFFF+1. Eight full 1 MiB RAM snapshots and aggregate sweep
RAM equality pass, plus atomic invalid argument/policy checks. These are
bounded edge/sweep checks, not Cartesian full-state coverage. Eleven loaded-
code metadata tests, exact source/constructed-image identity/mismatch checks,
strict C11 warnings-as-errors ASan/UBSan, hash-bound source membership 231/231,
full public hygiene/notices/oracle shell syntax pass. CMake adds only this
fixture source/test while retaining every main registration.

GAME.OVR: 206105 bytes, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Entry4 file [196813,197723), SHA256
59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae.
The source audit pins media/body/exact instruction and 16-bit decode.
Optional paired --loaded-image IMAGE --loaded-cs 0x1404 compares instruction
bytes only in a supplied 1 MiB image with A20 physical wrapping. No other
RAM equality, image provenance, producer consistency or original-frame join
is inferred. Synthetic constructed images pass exact instruction checks;
one flipped instruction byte rejects with CLI exit 1. Media, images, decoded
evidence, binaries and logs stay under ignored resources/.

Both branches start from fixed f02a70acf2b695c51a2246019b9a38b345499458.
PM i6141 assigns the disjoint local work; i6161/i6162 record interfaces.
Bounded deja "U3C2 POP BP metadata ADD AX DX 0905 05EB" completed exit 0 in
3.496 seconds, no matches across zero indexed sessions; no indexed finding
reused. Reused metadata helper b508f426acfcaca737a417eb8607e71526da993a,
ADD source/reference tests 672b49cf95b56970f0ed223360ebe9a719819ca6,
byte/loaded-image helper 4ef6fb67332cef20ba547e45a44622faae602cdc,
owned map 4313dcbbc6e981477f252e1f7ddacab2c4e9931c and hash-bound source
membership checker 67b8b3e20801db497703cbf446eab424ed858a96.

Before edits: 643 enumerated /341 available directories, 1039 permitted files,
41 inspected predecessor stop/source/test/audit/lead refs, zero errors.
Next scan: 648 enumerated /346 available directories, 1219 files, 12 inspected
current-stop and owned closure-contract refs, zero errors. +0906 is already
reserved by that contract as an unresolved return-policy boundary; no free
runtime candidate is claimed. No duplicate +05ED implementation/reservation
found in available permitted scope; hidden external ownership not inferred.
Reserved entry1/+002E and U3C6 paths excluded before content search. +05BE
is untouched. No RETF policy selection, popped frame values or native/RETF/
strict full-C credit; no accounting/denominator change. Source-only bounded
nice 19 /20-second checks run serially; no broad build/full CTest, configure,
emulator or original/native/oracle execution. Local exact-head review and
publication/CI remain pending. Earlier reviewed heads and root/setup/prepared-
only unexecuted 446-case manifest are preserved.

ASan/UBSan: elapsed=1.51 user=1.30 system=0.04 maxrss_KiB=11512 exit=0.
Instruction file [196925,196927), 2 bytes, SHA256
dc98dcbca7373c462b8ddafeaf7590bc8549185d60dd109e82f5573804afb6e5.

Next unimplemented +05ED byte store SS:[BP-3]=AL only, independent supplied
state, stop before +05F0. File [196927,196930), SHA256
be7e3c08b70995be17c8c4a7c48e719b5756bc9b2bdad5c36d3cb07320ea26ba. No reservation or implementation here.

Run python3 tools/overlay/u3c2_add_ax_dx_05eb_static_audit.py --root OWNER_CLONE
with Capstone 5; optional paired --loaded-image IMAGE --loaded-cs 0x1404;
--output writes JSON. Metadata tests: python3 -m unittest discover
-s tests/overlay -p test_u3c2_add_ax_dx_05eb_static_audit.py.

Serial restack after PR555: fixed parent 2490704954273acbc34a1079c25e7599f8b3ecaa
under PM i6394. Header, implementation, C test, static audit and Python tests
remain byte-identical to reviewed 49d8bfbf59f3e7b13845d3d761e52ab63fb1bc4b.
Only additive CMake registrations and this appended provenance differ.
Current hash-bound source membership is 242/242; the earlier 231/231 result
above belongs to the original base. All 11 metadata tests, exact source/
constructed-code-image positive and flipped-byte rejection, strict C11
warnings-as-errors ASan/UBSan, public-tree hygiene/notices/oracle syntax
pass. Restack sanitizer: elapsed=1.51 user=1.35 system=0.03 maxrss_KiB=11500 exit=0.
Merged static QA reports 282 CTest registrations, 291 source paths, zero
literal DEPENDS setters and an acyclic graph. Foundation checker sources
remain unchanged. CMake preserves every parent source and executable
registration, adding only this fixture. Checks run serially at nice 19 with
20-second limits; no configure, broad build/full CTest or native/oracle run.

Bounded deja "U3C2 05EB ADD AX DX restack" completed exit 0 with no matches
across zero indexed sessions; no indexed finding reused. Reviewed ADD
source/reference tests 672b49cf95b56970f0ed223360ebe9a719819ca6 and the
owned map/helper/membership references above retain their bounded scope.
The new cross-clone ownership refresh timed out at 20 seconds and is not
a complete search. A narrower current-main plus owned contract/original
fixture screen checked 94 permitted files in three directories; inspected
references describe predecessor stops and the existing +0906 reservation.
External ownership is not resolved. Two separate proposed follow-ons are
+0906 source/input metadata only (reserved return boundary; no policy or
transition) and +05ED supplied-state byte store stopping before +05F0.
No follow-on implementation or new reservation is made by this restack.
Prepared for exact clean pre-SSH PM review; no publication or remote CI
claimed here. Original reviewed heads and the unexecuted prepared-only
446-case manifest remain preserved. Zero native/RETF/strict-full-C credit.
