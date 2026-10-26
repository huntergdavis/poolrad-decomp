# DOS party assembly from existing records (t174, step 4)

`tools/character/assemble_party.py` copies selected existing character records
and their present sidecars into a new save-slot set. Character and sidecar
bytes are preserved exactly. The only world-frame edits are the character
count and the occupied basename length/text fields. It does not generate
characters or edit stats, levels, spells, position, or unknown world state.

```sh
python3 tools/character/assemble_party.py "$private_source" "$new_copy" \
  --world SAVGAMA.DAT --slot B --members CHRDATA1.SAV CHRDATA2.SAV
```

The output is a new directory containing `SAVGAMB.DAT`, `CHRDATB1.SAV`,
`CHRDATB2.SAV`, their present `.ITM` / `.SPC` sidecars, and a receipt. The
first requested member becomes basename B1, the second B2, and so on.
Selection order is preserved. The original files and existing output
paths are never overwritten. Missing optional sidecars remain absent.

## Byte contract

The saved-world layout is inherited from [DOS_SAVGAM](DOS_SAVGAM.md) and
[SLOT_A_PARTY_INPUT](../re/SLOT_A_PARTY_INPUT.md): byte `0x3208` is the
character count; eight 41-byte Pascal basename slots start at `0x3209`.
The original loader traverses occupied slots in order and rebuilds the
runtime pointers and party slots. Those runtime changes are not applied to
the disk records by this assembler.

For each selected member, the assembler changes only the slot's length
prefix and occupied name bytes. Trailing bytes and unused slots are kept
exactly as they were; the new count excludes unselected slots. New stems
have eight ASCII bytes, `CHRDAT` plus a slot letter and a member index.
The slot's disk filename changes through copy naming, not by rewriting the
character's internal name.

Tool bounds are one to eight distinct source filenames and a destination
letter A through J. The eight-member bound follows the storage table. The
letter restriction is a conservative tool policy. These checks are not a
claim that every class/race/NPC combination, party size or target slot has
been accepted by the game. Duplicate selections, missing members, invalid
base worlds and unsafe output names fail before any output directory is
created. Reusing a source character through another filename is not a
verified gameplay capability.

The shared exclusive copy writer from [DOS_SAVE_ROUNDTRIP](DOS_SAVE_ROUNDTRIP.md)
now accepts the assembler's planned fixed-length changes. It checks output
names, requires a reason for every changed byte, rereads written bytes,
checks source snapshots and emits before/after hashes and bytes. Existing
output and source-descendant refusals and failed-write rollback are shared
with the no-op exporter. Receipts say game loading is UNVERIFIED until a
specific assembled copy has been tested. This does not imply transactional
protection against host crashes or concurrent filesystem attacks.

## Original corpus: unedited assembly control

The first control selects the two unedited original slot-A members and
creates a fresh slot B. Seven files totaling **14,004 bytes** were written;
the six character/sidecar files remain byte-identical to their sources.
The world file differs from the original slot-A world at exactly:

| Offset | Change | Reason |
| --- | --- | --- |
| `0x3208` | count 6 to 2 | Select two party members |
| `0x3210` | basename slot 0: A to B | Point at new member file B1 |
| `0x3239` | basename slot 1: A to B | Point at new member file B2 |

The seven files were installed exclusively as new files in the separately
prepared `chared-t174-party-control` run. None of that run's original 153
files was overwritten. Copy and assembly receipts are retained privately.
The original game loaded slot B through `Return,l,b,v`. Visual inspection
of the private control roster shows exactly **two members**, and the first
member's character sheet shows **STR 17**. The control roster screenshot
SHA-256 is `a5c999bd63f0a68b928d2a3cec58068127b272eb670f5f2a54d8a672b49d10b2`;
the sheet SHA-256 is
`4b049bba7df679ea3702d7b4a15906be4024b1349cc9a5e0d04d8133cfad70b8`.
The control completed normally before the edited-party run was submitted.

Only after observing this accepted control was the same two-member party
assembled with the previously proven STR16 character. Its seven output
files differ from the unedited assembly at exactly one byte:
`CHRDATB1.SAV +0x10`. Their world files and all other bytes are identical.
The original game accepted this edited slot B too: the gameplay frame shows
**two members**, and the first character's sheet shows **STR 16**. The edited
roster screenshot has the same SHA-256 as the control roster,
`a5c999bd63f0a68b928d2a3cec58068127b272eb670f5f2a54d8a672b49d10b2`.
The edited sheet SHA-256 is
`c673e4526283a31f983cc0136e8febc2b2511164cad5c6707c0a3353a066c0e2`.
These private images are under `resources/oracle-runs/chared-t174-party-edit/`.

Each run contains 160 game files: the unchanged 153-file installation plus
seven newly installed slot-B files. An independent comparison across both
complete run copies found exactly one differing file and byte:
`CHRDATB1.SAV +0x10`. All 153 original source-file hashes still match the
pre-experiment manifest. Comparing the edited assembly outputs with their
original slot-A source roles gives exactly four changed bytes: the three
world-file bytes listed above, plus character `0x10` for the prior approved
STR17-to16 edit. No other byte changed. The assembler itself preserves its
selected character input; the stat edit comes from the already-proven input.

Both runs used `Return,l,b,v`, recorded in input JSONL. Each actual emulator
process was checked against the owner's DOSBox-X2026.10.01 binary hash
`a11d55f0cb9bf10e36e69d8df27c3bd6bc303f5aacde32f3e7cdc96c3ebfe967`;
its arguments mount the corresponding distinct run as drive C. Loading B
therefore selects the new slot-B files. No emulator savestate was restored.
DOS executable hashes remain the pinned v1.3 hashes in DOS_SAVGAM.md.
The control ran and completed before the edited run was submitted; each used
its own Xvfb and tmux socket through the heavy semaphore. No owner Play pane,
desk service, or live-run files were controlled or modified.

`resources/chared-t174/party-game-proof.json` retains every original/output
hash, all four changed-byte values and reasons, screenshot hashes, and
observations. Each run's `chared-assembly-copy-receipts.json` identifies the
actual installed files and the game build that loaded them. Raw screenshots,
original bytes and machine-local paths stay out of the public repository.
This proves the tested two-member initial-load composition only.

## Validation and provenance

Fourteen synthetic tests pass, covering the existing nine no-op safety
cases plus reordered subsets, untouched unknown world bytes, same-slot
no-op assembly, exact changed-byte receipts, optional sidecars and invalid
selections. After the shared-writer refactor, an independent comparison
again found **36 original save files / 31,656 bytes / zero differences** on
no-op export; all **153 original installation hashes** remained unchanged.

`ran run_media_checks.sh: not run`; targeted synthetic tests, the original
no-op gate and the paired isolated original-game controls are the relevant
checks. No function-accounting or strict-C credit change is requested.

Before implementation, Dunking Sheep recall for the party count/basename
layout found prior DOS-save sessions, including
`3e3ff494-f865-446a-8777-6a4baca8de94`. Reused the linked DOS world-frame and
party-input research. The earlier accepted single-field STR edit is PR
#1592. VP assigned **pi-review-1** to this separate assembly review (i26708),
requiring an unedited two-member control before the edited-character test.

NOT VERIFIED: general legal stat/level/spell ranges, all party compositions,
combat behavior, unrelated field edits or the web UI. The successful initial
loads do not establish those behaviors.
