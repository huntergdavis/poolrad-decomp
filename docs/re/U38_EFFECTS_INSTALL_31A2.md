# Unit 38 +31A2 effect-table installer

## Original extent and ownership evidence

The exact body is unit 38:+31A2..38A8 inclusive, GAME.OVR file
[73213,75012): 1799 bytes, 556 contiguous instructions, SHA-256
4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519.
It is PUSH BP; MOV BP,SP; 138 four-MOV pointer groups; POP BP; RETF.
There are no branches, calls, interrupts or flag-writing instructions.
Public entry 1 resolves to original stub 0041:0025.

Owner-supplied START.EXE is 65231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR is 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The source audit reconstructs descriptors/fixups from these pinned inputs.

The table's game role is independently documented by
OVERLAY_UNIT38_EFFECT_ENTRIES.md, FUNCTION_LEDGER.md and U9E_0160.md:
the effect dispatcher calls its entries through DS:6828. This initializer
installs pointers for those recovered game effects. At the time of this PR its canonical classifier
label was game_or_other_library_unresolved, and this PR supplied game-role
and parity evidence without changing any identity or strict-credit output.
**Update 2026-10-20:** the row is now identity- and strict-credited since PR #467 (reviewed commit `aca640cf`, accounted in `21b65e9a` on 2026-10-14);
the unresolved label above is historical.
Selection began at main dfb6df6d3c5622a4803dde786cb3df93a28d267e. Source,
header, test and strict-manifest searches found no existing implementation or
credit; pF's separate U38 +0918 source/test lane stays untouched.

## Ordered stores and the skipped slot

There are 138 far-pointer installs (276 word stores), targeting DS:682C
through 6A57. Every install writes offset first, segment second. Groups
0..86 use 682C+4*i; groups 87..137 use 682C+4*i+4. The four-byte slot at
DS:6988..698B (table item 88 relative to base 6828) has no table store.
It is not zero-filled. Ordinary stack aliases can still change those bytes.

The missing slot matters: the immediately preceding U86 +3413 initializer
sets DS:6988 to its entry-63 pointer. The game's allocator calls U86, then
U38, so U38 retains that pointer when the frame is disjoint from the table.

Original segment operands are 0041 in 136 groups and 0068 in groups 72/73
(CS:3551/355E). All 138 immediate segment words are loader fixups. The 138
installs resolve to 117 distinct original overlay entry pointers; repeated
slots are retained and do not become new function identities. The installer
does not execute the installed callbacks.

## Raw C contract

pr_u38_effects_install_31a2 accepts 1 MiB RAM and the 14-word CPU layout
AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS after a far CALL. It has no
arguments and returns through plain RETF. Host null/short inputs return
ARGUMENT; otherwise it restores actual BP and CS:IP from RAM and returns OK.
Entry SP advances by four overall. All FLAGS bits, including AF, are preserved
exactly because none of the original instructions writes FLAGS.

The C compacts the repeated MOV groups with a host loop. It reads each loaded
segment word at CS:(31A9+13*i), i=0..137, before the corresponding two stores.
Every other operation/offset is translated in C. Public synthetic tests supply
just those relocation words; no original instruction stream is required by
the C model. Uniform loader bases and independently varied operand words are
both checked; the latter are stress inputs rather than real-loader claims.

Offsets and SP wrap at 16 bits. Multi-byte operands and RETF read contiguous
physical bytes with A20 disabled (1 MiB wrap). Code/relocation words must remain
immutable during execution. Saved-BP and return-frame aliases with table stores
are preserved, rather than restored from host snapshots. The source has no
MOV SP,BP epilogue; POP uses the current SP after the sole PUSH.

## Caller reachability and native composition

The caller is game-verified U36 +002D, the window/script-buffer allocator,
public raw alias +082D normalized to +002D. Its aligned, fixup-confirmed
suffix is +0607 -> U86 entry 10 (0096:0052), followed by +060C -> U38 entry 1
(0041:0025). The audit finds a 507-instruction structural CFG path from the
normalized caller entry through both calls. This assumes other calls return;
it does not prove allocation success, branch feasibility or natural gameplay.

