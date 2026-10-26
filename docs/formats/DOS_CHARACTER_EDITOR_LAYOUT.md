# Character editor: DOS disk layout evidence (t174, step 1)

The real DOS files are `CHRDAT*.SAV`, paired `.ITM` / `.SPC` sidecars,
and `SAVGAM*.DAT`. The held corpus has `CHRDATA1..6.SAV` and
`CHRDATJ1..6.SAV`, with `SAVGAMA.DAT` and `SAVGAMJ.DAT` respectively.
They are distinct from DOSBox-X emulator savestates. This document is a
layout investigation, not a working character editor or a completed semantic
map. Reviewer assigned by VP: **pi-review-2** (inbox i26449).

The [machine-readable field ledger](DOS_CHARACTER_EDITOR_LAYOUT.csv) covers
every byte of the 285-byte character frame, without gaps or overlaps. Each
row has an offset, width, name, encoding, valid-range qualification, status,
and evidence. Unknown spans are preservation ranges, **not a claim that
those bytes form one logical field**. Further tracing will split them.

`VERIFIED` means the specifically named structural role has evidence. It
never means arbitrary edits to that field have loaded successfully in the
game. Qualifications in encoding, range, and evidence remain binding.
`UNVERIFIED` includes fields whose machine access is known but whose game
meaning or allowed editing values are not. A byte's representable 0..255
range is not evidence of a gameplay-valid range. No editable stat, level,
HP, class, spell ID, or ability range is verified by this investigation.
The Pascal name span is established; the 15-byte cap is a conservative
reader limit supported by the corpus, not a proved universal game limit.

## Provenance and measurements

The source was read only. All subsequent byte inspection used private,
read-only copies under `resources/chared-t174/evidence/DOS/POOLRAD/` in this
worktree. A private `copy-receipts.json` records source and destination paths,
size, both SHA-256 values, empty changed-offset lists, and `game_build_loaded:
null` for every copy. All **38 copies** compared byte-identically with the
source data at copy time. This is a copy-integrity check, **not the step-2
reader/writer round-trip gate**. No original save was modified.

| File class | Files | Sizes in bytes |
| --- | ---: | --- |
| `CHRDAT*.SAV` | 12 | 285 each; 3,420 total |
| `CHRDAT*.ITM` | 12 | 63, 126, or 189; 1,764 total |
| `CHRDAT*.SPC` | 10 | 9, 18, 27, 36, or 45; 198 total |
| `SAVGAM*.DAT` | 2 | 13,137 each |
| `START.EXE` | 1 | 65,231 |
| `GAME.OVR` | 1 | 206,105 |

Pinned build: supplied DOS v1.3, with SHA-256:

- `START.EXE`: `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
- `GAME.OVR`: `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`

These identify the DOS inputs, not the owner's DOSBox-X host executable.
No game load or screenshot was taken for this step.

## Code evidence and reuse

Before investigation, `dunkingsheep memory context "poolrad t174 CHRDATA
character record original game save layout media"` returned prior DOS-save
sessions, including `01a12855-addb-78d1-ad73-89ae2590dc75` (unit `0x6f`
CHRDAT write path) and `01a12b3e-8cea-7de2-bfd1-ba3a8310754f` (six-record
C save loader). Reused evidence is cited below; older documents' Deja
commands are historical and were not run.

- [DOS_CHARACTER](DOS_CHARACTER.md): frame, Pascal name use, pointer slots,
  party slot, sidecar sizes, and the existing read-only C API.
- [DOS_SAVGAM](DOS_SAVGAM.md): the ten world transfers, count at `0x3208`,
  and eight 41-byte basename slots beginning at `0x3209`.
- [SLOT_A_PARTY_INPUT](../re/SLOT_A_PARTY_INPUT.md): party ordering,
  post-load mutations, and PARTYSTRENGTH field access. This prior bounded
  loader is not an export serializer.
- [U9B_RECORD_INCR_01AF](../re/U9B_RECORD_INCR_01AF.md): byte `+0x12`
  controls conditional increments at `+0xB2..+0xB4`.
- [RESIDENT_CHARACTER_FIELD_ZERO](../re/RESIDENT_CHARACTER_FIELD_ZERO.md):
  the zero predicate on byte `+0x10E`, without assigning a game field name.

Fresh static checks used the copied executables and the existing overlay
inventory tool. Unit `0x6f` starts at `GAME.OVR +0x1DFE6`, is 9,189 bytes,
and hashes to `30da2d2a28149942431395bf59511b55a9eab4d8f728bff625c5a22d81638882`.
Disassembly confirms:

| Site within unit | Checked behavior |
| --- | --- |
| `0x6f:+0x0C35..+0x0C44` | Record pointer `[BP+0xA]`, count `0x11D`, call `0xAF8:0x14FB` |
| `0x6f:+0x08B3..+0x08C6` | Same record pointer, count `0x11D`, call `0xAF8:0x1502` |
| `0x6f:+0x0C54..+0x0C8A` | Zero word pairs at record `0x104/0x106`, `0xC8/0xCA`, `0x7F/0x81`, `0x108/0x10A` |
| `0x9b:+0x01AF..+0x0254` | Unsigned comparisons of `+0x12` with 12..16; conditional byte increments of `+0xB2`, `+0xB3`, `+0xB4` |

I/O helper roles and remaining name/slot/party traces are reused from the
linked prior research; the instructions above were rechecked, not a new
full reverse engineering of every caller. Unknown game meanings remain
unknown even where the increment or pointer-reset mechanism is verified.
The Macintosh 302-byte format is a research lead only: its offsets,
endianness, name encoding, and spell rules must not become DOS facts by
analogy.

## Reproduce the read-only audit

Run from the repository root:

```sh
python3 tools/character/audit_layout.py
python3 tools/character/audit_layout.py \
  --media resources/chared-t174/evidence/DOS/POOLRAD
```

The first command checks the ledger's exact 285-byte coverage and explicit
statuses. The second additionally checks the two executable hashes,
character and world sizes, conservative name bounds, sidecar framing and
pairing, and that each occupied world basename resolves to a character
file. It prints aggregate measurements only. Sidecar lengths establish
framing multiples, not item/spell meanings. Basename ASCII and safe spelling
are audit constraints for this corpus, not a universal DOS pathname rule.

The audit does not execute DOS code, write saves, interpret unknown fields,
or validate arbitrary edits. A successful structural audit must not be
presented as game acceptance. Missing media is a failure when `--media` is
specified; omitting it reports coverage only.

Five targeted negative controls were run: a gap, an overlap, an invalid
status, shortened coverage, and a wrong executable hash. All five were
rejected. This counts those exercised scenarios, not every validation guard
in the audit; it is not exhaustive negative-path coverage.

## Remaining gates

The game-facing meanings and legal ranges for stats, level, HP, abilities,
and spells still require DOS code traces and controlled in-game evidence.
The next separate PR must preserve the complete original byte buffer,
including unknown fields and stale pointers, when implementing disk
read/write. Do not reuse the runtime loader's pointer clearing, party-slot
allocation, or derived-counter updates for no-op export. Each actual save
write must identify the output copy and emit offsets and before/after bytes;
record game validation as UNVERIFIED until measured.

After the byte-identical reader/writer round trip, request a Play-pane slot
from VP before the edited-copy game proof (inbox i26449). Party assembly and
the desk web UI follow their own gates. No editor, writer, in-game load,
party assembly, or web UI is claimed here. `run_media_checks.sh` was not run
for this layout-only change; the read-only layout audit is the targeted check.
