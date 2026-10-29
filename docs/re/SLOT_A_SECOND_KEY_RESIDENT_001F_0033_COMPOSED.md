# Second-key resident caller composition `+001F..+0032`

This adapter composes the merged
[`+001F` caller prefix](SLOT_A_SECOND_KEY_RESIDENT_001F_CALLER_PREFIX.md),
[`+002A` call frame and `+002F` return](SLOT_A_SECOND_KEY_RESIDENT_002A_CALLER_RETURN.md),
and [`+0031` copy](SLOT_A_SECOND_KEY_RESIDENT_0031_CALLER_COPY.md).
It stops at loaded `10D2:0033`. The arithmetic helper at
`131C:0A5B` is a callback that **supplies** its returned CPU state
and RAM effects; this is conditional caller parity, not a translation
of that helper or the full second-key route.

## Source and native input

The required `deja "Poolrad RES_08AE 001F 0033 compose supplied native
arithmetic return state"` query found no indexed prior session. The
three linked notes supply the separately reviewed instruction models.

The controlled slot-A second-key replay used DOS v1.3 START.EXE
SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and the 13,137-byte slot-A SAVGAMA.DAT SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The direct same-run caller span is 20 original START bytes at file
`0x8B16..0x8B29`:
`[scrubbed: span=SLOT_A_SECOND_KEY_RESIDENT_001F_0033_COMPOSED-1 sha256=349f8d9ba972e240f5bb27986d59dfb930707d7ed8d56e9a578bdf02d6dc48f4 bytes=20 offset=START.EXE:0x8b16]`,
SHA-256
`349f8d9ba972e240f5bb27986d59dfb930707d7ed8d56e9a578bdf02d6dc48f4`.
The loaded RAM copy differs only in the relocated call segment
`1C 13` at `10D2:+002D..+002E`.

## Same-run boundary and supplied state

The 14 CPU words below are ordered
`CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`.

| Stop | CPU words | 640 KB RAM SHA-256 |
| --- | --- | --- |
| caller start `10D2:001F` | `10D2 001F 14A0 1D16 1D16 3D9A 3DA6 0001 0000 0100 2718 0001 3DA2 7246` | `770d855ea3c015ac208df429bf5ea6eeadffa87c346f1672aa885a6f65f99cfe` |
| arithmetic entry `131C:0A5B` | `131C 0A5B 14A0 1D16 1D16 3D96 3DA6 0001 0005 7E40 0000 0001 3DA2 7246` | `39aac77ba469b8f399149aec5e1c1a2bcb7d84b32c232fefeff6513af77c17bb` |
| **supplied** helper return `10D2:002F` | `10D2 002F 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 0000 0005 0001 0000 7206` | `420da85f0c5ed4da09e42dbe7f32443cb5f5b6b7135d3bf24532ed740c3a62f9` |
| final caller stop `10D2:0033` | `10D2 0033 14A0 1D16 1D16 3D9A 3DA6 7E40 0005 7E40 0005 0001 0000 7206` | same as supplied return |

The callback receives the CPU at the relocated helper entry. In the
ignored native-seeded harness, it compares the complete entry RAM and
CPU to the native stop, then injects the measured `+002F` return CPU
and the helper's **four-byte** RAM delta at SS:`3D92..3D95`.
The callback performs no caller instruction. The composition then
executes `MOV CX,AX` and `MOV BX,DX` and stops at `+0033`.

The strict C11 harness reports **0/655,360 RAM and 0/14 CPU-word
mismatches** at the helper entry, and **0/655,360 RAM and 0/14
CPU-word mismatches** at the final `+0033` stop. It also confirms the
callback was called once and supplied exactly four RAM bytes. The
registered synthetic CTest uses a different local WORD and helper
return values `AX=BEEFh`, `DX=A55Ah` to show the returned values flow
through the two caller copies. Private RAM captures and the harness
remain under ignored `resources/`.

The next instruction at `+0033`, arithmetic helper internals, and a
complete second-key or one-Up replay remain separate gates.

## Parity review

- **target_function:** bounded composed caller path
  `RES_08AE:+001F..+0032`, stopping at `+0033`.
- **source_candidates:** `pr_rec_res_08ae_001f_to_0033` calls the
  three merged instruction models and an explicit external helper
  callback.
- **signals:** 20 exact original bytes and relocated loaded copy;
  direct same-run start, helper-entry, return, and final stops;
  full entry/final RAM and CPU equality; synthetic distinct helper
  outputs.
- **score and verdict:** 4/5 corroborating signal groups,
  **conditional match** for the caller composition. Independent
  helper implementation and return parity are absent.
- **review_notes:** the callback owns all arithmetic return CPU and
  RAM effects; injecting the measured return does not verify the
  helper. `local_word` and `return_frame` must map to SS:`[BP-6]`
  and SS:`[SP-4..SP-1]`; the native harness checks both mappings.
  This adapter earns zero strict full-function credit.
