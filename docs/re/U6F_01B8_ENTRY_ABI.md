# Bounded U6F +01B8 entry and real +02AE return

This gate executes the five original instructions in U6F [01B8,01C3),
11 bytes, and composes the unchanged bounded +02AE ABI. It stops BEFORE
the +01C3 file/path setup instruction after the actual callee returns.
It does not implement the 226-byte whole caller, its text-file services,
retry/list loops, direct UI return +0239 or outer RETF4.

The near CALL +01C0 follows PUSH CS, so the callee receives a real far
frame with return +01C3. Entry's outer caller frame and four-byte argument
remain live. For ordinary non-alias frames, entry SP=S yields caller
BP=S-2, callee-entry SP=S-030C and normal +01C3 SP=S-0308. The callee RETF
reads its actual stack; an altered return frame yields TRANSFER with the
actual resulting CPU, not a fabricated normal boundary. A state already
at +01C3 also reports BOUNDARY; this stateless API does not prove history.

Missing DOS events block at loaded 131C:1821. Other +02AE errors block
BEFORE message setup +02E9; no UI callback or successful UI effect is
invented. The existing +02AE +031F continuation never substitutes for
the separate +01B8 direct UI +0239 return. Accepted DOS events obey the
existing immutable-input, complete-CPU and ordered-byte-journal contract.
Synthetic events in tests demonstrate that contract, not natural DOS
behavior, execution reachability or native parity.

Each step executes one source instruction except inherited atomic REP
and DOS events. Unsupported spans/PCs/DF and invalid or missing events
leave that step's CPU/RAM unchanged; earlier completed steps remain.
RAM is at most1MiB with linear spans fitting their segment and captured
RAM, without A20 wrap. Disjoint state/RAM/journal and audited loaded-code
identity are caller obligations. All16 CPU fields are retained; defined
SUB flags are computed and inherited undefined logic AF normalizes to0.
No retry cap is introduced and no broader guest-state domain is claimed.

Inputs: DOSv1.3 GAME.OVR206105B, SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
U6F unit base1DFE6, original entry file[1E19E,1E1A9), SHA256
1acbf172e8b73392aced0f987184f1fb31aee4dd64e1ff0c3858d594431e78be.
START.EXE65231B SHA256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Loaded U6F2A8E and SYSTEM131C are inherited audited model identities,
not new +01B8 native observations. Known original callers +0779 and
U6C+327E do not establish natural reachability of this bounded gate.

Reused evidence: [bounded02AE ABI](U6F_POSTINSERT_02AE.md),
[complete02AE caller composition](U6F_02AE_MESSAGE_FULLSTATE.md),
the unchanged u6f_02ae_abi, string0356, CBC/B98 and directory adapters;
private static packets PMi7649/i7589 (address/hash/count/dependency
evidence only). Recall query 'U6F01B8 entry frame 01C3' found no focused
indexed match; private source packets and merged code provide the proof.
Relative static clearances are distinct from PMi7663's bounded edit gate;
p5i7680 clears only the additive isolated CMake registration.

Validation: focused strict and ASan/UBSan synthetic ABI tests; isolated
CMake target/CTest; optional private original-span disassembly audit.
Tests cover frame/argument preservation, defined flags,0/5 actual callee
return, UI/DOS rejection atomicity, malformed events, segment-edge push,
generic live-frame transfer and exclusion of +0239. No original-byte
executor, emulator or heavy whole-project build is needed.

Parity target: bounded01B8 entry through supplied-service02AE return.
Signals: exact11B/5 source graph, real frame/stack state, unchanged
dependency composition and explicit blocked/transfer behavior.
Verdict: source-backed bounded composition; whole/native parity unclear.
New whole-function, native, identity and strict credit: ZERO.
