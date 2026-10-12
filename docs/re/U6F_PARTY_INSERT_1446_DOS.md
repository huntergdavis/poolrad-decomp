# Unit `0x6F:+0x1446` party insertion over 20-bit DOS RAM

The `pr_u6f_party_insert_1446_dos` entry follows the complete terminating
path of the 313-byte unit `0x6F:+0x1446..+0x157E` routine through one
byte-addressable, A20-off 1 MiB RAM view. It reads DS globals from that
view when each original instruction reaches them. Far record and window
pointers use the same memory, so segment aliases and earlier writes can
affect later reads. The previously merged
[bounded segment-view model](U6F_PARTY_INSERT_1446.md) remains available.
This branch requests parity review; it claims no strict full-C or identity
count before that review.

## Source and reused evidence

Pinned GAME.OVR is 206,105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The executable body is file `0x1F42C..0x1F564`, 313 bytes, SHA-256
`370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245`:
99 x86-16 instructions, ten internal branches, three far calls, and
`RETF 4` at `+0x157C`. The adjacent `+0x157F..+0x1601` data is excluded.
The current branch independently redecoded that span before changing the
model. Before debugging, `deja "Poolrad u6F +0x1446 party insertion faithful
20-bit memory aliasing u9B PR257"` found no exact match;
`deja "u6f 1446"` surfaced prior PM party-build research. Reused the
[PR #257 source audit](U6F_PARTY_INSERT_1446.md) and the merged
[first saved-case snapshots](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md).

The two SYSTEM helpers and final overlay callee were rechecked against
hash-pinned instructions. Loaded START.EXE `0xC628..0xC63B` (20 bytes,
SHA-256 `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`)
uses `REP STOSB` to clear eight local bytes. Loaded `0xBD40..0xBD60`
(33 bytes, SHA-256
`753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c`)
tests membership using only the pushed low byte, so the source's retained
AH does not change the slot decision. The set at file `0x1F40C..0x1F42B`
has exactly members `0..7`. Far call `0x00AC:0x0025` reaches the merged
[u9B five-step record updater](U9B_RECORD_INCR_01AF.md). Its five
threshold checks re-read the type byte and conditionally re-read and
increment counter bytes in source order. The new entry applies those steps
through the same physical RAM view, including any aliases created by the
window-word increment.

## Memory and write order

A byte access at `segment:offset+delta` first wraps the effective offset
to 16 bits, then calculates `((segment << 4) + effective) & 0xFFFFF`.
For a word access, byte two is the **next physical byte**, including across
the 20-bit boundary. This matches the local DOS emulator's linear-address
word access. The host supplies all 1 MiB before execution; there is no
per-access mapped-view failure or node-count cap.

| Source site | Parity-relevant order |
| --- | --- |
| `+0x144F..+0x145C` | Write incoming `+0xBF = 0xFF`, then read DS head offset and segment. The first write can alias the head. |
| `+0x1464..+0x1474` | Empty head writes the head offset, head segment, active offset, active segment as four separate words. |
| `+0x148A..+0x14B9` | Nonempty head tests both link words; on nonzero it re-reads both to advance. At the terminal node it writes incoming offset then segment. |
| `+0x14CA..+0x151C` | Clear eight local membership bytes, re-read the DS head, then read each node's slot, re-read it for an in-set mark, and read both next words. |
| `+0x1524..+0x1563` | Write active offset then segment; re-read that DS far pointer into a local; reset its slot, then increment the actual byte once per occupied slot. |
| `+0x1565..+0x1574` | Read the window pointer from DS after previous writes, increment its `+0x67C` word, then apply all five u9B steps through the original incoming far pointer. |

There is no host `max_nodes` parameter. A cyclic list does not terminate,
matching the decoded DOS loops. The only rejection is a missing or shorter
than 1 MiB host RAM view or a missing report pointer, before the first DOS
effect.

## Saved-case and synthetic verification

The hash-pinned first insertion RAM captures from
[the native return watch](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md) are each
655,360 bytes. The optional private test validates their published hashes,
copies the before image into a zero-padded 1 MiB view, executes with
`DS=0x14A0` and incoming `0x449A:0008`, then compares the full 285-byte
record, eight bytes of head/active DS globals, and the window counter word
with the after image. It observes an empty-list insertion, slot zero, and
window count one. This checks the relevant game-memory delta; the native
captures also contain stack and unrelated changes, so it makes no
whole-RAM equivalence claim.

Synthetic tests cover empty and multi-node lists, all eight occupied
slots, an incoming-slot write that aliases the DS head before the branch,
a window counter word that aliases u9B counters, and a word increment
crossing physical `0xFFFFF` to `0x00000`. The focused checker also reruns
the six-span original GAME/START hash gate and the set-membership assertion:

```sh
bash tests/recovered/check_overlay_u6f_party_insert_1446_dos.sh
bash tests/recovered/check_overlay_u6f_party_insert_1446_dos.sh \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE \
  resources/path/to/ignored-before-ram.bin \
  resources/path/to/ignored-after-ram.bin
```

The C API takes the incoming far pointer as a value and represents the
Pascal frame and eight-byte membership array as C locals. It does not
model arbitrary overlap of the active SS stack frame with DS globals or
record memory. The saved native frame is disjoint from the watched head,
record, and window fields; a corpus-wide stack non-overlap proof is still
open. That is the remaining qualification for strict credit if the review
requires parity under arbitrary SS aliasing. No private RAM, executable
bytes, saved data, or media are committed.
