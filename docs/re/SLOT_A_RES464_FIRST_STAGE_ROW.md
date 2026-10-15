# Saved slot-A: first dirty row at RES_0464:1B52

The saved first view reaches resident `0464:1B52` with video state one.
The bounded C prefix scans to the first dirty row, stages only that row's
four offscreen-object lanes into `A200`, and stops before the next row
decision at `+1D27`. It does not write the page, reset metadata, or
produce a visible frame.

The private same-run flush-entry and immediate-return captures show row
16 is first dirty. Its source words are 1352..1395, its staging offset
is `0x03C3`, and each plane receives eleven bytes. Starting from the
native entry object and native pre-staging planes, the C prefix matches
**44/44** corresponding bytes in the native post-return staging planes.
Native value changes by plane are 11, 11, zero, and 11 bytes. There is
no native stop immediately at `+1D27`; the output comparison is at the
later immediate procedure return. The previously merged all-dirty model
independently covers the rest of the reached transfer.

Before tracing, `deja "Poolrad saved slot A U3BD +0AD5 object to page
RES_0464 1B52 transfer entry same-run snapshots"` found no indexed
session. Reused the merged [last-six placement return](SLOT_A_U3BD_VIEW4_LAST6_COMPOSITION.md),
[all-dirty transfer](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), and the
approved [view-three loop object stop](SLOT_A_U3BD_VIEW3_NEXT_WRITER.md).
The first-row prefix is a separate boundary from the queued native
single-byte watch of the earlier group-two tile.

## Original instruction path

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The full resident `+1B52..+1E5B` body is `START.EXE [0x61CB,0x64D5)`,
778 bytes, SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
The bounded prefix is `+1B52..+1D27`, file `[0x61CB,0x63A0)`, 469 bytes,
SHA-256
`4a9724b105a51c627c182d056235edba06e19830cb4c7ee7ae7bdeb2d0bcacae`.

At `+1B58..+1BCE`, the video-state byte `DS:[4A1E]` selects the
state-one branch. `+1BD7` sets row index zero; `+1BE1..+1BEB` scans
`DS:[4A28+row]` until dirty. On the captured input, rows 0..15 are
clear and row 16 is set. `+1BEE..+1C06` combines the row's staging base
and low offset, adding one. `+1C09..+1C0F` selects map mask one. The
first lane reads interleaved object bytes at `+1C2B` and stores them
at `+1C37`; the other stores are `+1C7E`, `+1CC6`, and `+1D0F`, under
map masks two, four, and eight. Each source index advances by four,
each staging offset by one. The `+1D25` loop test reaches `+1D27`
after the fourth lane's eleventh byte. The C prefix stops there.

The original video-state-one path performs other later rows and then a
runtime Move into `A000`. The C prefix never invokes those effects.
Its host bounds and Graphics Controller checks are explicit supported
route limits, not claimed DOS fault behavior.

## Measured native/C boundary

The ignored same-run captures are under
`resources/oracle-runs/slot-a-view4-last6-compose-p5-001/`:

| Capture | SHA-256 |
| --- | --- |
| Flush-entry 640-KiB RAM | `48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7` |
| Immediate-return 640-KiB RAM | `db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f` |
| Entry and return 14,135-byte object | `489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1` |

The entry stack carries return `2D14:05FB` and outer return
`2D14:0DAE`; `DS=14A0`, the object pointer is physical `0x3DE48`,
and `DS:[25F6..25F9]` selects map masks one, two, four, eight. The
four private pre/post `A200` plane captures are individually hash
locked in the audit. It copies each pre plane into C, stages only row
16, checks 44 predicted bytes against the corresponding native
post-return positions, and checks all other C staging bytes remain
equal to their pre-inputs. The native object remains identical across
entry and return; full native RAM changes at 469 addresses, which this
prefix does not reproduce or attribute. The Graphics Controller values
zero/zero/zero/zero/zero/`0xFF` were read at the same address in a
**separate** saved-route capture; they are not a same-run measurement
for these particular plane files.

At the approved `+0AD5` object stop the object hash is
`f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d`.
The prior third-placement return has object hash
`8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d`;
before this transfer entry another 615 object bytes change. This
prefix takes the **native flush-entry object** as input. It does not
predict those intervening object changes, claim a continuous all-RAM
join from `+0AD5`, or establish page, palette, RGB, or full-frame parity.
It adds no strict full-C or game-identity count.

## Reproduce

Run `python3 tools/replay/resident_464_first_stage_row_audit.py` with
the ignored DOS binaries and named native captures present. Configure
with `-DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'`, build
`poolrad_resident_464_first_stage_row_tests`, and run its focused CTest.
The test uses synthetic object and plane data, including row selection,
four lanes, unchanged page buffers, and rejection before writes.
