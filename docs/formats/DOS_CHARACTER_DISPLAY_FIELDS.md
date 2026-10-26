# DOS character-sheet operands (t174 follow-up)

Static DOS evidence ties character record bytes `0x10..0x15` to the displayed
STR, INT, WIS, DEX, CON, and CHA numbers respectively. This is derived from
the DOS executable's label-selection loop and value-printing helper, not
from the Macintosh format. The valid editing range for each ability remains
UNVERIFIED; identifying the display operand does not establish all of its
combat effects or class/race constraints.

## Static trace

Pinned `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x77` starts at file `0x2103A`, length 11,026, SHA-256
`3011cde3c5b83ac154a7c9f735c4c95d5449154cf250a655992c1fcbb04911e1`.
All cited instruction offsets are relative to that unit. Private extraction,
disassembly, and label checks used audited copies under
`resources/chared-t174/evidence/`.

At `+0x0051..+0x005B`, the sheet routine copies the active character pointer
from `DS:0x5D92/0x5D94` to its display pointer `DS:0x4716/0x4718`.
The loop at `+0x020B..+0x02DA` iterates indices 0..5. Its six branches select
four-byte Pascal label strings at offsets `0x000B`, `0x0010`, `0x0015`,
`0x001A`, `0x001F`, and `0x0024`. The six labels were checked against the
copied original unit. `+0x02B2..+0x02C6` prints the selected label at row
`7 + index`. `+0x02CB..+0x02D3` pushes the same index and mode zero and
calls `+0x0901`.

The value helper's `+0x0962..+0x0976` reloads that index, reloads the active
character pointer, reads the byte at `record + 0x10 + index`, passes it to
the numeric-format helper `0xBA:0x1037`, and prints it via `0x709:0x0626`
on the same indexed row. `+0x0938..+0x0949` separately compares that byte
with ten to adjust the column for a one-digit number.

| Record offset | Width | Display role | Evidence status | Game-valid editing range |
| --- | ---: | --- | --- | --- |
| `0x10` | 1 | STR | VERIFIED static operand | UNVERIFIED |
| `0x11` | 1 | INT | VERIFIED static operand | UNVERIFIED |
| `0x12` | 1 | WIS | VERIFIED static operand | UNVERIFIED |
| `0x13` | 1 | DEX | VERIFIED static operand | UNVERIFIED |
| `0x14` | 1 | CON | VERIFIED static operand | UNVERIFIED |
| `0x15` | 1 | CHA | VERIFIED static operand | UNVERIFIED |
| `0x16` | 1 | Additional STR display suffix input | VERIFIED static role | UNVERIFIED |
| `0x96..0x9D` | 8 | Ordered level-display slots | VERIFIED static reads | UNVERIFIED; class-to-slot mapping not established here |

The additional STR formatting at `+0x097B..+0x0A41` is reached only for
index zero, STR byte equal to 18, and nonzero byte `0x16`. It separately
formats `0x16`, with special handling for values below ten and equal to
100. This establishes the suffix input and its guards, not a universally
valid 0..100 editing range.

The Level label is selected at `+0x02F2`. The loop `+0x0314..+0x03A2`
visits the eight bytes at `record + 0x96 + index`; a signed `JLE` excludes
values at or below zero. Remaining values are formatted in index order
and separated by a slash before the output at `+0x03A5..+0x03B4`.
A signed-positive display condition is not a legal-class-level validator.

## Control and edited-copy game witnesses

The first character of the held slot-A set was loaded through the original
game's own menu, using `Return`, `l`, `a`, `v`. No emulator savestate was
restored. The private screenshot
`resources/oracle-runs/chared-t174-control/control-character-sheet.png`
shows STR **17**, the other five ability values **12, 12, 17, 16, 15**, and
Level **1**. Its SHA-256 is
`4b049bba7df679ea3702d7b4a15906be4024b1349cc9a5e0d04d8133cfad70b8`.
The character file remained unchanged with SHA-256
`d7ba1321e1fca8464d1220e77c26ce92173d60cb67b0f155c71868926cfa6c8e`.

The run used the owner's DOSBox-X 2026.10.01 binary, SHA-256
`a11d55f0cb9bf10e36e69d8df27c3bd6bc303f5aacde32f3e7cdc96c3ebfe967`,
selected from the desk service's `VP_DOSBOX_X` setting. The actual process
executable and arguments were checked and recorded privately. It ran in an
independent Xvfb display and tmux socket with the copied run as the sandbox's
only writable game tree. It did not use the owner's Play pane or live run.

