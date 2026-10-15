# Saved-A selector0334 post-IOResult caller gate

This standalone adapter starts a fresh C execution at the actual later
selector1026:0334 post-IOResult input. It stores the full AX word at
DS:49CE, executes the word/error and local-name decisions, saves/reloads
the local AL result, restores SP/BP, and executes real RETF4 to the first
unsupported returned loader2A8E:CAF boundary. It stops before that loader
instruction/branch. No read-to-selector or DOS join is claimed.

PM i4074 authorized this distinct scope after publication of
[C49 prefixa13359e](https://github.com/huntergdavis/poolrad-decomp/blob/a13359ebc3a36764591078f920fa736cb86217b4/docs/re/SLOT_A_C49_CLOSE_PREFIX.md).
The [approved read-return/IOResult evidence](https://github.com/huntergdavis/poolrad-decomp/blob/e24fadbe5138fdd1bf020907ad1ed6a8b7668cb0/docs/re/SYSTEM_READ_152B_RETURN.md)
supplies the separately reached later0334 context lead. Existing merged
[probe CPU semantics](../../src/replay/slot_a_file_probe_02eb_abi.c) and
[selector evidence](RESIDENT_DAX_SELECTOR_02EB.md) are reused as leads;
all instructions/frame/flags/writes are rechecked against original bytes
and this new native capture. The new C compiles alone, with no unmerged
C adapter dependency. Original-byte readers/comparers/emulator stepping
reuse current main's audit infrastructure. A bounded prior-session0334/
49CE/caller-gate query expired after35s; no recalled result was reused.

Base is fresh main217e809448be096fcc48b2413714856f5b165284. Six new uniquely
named files only. Shared integration/scoreboard, PR410 maps,0425 guards
and other lanes' printer/key files are unchanged. Sidecar/allocator/printer
bodies and other follow-ons remain separate. Existing merged probe-tail
instructions are not presented as a new game-function identity.

## Native-first same-run input

A fresh original DOSBox-X heavy-debugger saved-A run first stopped at
loaderC26, then traversed original native code to the later CAA selector
call. It captured actual IOResult04A2 entry, actual returned0334, and every
caller instruction through real RETF4 toCAF. All14 CPU/full655360-byte RAM
states were frozen before writing C, without guest CPU/RAM edits; the
owned emulator then stopped. Original media, derived content, screenshots,
logs, dumps and binaries remain under ignored resources/.

IOResult's actual frame at SS3334 is[0334,1026], with BP33E2. One separately
counted original-only execution of its three instructions (XOR/XCHG/RETF)
matches the native0334 endpoint in full CPU/RAM. That provenance comparison
is excluded from C counts. No IOResult output is injected by this adapter.
Earlier original DOS, runtime, read, Close and sidecar traversal establish
the input only and receive no C credit here.

| Pin | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original resident0334..0355,START838B..83AC | 34 | 86e4cb84d29f3d8d6c422459c06563bb6218c34e1897c7d05628954c021c5014 |
| Original IOResult04A2..04A8 | 7 | d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b |
| Raw native transcript | private | 0be309e7e33f6d686825a1bba0ed77fdb43d33fbf0cf8388dfa60c267a717233 |
| Actual post-IOResult0334 full RAM | 655360 | c94efe5f61de97c002a13d783f448bcb41f2fe73e5e4546d8434bd279e791df5 |
| Returned loaderCAF full RAM | 655360 | 491a882722a183d2cdfba9d08b16da536fd9a4626ba3d5c8de4d0e831a0d3991 |

The audit pins full media/every RAM image, compares every CPU JSON with
raw EV rows, and checks original/loaded selector and IOResult spans in
all14 states. The original/loaded CAA far call confirms802->1026 relocation;
SYSTEM AF8->131C is the observed +0824 load relocation. IOResult code
fingerprinting/provenance does not grant it C coverage in this slice.

## Measured caller ABI and first unsupported boundary

Actual0334 input: CS1026 DS14A0 ES=SS1D16 SP3338 BP33E2 AX0000 BX0006
CX0000 DX3368 SI33D0 DI3338 FLAGS7246. The preserved outer selector frame
at SS33E2 contains five words:

~~~
37A6 CAF 2A8E 34ED 1D16
~~~

The first word is saved BP, followed by the real far-return IP/CS and
four-byte argument. The local name's measured length byte at SS:[BP-2A]
(33B8) is23. IOResult's AX0000 is stored as a full word at DS49CE; native
zero remains zero. CMP word0 selects the local-name test; CMP byte23
selects AL1. Only AL is changed, stored at SS:[BP-1]=33E1 and reloaded.

MOV SP,BP/POP BP restore BP37A6 and position SP33E4 at the real far frame.
RETF4 consumes eight stack bytes and reaches2A8E:CAF, SP33EC, AX0001,
FLAGS7206, retaining DS14A0 ES=SS1D16 BX6 CX0 DX3368 SI33D0 DI3338.
The only net native RAM change in this C fragment is the one-byte local
result at physical20541. The typed CAF boundary preserves the complete
CPU/RAM and stops before OR AL,AL or any allocator/sidecar continuation.

The word test uses all AX bits, not only AL. A controlled IOResult0100
stores0100 at DS49CE, takes the error branch and returns AX0100/AL0 with
AH retained. A zero error with empty local name returns AL0; any nonzero
length byte, including80/FF, selects AL1. Byte comparisons use the8-bit
sign flag; whole-word comparisons use the16-bit sign flag. No host boolean
normalization erases AH or substitutes a DOS/runtime result.

## Verification and adversarial review

Focused strict C11 compilation uses Wall/Wextra/Wpedantic/Wconversion/Werror.
Focused tests and the complete differential audit also pass ASan/UBSan:

- 838 independent original-byte Unicorn/C full-state comparisons.
- All11 post-input native full CPU/RAM checkpoints match, from14 same-run states.
- All16 CPU fields, every FLAGS bit and all655360 RAM bytes are compared.
- One original-only IOResult endpoint comparison over three original
  instructions is excluded from C counts. No masks or runtime/DOS credit.

Controlled fresh inputs cover full error words0/1/0100/8000/FFFF, local
lengths0/1/23/80/FF and flag/DF variants; global DS49CE aliasing the local
name, actual return IP, or local result/saved BP; complete caller stack
wrap; unknown real return; and an independent local-byte reload preserving
AH. Store/read ordering is checked against unchanged original bytes.
These controlled errors/empty names are not additional observed native
I/O outcomes. Focused tests also check full-word/high-byte decisions,
8-bit flags, real return consumption, NULL arguments and atomic bounds.
Cases are bounded at100 instructions/10s; process120s plus5s kill allowance.

target_function: reached originalresident802:0334 post-IOResult caller
tail, loaded1026, through actual selector return to unit6F:CAF.
source_candidates: src/replay/slot_a_selector_0334_gate.c only.
signals: same-run actual IOResult return/frame; pinned original/loaded
caller bytes; whole-word store/word test and byte-name flags; local AL
save/reload/AH retention; real RETF4;11 unmasked native full-state
checkpoints; aliases, high-byte failures and stack-wrap controls.
score: 4/5 ordinal bounded evidence strength, not a probability.
verdict: match within the stated post-IOResult-to-CAF scope.
review_notes: native zero-error/nonempty-name route only; controlled error
and empty-name branches have no additional native captures. IOResult,
earlier DOS/runtime, loaderCAF branch, allocator, sidecar and printer
semantics remain outside this C execution. No live snapshot replacement,
continuous read-to-selector path, full-C identity/count or playable credit.

~~~
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
bash tests/replay/check_slot_a_selector_0334_gate.sh \
resources/oracle-runs/slot-a-selector-0334-gate-20261015
~~~
