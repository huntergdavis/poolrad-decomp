# Unit 33 +08A4 byte flags

## Provenance and interface

Owner-supplied resources/DOS/POOLRAD/GAME.OVR is 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The exact source is file [52853,53608), unit 33:+08A4..+0B96 inclusive:
755 bytes, 338 contiguous instructions, SHA-256
c721e542769e5b590a6ef00ee92ffa9b4a9b7a5a76a9c4563eda25b3a4b703bc.
Every branch stays within this span; there are no calls or external-effect
callbacks. The final instruction is RETF without argument cleanup.

pr_u33_byte_flags_08a4 accepts the same 14-word CPU layout used by the other
raw RAM translations: AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS.
The state is after the far CALL, with return IP/CS at SS:SP. It returns
OK or ARGUMENT for a null/short input. Offsets and SP wrap at 16 bits;
word/LES/RETF operands read contiguous physical bytes with A20 disabled.
The code must remain immutable. Aliased frame writes affect actual returned
BP and CS:IP rather than a host-saved return. AF is undefined after logic
instructions and defined after CMP; the C normalizes undefined AF to zero.

## Source behavior

The function pushes BP, reserves two local bytes, then reads DS:465A once.
It selects one of the source-ordered byte classifications and writes the
first local at SS:[BP-2]. Earlier overlapping ranges win: input 7C selects
08 rather than the later 09 branch; F5 selects 20 rather than the later
F4..F6 ->80 branch. The C keeps that source order.

If none of the original branches matches, the first local remains the actual
old RAM byte. It is neither zero-filled nor rejected. This input-dependent
stack behavior is part of the recovered function, so a pure byte-to-byte
lookup table with a default value would be incomplete.

At +0B68 the function tests the local's high bit. When set, LES reads the
live window-0 pointer DS:49D2/49D4 into ES:DI and CMP reads the word at
ES:[DI+0366]. If that word is exactly 00FF, the function subtracts 80 from
the live local, loads it into AL, ORs AL with 1, and writes it back. Otherwise
the class byte is retained and the returned flags come from that word CMP.
When the high bit is clear, there is no LES/window read and returned flags
come from OR AL,AL of zero.

Finally the first local is copied to AL, then to SS:[BP-1], then back to AL.
AH is preserved. BX/CX/DX/SI are preserved; ES/DI change only on the high-bit
path. The routine restores its live BP/return frame and advances SP by four.
The implementation preserves reads after writes that can alias DS globals,
the window pointer, saved BP, or the return frame. Selection CMP flags can
be folded because +0B6B AND and +0B6D OR overwrite them before any observation.

## Reproducible verification

With Capstone 5.0.7, Unicorn 2.1.4 and owner-supplied original media:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u33_byte_flags_08a4_audit.py --output resources/u33-08a4-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u33_byte_flags_08a4_native.py

The original-byte oracle passes 2644 cases, each comparing the entire 1 MiB
RAM and all 14 CPU words. All 338/338 source instruction starts are reached.
It tests every input byte with four old-local seeds and both window FE/FF
states, every possible old local for an unmatched input, stack/DS-table
aliases at 16 stack offsets, word-comparison flag boundaries, and A20-off
window reads crossing physical 1 MiB. RETF is single-stepped separately;
AF is masked only after logic, so defined CMP AF is checked.

The fresh DOSBox-X normal-core 386 oracle executes 512 synthetic actual
far-CALL frames: every input byte with window 00FE and 00FF, with unmatched
stack seed A5. It compares all 14 CPU words, 48 stack bytes, the input word,
the window pointer and the window word. All pass. The final result SHA-256 is
93cf8dabd1ace03f2f173e466d07732f06c1b80ab36d1ee8e44c2115dacb0a10.
DOS allocation segments can make this hash environment-dependent. All copied
source bytes, assembly, COM images, results and logs stay in ignored resources/.

The native harness disables interrupts from stack initialization through
capture (caller FLAGS=0002), then enables them for DOS I/O. An earlier IF=1
run agreed on all routine CPU results but showed interrupt-handler stack
writes below the locals; the isolated oracle measures this routine's effects.
These are synthetic native caller frames, not a reached game caller or an
unedited saved-game route. No such route-specific evidence was supplied.

Focused strict C tests cover input validation, live local preservation,
overlapping source ranges, AH/register/frame preservation, defined CMP flags,
and a frame/local write changing both words of the window pointer before LES.

## Review and reuse

Target_function: OVL_0033_08A4. Source_candidates: the new
pr_u33_byte_flags_08a4 implementation. Signals: pinned exact source extent,
all branches internal and zero calls, full source-order local/window effects,
2644 full-RAM/CPU comparisons with complete instruction coverage, and 512
DOSBox-X synthetic native frames. Score: 5/5 listed behavioral checks pass;
this is a checklist, not a probability or identity score. Verdict: match
under the documented entry-frame, immutable-code, A20-off contract.
Review_notes: unmatched input observes old stack RAM; the meanings of DS:465A
and the category bits remain unnamed; no natural caller was measured.
Strict credit and identity require PM's independent review and are not changed
by this PR.

The merged FUNCTION_ALIAS_AUDIT.md and function_leaf_candidates.csv supplied
the candidate and pinned span. Native harness construction reuses the method
in tools/overlay/u25_load_word_0e0c_native.py. Bounded deja searches for these
addresses, the U33 classifier, and native interrupt isolation produced no
completed prior-session finding. Neither U33 nor U77 appeared in the strict
manifest at U33 branch base 289f060aeb13d1621d5a2fff632c824d915533c8.
