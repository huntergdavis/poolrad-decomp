# Slot-A party string boundary after the merged loop entry

This is a static source and ABI audit of the next loader step after
`pr_slot_a_party_entry_begin`. It does not execute the DOS loader, claim a
complete C bridge, or award strict full-C or saved one-Up parity credit.
`deja "poolrad unit 6f 191e 1926 party loader character sidecar post transfer"`
found no direct prior session. The merged [party-loop entry](SLOT_A_PARTY_ENTRY.md),
[party-input model](SLOT_A_PARTY_INPUT.md), [post-transfer step](SLOT_A_POST_TRANSFER.md),
and [string-runtime audit](U1C_HORIZONTAL_MENU_10D1.md) supplied the transfer
layout and previously decoded Pascal string operations. The call sites and
resident bodies below were independently checked against the pinned bytes.

| Ignored input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| unit `0x6F:+0x1965..+0x1973`, GAME file `0x1F94B..0x1F959` | 15 bytes | `8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126` |
| resident `0x0802:0x0356`, START file `0x083AD..0x0847B` | 207 bytes, 82 instructions | `ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889` |
| resident `0x0802:0x0425`, START file `0x0847C..0x08565` | 234 bytes, 91 instructions | `28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3` |

The current C entry stops before `+0x1965`. That call is SYSTEM
`0x0AF8:0x0B0B`: it copies the private, one-character Pascal literal at
`CS:+0x15E7` into `SS:[BP-0x214]` and retains the destination far pointer.
The literal's two-byte span has SHA-256
`5598b3f30c8e047bfd81991dedffa70603c790fcf6f4414d144e6b0d8caeab5f`;
its text stays in ignored host inputs. Thus `+0x1965` is the first source
instruction absent from the C loader composition, and that SYSTEM copy must
be represented in the composed route. The first resident helper still to
integrate is the next instruction, `+0x196A`, which calls `0x0802:0x0356`.
The third call, `+0x196F`, invokes resident `0x0802:0x0425`.

## Resident argument and string flow

| Call | Far arguments at the resident frame | Return and result |
| --- | --- | --- |
| `+0x196A` to `0x0356` | `[BP+6]` one-character search string from caller local `-0x214`; `[BP+0xA]` selected 41-byte Pascal name slot; `[BP+0xE]` output caller local `-0x314` | `RETF 8` leaves the output pointer for the next call. |
| `+0x196F` to `0x0425` | `[BP+6]` intermediate at caller local `-0x314`; `[BP+0xA]` output caller local `-0x414` | `RETF 4` leaves the output pointer for the later bounded store. |

At `0x0356`, two `0x0B25` calls copy the source and search string to
255-character locals. The routine computes a **byte** loop bound of
`source_original_length - search_length + 1`, then scans one-based positions.
At each position, `0x0B57` copies a substring of the *mutable* source with
`search_length` characters, `0x0C10` compares it with the search string, and
an equal comparison calls `0x0CBC` to delete at that position. The loop
advances its index even after deletion and uses the original bound. A final
`0x0B25` copies the result to the far output with cap `0xFF`. The saved
literal has length one, so its bound equals the original name length.
For other inputs, the byte arithmetic and runtime clipping still matter;
this is not an unqualified global replace operation.

The independent runtime decode confirms `0x0B57` copies a Pascal substring
(START file `0x0B7CD..0x0B80D`, 65 bytes, SHA-256
`18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa`),
consistent with the merged [substring model](U3CE_ENTRY4_01C8.md).
`0x0CBC` uses prefix and suffix substring copies, concatenates, and stores
back to the target string (START file `0x0B932..0x0B987`, 86 bytes, SHA-256
`14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33`).
The visible `0x0356` body and these runtime calls have only Pascal string
buffer effects; no direct DS/window write or file I/O appears in them.

At `0x0425`, the routine copies its input to a 255-character local, loops
`i=1..10`, reads the **live** byte `DS:[0x0D19+i]`, converts it to a Pascal
single-character search string through SYSTEM `0x0C3B`, and calls `0x0356`.
Each result replaces the mutable local before the next table byte. It then
copies the first eight characters (`0x0B57`), uppercases each copied byte
through SYSTEM `0x182A`, and stores the result to its far output with a
`0x28` cap. The table bytes come from runtime DS; neither the save nor the
current C entry provides them. Embedding their observed values in public C
would not model the call's input contract.

## Minimal host inputs and next gates

For a bounded direct composition through `+0x1973`, the merged entry already
supplies the transferred count and selected Pascal name slot. The host must
also supply the private one-character literal and either the ten live table
bytes `DS:+0x0D1A..+0x0D23` or a read callback with the same order. The
composition needs separate Pascal scratch and output buffers so the retained
far pointers and the following `+0x1974` `0x28`-cap store are represented.
No file or heap callback is needed inside these two string helpers.

The caller next builds a filename, tests it through resident `0x0802:0x02EB`
at `+0x19A8`, and conditionally allocates, loads and inserts a character
record through `+0x19B1..+0x19E7`, including the still unresolved `+0x02AE`
post-insertion call. The existing [party-input model](SLOT_A_PARTY_INPUT.md)
models the bounded resulting chain but skips sidecars and `+0x02AE`.
The separate [native watch in PR #242](https://github.com/huntergdavis/poolrad-decomp/pull/242)
measured the far window-1 word
`DS:[0x49D6]+0x067C` as 0 immediately after the clear at `+0x191E` and 6
after the loop at `+0x19F7` on one pinned slot-A run. These are endpoints,
not an intermediate write trace or C parity proof.

The separate resident C workstream owns `0x0356` (coordination inbox i697).
This note adds no C code and makes no claim about
the later `0x0425` table values, filename existence, sidecar reads, record
bytes, or post-load effects.
