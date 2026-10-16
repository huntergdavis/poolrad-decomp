# U19 row-14 internal four-plane witness resolver

This source-only follow-on makes the proposed selected A000:118F witness check the active host write handler and its resolved backing index. It extends the [merged plane-integrity gate](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md) and the [merged selected-store route](U19_ROW14_SELECTED_STORE_JOIN.md). It does not run the game or establish a store, plane delta, page, or RGB result. Heavy native remains held.

A bounded deja search for Poolrad U19 row14 nonperturbing four-plane latch capture was attempted before this source review; the local index produced no result and was stopped after several minutes. Reused work is PR #722's latch/read-path warning and PR #709's pinned first-byte host probe. The private source remains at revision 5a9c7c53481c2e933948039c33256776653f4faf.

## New source boundary

The read-only resolver locks src/hardware/vga_memory.cpp SHA-256 f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14 and these exact line windows:

| Lines | SHA-256 | Relevance |
| --- | --- | --- |
| 331–385 | bdeed38164872c251e714d1a3b778a5f6c3a836e932f78e72a0209fb07e15efc | Generic unchained write remaps the plane index, consumes mode/latch state, then updates a four-byte backing word. |
| 580–665 | c68333a8194bdd04e78c09ed187dd3279ca4e17bd6585f18fc84ba37abb1b667 | Both unchained handlers map the guest address using the host's bank_read_full field, including the write path; the fast handler uses a different index mask and write operation. |
| 2491–2564 | 5ae149c01a7abe9eeb1081c22d6c9ee852d4ab02aad43dcf79223b2b79c93a7b | The existing private first-byte probe samples another address from internal backing memory without guest reads. It is a design reference, not a selected 118F capture. |
| 2815–2840 | 2eb7a3090afe61d6eb6b08e8d5fcd328238cd8c0949cca2f32ce0bb46ee8ced5 | Handler selection can choose uvga_fast when complexity, optimization and mode allow; another branch chooses uvga. |

The synthetic resolver accepts the already translated host physical address and separately measured page mask, bank, memory mask/size, mode flags, and handler identity. It yields a four-byte backing-word index or a typed unsupported/out-of-bounds failure. It does not translate A000:118F through paging and cannot infer handler identity from a DOS video mode or retained RAM. A probe copied from the earlier first-byte tool with a fixed index or guessed write-bank value could sample the wrong plane word. This source maps both unchained write variants with bank_read_full; that is a source fact, not a general hardware rule.

A later authorized same-run native stop must establish the actual handler pointer and host paging translation at the selected printer instruction, record all resolver inputs and the four bytes directly from internal backing memory before and after the one stepped instruction, plus latch and GC/Seq state. Reading the backing array is distinct from a guest A000 read; the latter loads the latch. The probe must report the actual resolved index at the host write and compare it with this independent resolver. Any mismatch, unsupported handler, out-of-range index, intervening write, or changed latch caused by observation is a typed blocker. Host observations remain private under ignored resources/.

Read-only reproduction from the primary isolated checkout:

    python3 tools/replay/cold_start_codeword_row14_internal_plane_resolver.py --source resources/dosbox-x-16a1-private-src
    python3 -m unittest tests.replay.test_cold_start_codeword_row14_internal_plane_resolver

Use the absolute private source path when running from a separate worktree without resources/. The tests cover both handler variants, bank and chain mapping, odd/even remap, extended-memory address bit, unsupported handler and backing bounds. No full C, writer, page, RGB or full-frame claim follows.
