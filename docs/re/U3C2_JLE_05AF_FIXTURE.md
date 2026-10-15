# U3C2 +05AF supplied-state gate

JLE from independent supplied post-CMP24 FLAGS. ZF=1 or SF!=OF selects
+05B8, otherwise +05B1. Stop before both effects. Only IP changes; every
supplied flag, all other CPU words and all RAM retained. No selector/body
join or actual producer frame is asserted. Null/short RAM/CPU or wrong IP reject unchanged.

    pr_u3c2_jle_decision_05af_status pr_u3c2_jle_decision_05af_fixture(
        uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);

PASS all 65,536 FLAGS with complete 14-word CPU expectations: 49,152 select
05B8, 16,384 select 05B1; eight ZF/SF/OF classes have 8,192 each. Eight full
1 MiB RAM/CPU snapshots, unchanged loop RAM and invalid arguments pass.
ASan/UBSan elapsed 0.09s, user 0.06s, max RSS 10,484 KiB.

Slice file [196865,196867), 2 bytes, SHA256
ce2c2b224539c43b6249e675c7a9f9b4c69823aa9081bdd5355244bc118ee9db.

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
GitHub outgoing, push, PR or merge. Local review only.

PM i5735 assigns both gates; milestones i5743/i5744 record interfaces and
parent 217e809448be096fcc48b2413714856f5b165284 (fixed local main; no outgoing
refresh). Before edits: 578 enumerated /278 available directories, 254 permitted
files, 20 predecessor/lead endpoint refs, zero errors. Next-lead scan: 586
enumerated /286 available directories, 262 permitted files, 78 current/
predecessor endpoint refs only, zero errors. Reservations excluded before
content search. No duplicate implementation/reservation in available local
scope; hidden external ownership not inferred, entry4 remains PM-owned.
The 78 next-lead matches are reviewed branch destinations, header stop
contracts, expected/invalid-IP tests, static target metadata and documentation
endpoint/lead references; no implementation entry/entry-guard or new reservation
identified in available local scope. Full classification is retained privately.
Bounded deja query "U3C2 JLE table load 273F AF" completed exit0 in6.035s,
no matches across zero indexed sessions; no indexed finding reused.
Reused reviewed JG flag-test skeleton e14040ddc10c0d997d344272ad61b0bc3f47c2fa,
inverted for JLE; table-load source/address/reference tests
82021b02680bcc183981a3ac2c162cf4c4e9e77f with base273F and adjusted boundaries;
predecessor CMP24 10d9a0a2ebbc6100018b0da214201ddeeeebd5d8 and MOV DI
b5b3ef66b8f97873a9a174898a91ce8166ed58ee; byte-audit helper
4ef6fb67332cef20ba547e45a44622faae602cdc; owned map
4313dcbbc6e981477f252e1f7ddacab2c4e9931c; shared CPU ABI and approved
membership checker 67b8b3e20801db497703cbf446eab424ed858a96.
Upstream XOR AF remains undefined; explicit synthetic preservation convention
is reviewed at 6d8bd10018747ddaecf0d9e8911198a153b44a1e. These gates retain the
supplied bit without selecting a native AF result or joining a producer.
Held heads and prepared-only unexecuted 446-case manifest remain unchanged.

Next lead, unimplemented/unreserved: +05B1 MOV byte SS:[BP-1],0;
input independent supplied range-reject successor state; all supplied FLAGS retained. Stop before 05B5.
File [196867,196871), SHA256
7d66dcc7f938e8e81df61c6f4c59d23074f9b887db8844849e126dda0eb3c357.

Run tools/overlay/u3c2_jle_decision_05af_static_audit.py --root OWNER_CLONE
with Capstone 5. Optional --loaded-image IMAGE --loaded-cs 0x1404 compares
only instruction bytes; --output writes JSON. Focused metadata tests:
python3 -m unittest discover -s tests/overlay -p test_u3c2_jle_decision_05af_static_audit.py.
