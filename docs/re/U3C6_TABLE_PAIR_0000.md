# U3C6 entry1 complete table-pair source candidate

This local draft composes the complete call-free OVL_03C6_0000 body:
[0000,0068), 104 bytes / 41 instructions, RETF12 at0065. The next private
function begins0068; the old public-entry span ending00E3 includes it and
must not be called this whole function. Body SHA256:
05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207.
GAME.OVR SHA256:
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
START.EXE SHA256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.

The prior bounded whole-function deja recall timed out without reusable
evidence. Direct source reuse: 2b287c2's ordered entry1 prologue and CPU
conventions; 719cb89's first CMP; ed6b632/3d83581's two first branch sites;
8e9871a/1126457's live-local load and MOV SP,BP. Their earlier tests do not
establish parity of this new composition. The CPU type retains the reviewed
fourteen-word AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS order.

Six aligned near callers, each preceded by PUSH CS:0176(private0127),
040C(entry5),05F0(entry7),0772(entry9),0D2A(entry16),0EEC(entry17).
U3C2 private0909 has aligned far callers092A/09D8 with segment fixups
092D/09DB resolving to U3C6 exported entry1 stub03E0:0025. Indirect/runtime
references have not been proven absent. No callee, loader callback or
dependency substitutes for any original instruction in this body.

At entry SP=S, output2 pointer is S+4/+6, output1 is S+8/+10,
x low byte S+12, y low byte S+14. The saved BP is S-2 and local S-3.
The first CMP branches around both stores if y is zero. Otherwise each
table access independently reloads x/y and computes2*x+8*y, reading DS2844
then2845 and writing output1 then output2. The final CMP reloads output1's
pointer and its byte. Nonnegative writes local1; negative retains its
current value. Output aliases can change local, input arguments, pointers,
table, saved BP or return words. Return AL can therefore be non-Boolean.
Final flags are fully defined by CMP(y,0) or CMP(readback,0); intermediate
undefined XOR/SHL AF is overwritten on every exit. Other FLAGS survive.

The host ABI takes exact1MiB shared RAM, separate14-word CPU and explicit
return policy. UNMEASURED stops before RETF at0065 with prior effects;
WRAPPED_SS and CONTIGUOUS_PHYSICAL are distinct synthetic candidates.
Neither is promoted to native truth by borrowing another function's
capture. Word/far-pointer operand bytes are physically consecutive with
A20 wrap; effective addresses and separate candidate stack pops wrap16.
Code must remain immutable. Invalid host arguments reject before effects.

No source classifier/count changes, publication, emulator or native run.
Fresh local C11 -Wall -Wextra -Wpedantic -Werror compilation and169
independent synthetic full1MiB/all14CPU snapshots passed. They cover three
semantic routes, changed live x/y/table, non-Boolean local return, reloaded
far-pointer offset/segment, saved-BP/IP/CS aliases, LES at offsetFFFF,
A20 crossing, all three explicit return profiles, FFFD/FFFE/FFFF stacks,
both incoming arithmetic flag patterns and seven invalid-host guards.
Run elapsed0.10 seconds, maxRSS3760KiB, serial nice19, timeout20s per
compile/run; fresh host load5.04, CPU PSIavg10=7.99/avg300=11.95. These are
synthetic source checks; original execution parity remains unmeasured.

Pending gates: exact original bounded full-state comparisons and same-input native whole-function/
return/alias proof after PM lifts the host hold. A naturally reached game
frame and loader identity remain separate. All original media and derived
instructions/captures stay ignored under resources/.

target_function: OVL_03C6_0000. source_candidates: raw C/header/test above.
signals: hash-pinned104B/41starts, two conditional sites/four edges,
six near/two far aligned callers, ordered raw aliases,169 synthetic snapshots.
score: source candidate and synthetic checks only. verdict: unclear for
original/native parity. review_notes: no callback, silent policy default,
undefined final-flag normalization or strict-count promotion.