VP approved a separate copy with only record byte `0x10` changed from STR
17 to 16 (inbox i26502). The successful edited run is
`chared-t174-edit-auto`; its private character-sheet screenshot is
`resources/oracle-runs/chared-t174-edit-auto/edited-character-sheet.png`,
SHA-256 `c673e4526283a31f983cc0136e8febc2b2511164cad5c6707c0a3353a066c0e2`.
Visual inspection shows **STR 16**, with the other five abilities and Level
unchanged. The displayed THAC0 changed from **19 to 20**. This is an observed
sheet value, not a combat-execution or general derived-stat correctness test.

The edited character SHA-256 is
`e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a`.
An independent post-run comparison of all **153** installation files in the
control and edited copies found only `CHRDATA1.SAV` different, at exactly
one offset, `0x10`. All **153 original source-file hashes** still match the
pre-experiment copy receipts. All sidecars, world files and game executables
in the two run copies compare byte-identically. The unchanged slot-A world
file selects this character as the first member.

Both runs used the same original game menu sequence: `Return`, `l`, `a`,
`v`. In the successful edited run the detached driver captured the codeword
prompt, menu, slot chooser, gameplay, and character sheet separately. It
recorded the actual process executable/hash and command line, including the
unique edited run mounted as C, before sending input. The game loaded slot A
from that mount; no emulator savestate was restored. The same character's
sheet changed from 17 to 16 after the sole disk-byte edit. The input JSONL,
process identity, intermediate frames, final frames and copy manifests remain
private under the named runs. `resources/chared-t174/strength-game-proof.json`
combines the independent comparisons and observation hashes, and the edited
run's `chared-edit-receipt.json` now records the build that loaded the copy.

The control was completed before the edited run was submitted. Its emulator
needed termination after capture; the process identity was checked before
closing only the owned process. An earlier edited run (`chared-t174-edit`)
exited normally without inputs or captures and **provides no edited-load
proof**. The successful repeat placed the proven control input sequence and
captures inside the detached job, so a delayed semaphore grant could not
leave the test waiting for an inactive assistant turn. All game runs used
the heavy semaphore, distinct tmux sockets and private Xvfb displays. The
owner's desk, Play pane and live game were not controlled or modified.

This establishes one specific edit/load/display case: **this character's
STR 17 to 16** in the pinned original DOS game and owner emulator build. It
does not establish a general legal range, all derived effects, level edits,
other ability edits, or party assembly.

## Prior evidence and limits

[DOS_CHARACTER](DOS_CHARACTER.md) and [DOS_SAVGAM](DOS_SAVGAM.md) establish
the disk framing; [SLOT_A_PARTY_INPUT](../re/SLOT_A_PARTY_INPUT.md)
establishes the saved party ordering. [DOS_SAVE_ROUNDTRIP](DOS_SAVE_ROUNDTRIP.md)
records the already-reviewed 36-file no-op gate. Initial Dunking Sheep recall
located prior save sessions; the later gameplay recall returned session
`01a12855-addb-78d1-ad73-89ae2590dc75`, including the observed Return-to-menu
and load-slot-A route. The existing oracle preparation/launch scripts and
VP gameplay procedure were reused for isolation and capture.

No broad ability/level legality, derived combat behavior, arbitrary spell
editing, newly assembled party, or web UI is established here. Original
media and private screenshots remain ignored. No function-accounting or
strict-C credit change is requested.


## Validation and review boundary

`python3 tools/character/audit_layout.py` passes with **39 ranges covering
285 bytes**, no gaps or overlaps, **21 VERIFIED structural/display roles and
18 UNVERIFIED ranges**. These labels scope the described role, not a blanket
gameplay-edit guarantee. The original step-1 ledger counts are historical.
Six original label literals and their length prefixes were checked, and the
label/value instruction paths above were read from the copied executable.
No executable implementation or function-accounting files changed.

`ran run_media_checks.sh: not run`; this documentation/ledger slice uses the
layout-coverage audit, original-byte static inspection, independent full-file
comparison and the two original-game screenshots. No broader native/C parity
or gameplay coverage is claimed. Reviewer: **pi-review-2**, assigned by VP
inbox i26684. Private frames and receipts are supplied for independent review;
they are not committed to the public repository.
