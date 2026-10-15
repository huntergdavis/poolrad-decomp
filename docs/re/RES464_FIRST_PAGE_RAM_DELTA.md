# Saved slot-A first-page flush: conventional RAM deltas

Between the saved first-view `RES_0464:1B52` flush entry and its immediate
`2D14:05FB` return, **469 of 655,360 conventional-RAM bytes** differ. All
469 changed addresses fit six short ranges: 440 bytes in dirty-row metadata,
28 bytes below the entry stack pointer, and one byte at the BIOS timer
location. No other conventional-RAM address has a different final value.
This classifies snapshot deltas; it does not attribute every write to an
instruction or measure a first `A000` store.

The required bounded `deja "Poolrad saved slot A first page flush
conventional RAM delta dirty metadata stack A200 A000 0x16A1"` returned no
indexed sessions. This audit reuses the merged
[same-run placement/flush boundary](SLOT_A_U3BD_VIEW4_LAST6_COMPOSITION.md)
and [all-dirty transfer model](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md),
including its dirty-row field layout and immediate-return stop.

## Pinned capture and complete partition

The ignored run is `resources/oracle-runs/slot-a-view4-last6-compose-p5-001`.
Both full 640-KiB RAM files are hash-locked before comparison:

| Stop | SHA-256 |
| --- | --- |
| `0C88:1B52` entry, `ram-flush-entry.bin` | `48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7` |
| `2D14:05FB` immediate return, `ram-flush-return.bin` | `db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f` |

| Address class | Physical inclusive range | Changed bytes |
| --- | --- | ---: |
| BIOS data-area timer at `0040:006C` | `0046C` | 1 |
| `DS=14A0`, dirty flags for rows 16..103 | `19438..1948F` | 88 |
| `DS=14A0`, first-extent words for those rows | `19640..196EF` | 176 |
| `DS=14A0`, last-extent words for those rows | `19790..1983F` | 176 |
| `SS=1D16`, below entry `SP=3EBE` | `20FD6..20FE7` | 18 |
| `SS=1D16`, below entry `SP=3EBE` | `20FEA..20FF3` | 10 |

The read-only [classifier](../../tools/replay/res464_first_page_ram_delta.py)
checks both capture hashes, all 168 dirty-row records, every changed
physical address, and the disjoint six-range partition. For rows 16..103 it
checks that dirty flags and extent records take the expected reset state on
return; other rows' metadata must be unchanged. The stack ranges are
consistent with call-frame residue but their individual writer instructions
are not isolated by these two stops. The timer address is an external BIOS
clock location; the snapshot does not isolate an interrupt or its timing.

The complete [address-only CSV](res464_first_page_ram_delta.csv) has SHA-256
`0a5894ac4decf7d6afcda7fc9bd306db1620b06cdc82b9e92a2f3d19eccae043`.
It contains addresses and counts only. No captured RAM values, EGA planes,
screenshots, or original game bytes are published.

## Boundary

The conventional-RAM dump ends before planar EGA memory. The merged
all-dirty model separately compares native `A200`/`A000` plane captures;
this audit does not repeat or extend that plane parity. Equal bytes at the
two RAM stops could have been written and restored, and same-value writes
are invisible. The held direct native `0AF8:16A1` stop and first-byte page
write remain open. No native retry or full-routine credit follows here.

```sh
python3 -B tools/replay/res464_first_page_ram_delta.py
```
