# U3C2 +0905 POP BP source/input audit

Metadata requirements only. There is no POP execution fixture, CPU object,
stack lookup, decoded BP value or SP/IP transition. The source instruction
end is +0906 as an address fact; reach of that return is not established.

    required_stack_addresses(ss, sp)

Optional paired --ss/--sp accept supplied 16-bit integers; bools, out-of-range
and noninteger API values reject. Under the two prior frame hypotheses,
wrapped_offset_word_starts and contiguous_physical_frame, the first word
has the same input byte addresses: physical SS*16+SP modulo 1 MiB, followed
by the physically adjacent byte modulo 1 MiB. This adopts the prior audit's
unexecuted word-byte input convention. It does not validate native POP
accesses or resolve a multiword RETF policy. Both candidates are reported;
selected_policy is null and native_wrap_policy_resolved remains false.
No stack bytes at the reported addresses are queried. Optional image reads
check the source instruction only, without correlating the image to a frame.

Seventeen synthetic tests pass: eleven loaded-code metadata cases and six
stack-input/domain/no-effect/pairing tests. Literal expected addresses cover
ordinary inputs, SS:FFFF word straddling, physical A20 crossing and SS=FFFF /
SP=FFFF. Candidate agreement for one word supplies no runtime policy proof.
CLI/API missing SS/SP reject before media access. Positive source/code-image
plus supplied SS/SP output confirms stack_contents_read, pop_value_read,
cpu_transition_evaluated and execution_fixture_implemented are all false.
CMake is unchanged; baseline hash-bound source membership 230/230 passes.
This is a three-file tool/test/document audit with no C source/header/test.

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

Instruction file [197719,197720), 1 byte, SHA256
cfae0d4248f7142f7b17f826cd7a519280e312577690e957830d23dcf35a3fff.

+0906 remains the existing reserved RETF source/input boundary. Any further
audit requires ownership coordination; runtime effects remain held behind
the unresolved native-wrap gate. No +0905 predecessor transition is supplied.

Run python3 tools/overlay/u3c2_pop_0905_input_audit.py --root OWNER_CLONE
with Capstone 5; optional paired --ss SS --sp SP and independent paired
--loaded-image IMAGE --loaded-cs CS; --output writes JSON. Focused tests:
python3 -m unittest discover -s tests/overlay -p test_u3c2_pop_0905_input_audit.py.

Serial restack after PR549: this three-file audit is based on
7865cf2a80847794a7361c05497c2848c8f444ca under PM i6347. Tool and tests
remain byte-identical to reviewed fef0b1622a3b4cb77b2123cbb4b9a8678fd5f61c;
only this validation provenance is appended. CMake and all recovered C
sources remain unchanged. The current baseline hash-bound source membership
is 239/239; the earlier 230/230 result above describes the original base.
Seventeen metadata tests, exact source/constructed-code-image check,
flipped-byte rejection, merged static QA foundation chain, full public-tree
hygiene, notices and oracle shell syntax pass in bounded serial runs.
Static QA reports 279 CTest registrations, 288 source paths, zero literal
DEPENDS setters and an acyclic graph. Foundation checker sources are
unchanged from the parent; this is
script-mode metadata QA, with no configure, CTest build or native execution.
Bounded deja "U3C2 0905 POP BP metadata restack" completed exit 0 with no
matches across zero indexed sessions. No indexed result was reused; the
reviewed helper/map/membership references above retain their scope. The
original +0905 and independent +05EB reviewed heads are preserved, and
+05EB remains local. This restack is prepared for exact pre-SSH PM review;
no remote publication, CI, native or full-C credit is claimed.
