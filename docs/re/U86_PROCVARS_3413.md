# Unit 86 +3413 procedure-variable initializer

## Selection and provenance

This is the first newly untranslated, unclaimed complete game-evidenced leaf
remaining after the current ownership/model checks at main
21dd2b868c2883025a0ab6a749942e16ff332b48. The address queue was rebuilt privately
from the 898-start ledger, current classifier/strict/identity manifests and
the pinned inventory/near-call/resident inputs. No canonical accounting file
was edited. The historical seven-leaf list is stale: U6A +025C/+14A9 already
have strict credit, U6A +0824/+1601 are assigned, and U77/U33 are under review.
The two shorter remaining queue leads U7F +05E0 and U7D +02F2 already have
merged models; the former has a historical unknown claim, and the latter is
the timer carry normalizer. U7F +05E0 was proposed to PM as a reuse/parity
follow-on subject to clearing ownership, not as a new duplicate translation.

Reused findings: FUNCTION_ALIAS_AUDIT.md supplies the leaf lead and span;
function_candidate_queue.py supplies the measured ranking policy;
OVERLAY_FUNCTION_LEDGER.md documents the loader/stub coordinate system and
procedure-variable evidence. A bounded 15-second deja query yielded no
completed prior-session finding. The native harness reuses the U25/U77 COM
method and its interrupt-isolation correction.

Owner-supplied START.EXE is 65231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
GAME.OVR is 206105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The exact body is file [173099,174024), unit 86:+3413..+37AF inclusive:
925 bytes, 286 contiguous instructions, SHA-256
66ffc8c06e80199ee68e064db44ccb47bcb2e51d661786e0bcdc4f466080e5d5.
There are no branches, calls, interrupts or external-effect hooks. It ends
with POP BP; RETF and takes no arguments.

## Game caller and effects

This is public U86 entry 10, original stub 0096:0052. Its aligned,
fixup-confirmed far caller is unit 36:+0607 inside the game-verified
window/script-buffer allocator +002D (raw public alias +082D). The source
audit decodes from that containing function start and verifies both the
LCALL target and its relocation word. This connects the initializer to an
existing game-state allocation path; it is not merely a prologue match.

The function installs 69 far procedure pointers: one at DS:6B84, 67 ordered
slots at DS:6B88..6C93, and one at DS:6988. Their offsets resolve to 55
distinct U86 public entry slots. It also clears bytes DS:681F and 6DBF,
clears words DS:6DB5 and 6DB7, and sets byte DS:6821 to 1. There are 143
ordered DS writes in total. It does not invoke the installed procedures.
Their later behaviors are outside this initializer, not hidden call hooks.

The C preserves the source's offset-then-segment write order and the first
pointer installation before the state clears. Its host loop compacts the
67 repeated MOV groups without changing the emulated CPU flags. The source's
only flag-writing instruction is XOR AX,AX at +342D; all subsequent body
instructions are MOV. Returned AX is 015B, DX is the last loaded pointer
segment. Other general registers remain unchanged, except the live restored
BP and actual CS:IP/SP return state. Frame aliases through the globals remain
observable, including a pointer write replacing saved BP and both return words.

## Loaded relocation words and raw ABI

pr_u86_procvars_3413 accepts 1 MiB RAM and the 14-word CPU layout
AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS, after a far CALL.
It returns OK or ARGUMENT for null/short inputs. Offsets and SP wrap at
16 bits; multi-byte memory/RETF reads are physically contiguous, wrapping at
1 MiB with A20 disabled. Original code and loaded relocation words must
remain immutable during execution. XOR AF is undefined and normalized to
zero; other defined flags are preserved or set as in the source.

All 69 original MOV DX,0096 immediate words are present in the overlay's
fixup table. DOS load relocation therefore changes the installed segments;
hardcoding 0096 would be incorrect. The C reads each loaded word at CS:341A,
CS:(343E+13*i), i=0..66, and CS:37A5 at its corresponding source operation.
Every non-relocated operation and offset constant is translated in C. It
needs those 69 caller-supplied words, not the original instruction stream or
an interpreter. Public C tests seed just these words synthetically and run
without original code bytes. Uniform valid loader bases and independently
varied operand words are both checked; the latter are stress inputs rather
than a claim about DOS loader behavior.

## Verification and limits

With owner-supplied media and Capstone 5.0.7/Unicorn 2.1.4:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u86_procvars_3413_audit.py --output resources/u86-3413-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u86_procvars_3413_native.py

The original-byte audit passes 606 cases with full 1 MiB RAM and all 14 CPU
words equal, visiting 286/286 source sites. It verifies the source hash,
all relocation positions, entry mapping and aligned game caller. Cases cover
six loader bases, every byte-aligned frame across the pointer-table region,
other global/frame aliases, stack/A20 boundaries, and 256 seeded full-RAM
backgrounds with independent relocation operands and initial flags.

Six fresh DOSBox-X normal-core 386 cases execute actual far CALL frames,
including original, loaded, wrapped, zero, high and independently varied
pointer segments. All 14 CPU words, 48 frame bytes and the entire initialized
1536-byte DS:6800..6DFF region match C. IRQs are disabled during setup,
execution and capture (caller FLAGS=0002), then enabled for DOS I/O. Only
undefined XOR AF is masked. Result SHA-256:
52ab3d109c8abff1ec332b5d8f555e23f66ff2339156485e624071d667d98ed6.
DOS allocation segments can make this output hash environment-dependent.
Generated bodies, assembly, COM images, disassembly, captures and logs stay
under ignored resources/. These frames are synthetic, not a reached native
game caller or a natural saved-game route; no such route is claimed.

Focused strict C tests check guards, representative first/last pointers,
separate clear/set fields and untouched neighbors, relocated operands,
returned flags/registers, and a global write altering saved BP and return.

## Parity review

Target_function: OVL_0086_3413. Source_candidates: the new
pr_u86_procvars_3413 implementation. Signals: exact pinned call-free extent;
game-verified allocator caller and valid entry-slot targets; all 69 loader
fixups verified; 606 full-RAM/CPU comparisons with complete coverage; six
isolated native far-call comparisons. Score: 5/5 listed behavioral checks
pass; this is a checklist, not a probability or ownership score. Verdict:
match under the documented loaded-word, immutable-code, A20-off entry contract.
Review_notes: installed callbacks are not executed; real reached game state
is unmeasured; game identity and strict credit require PM's exact-head review.
No identity or count promotion is included in this change.