The native fixture uses the exact pinned original five-byte +060C call site
for six direct leaf cases, and the exact ten-byte +0607/+060C suffix for a
seventh composition case. Only caller segment fixups are relocated. Transparent
synthetic far-JMP stubs redirect the original stub offsets to both pinned
original bodies in separate code segments, preserving the real far-CALL frame.
The fixture does not implement INT 3F or run the earlier allocator instructions.
Thus it proves the complete leaf and synthetic caller suffix, not a reached
native game route.

The composition compares the existing reviewed U86 raw model, a real second
far-CALL frame, and this new U38 model against native. DS:6988 retains U86's
015B pointer and relocated segment. All other table words and both initializers'
affected data bytes match. This directly tests why the U38 hole must remain.

## Reproduction and measured gates

With owner media and Capstone 5.0.7/Unicorn 2.1.4:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u38_effects_install_31a2_audit.py --output resources/u38-31a2-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u38_effects_install_31a2_native.py

747 original-byte/C cases pass, comparing the entire 1 MiB RAM and all 14
CPU words with 556/556 instruction sites covered and no FLAGS masking.
Cases include six loader bases, every byte-aligned frame across the table and
its skipped slot, stack/A20 boundaries, and 128 seeded full-RAM backgrounds
with independently varied relocation operands and initial flags.

Seven fresh DOSBox-X normal-core 386 cases pass: six direct leaves and the
original two-call suffix. Each compares all 14 CPU words, 48 stack bytes and
1536 bytes at DS:6800..6DFF, covering all data writes from both initializers.
All flags, including AF, are checked in the six U38 leaf cases. Only AF is
masked in the composed case because U86's prior XOR leaves it undefined;
U38 itself preserves it. IRQs are disabled during setup/execution/capture,
then enabled for DOS I/O. Inputs include carry/overflow/AF and DF variations.
Result SHA-256:
4a85cefdb7675db571af7b73f96959b447482b9c62644e04acb20c4608d88bd8.
DOS allocation segments can change this hash in another environment.
All copied bodies, caller bytes, assembly, COM files, disassembly, captures and
logs stay under ignored resources/.

Focused strict C tests verify input guards, first/last and hole-adjacent slots,
per-site relocated segments, retained slot-88 state, exact FLAGS/registers,
untouched neighbors and table writes replacing saved BP and the far return.

## Review, reuse and follow-ons

Target_function: OVL_0038_31A2. Source_candidates: the new
pr_u38_effects_install_31a2 implementation. Signals: complete pinned extent;
game-effect table role and normalized allocator caller; 138 fixups and the
skipped slot verified; 747 full-state comparisons with complete coverage;
seven native leaf/suffix comparisons. Score: 5/5 listed behavioral checks pass,
a checklist rather than a probability or ownership score. Verdict: match under
the loaded-word, immutable-code, A20-off, entry-frame contract. Review_notes:
earlier allocator calls and natural game reachability remain unmeasured; PM's
exact-head review decides identity and strict credit. No count is promoted here.

A bounded 15-second deja lookup yielded no completed prior-session finding.
Reused findings are the cited effect/dispatcher notes, the 898-start ledger,
the U86 +3413 raw model and native oracle method, and the previous IRQ-isolation
finding. Counts/positions were remeasured from pinned bytes rather than copied
from historical prose.

Two disjoint queued follow-ons were sent to PM before this implementation:
OVL_006A_0159 (+0159..019B, 67 bytes/24 instructions), a record +17..2B high-bit
clearer with final +2C clear, called by U6A +0214/+090A; and OVL_002F_122B
(+122B..1279, 79 bytes/23 instructions), a record +10F flag and +108 link cleanup
based on byte +10E equality, called twice by U2F +031A. No source/header/test/
strict-credit collision was found. Their field meanings and game identity remain
subject to independent review; neither is implemented or credited in this PR.
