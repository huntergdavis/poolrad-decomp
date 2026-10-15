# Unit 0x6A +0824: original/C and synthetic native follow-up

This follow-up strengthens the complete translation from
[PR #391](https://github.com/huntergdavis/poolrad-decomp/pull/391) without
claiming a reached saved-game frame or advancing strict routine counts.
The remaining promotion decision belongs to PM and independent review.
The existing [source and behavior note](U6A_SORT_0824.md) remains applicable.

## Scope and source

Work began at main `75d24676525ad0a984e66cd14764de87638d96f2`.
The initial u7F target stopped at the bounded blocker recorded in
[its native-reach note](U7F_05E0_NATIVE_REACH_FULLC_GATE.md); the authorized
follow-on is u6A:+0824. Coordination with the owner of u6A:+025C confirmed
disjoint ownership and that their native evidence uses synthetic COM frames,
not a reached overlay segment. All follow-on files have unique names;
the existing C/header and shared CMake files are unchanged.

Original `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete leaf is `[0x18055,0x180F0)`, 155 bytes / 57 instruction starts,
SHA-256 `c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7`.
The aligned near caller is `+0A4E`, preceded by `PUSH CS` at `+0A4D`,
returning to `+0A51`. The surrounding `+090A` caller writes the selected
whole byte into the active record's 21-byte array before sorting it.
Spell-selection meaning is an inference from surrounding control flow,
not a measured gameplay identity.

## Original-byte and independent property checks

`tests/recovered/check_u6a_0824_fullc_gate.py` reuses PR #391's hash lock,
CPU layout, C build helper and fixtures. Its new original executor maps a
coherent high-memory alias for A20-disabled physical wrapping. This avoids
misclassifying Unicorn's unmapped linear address above 1 MiB as a C defect.

The 128 deterministic cases vary all input data bytes, scratch registers,
legal arithmetic/direction flags, stack return wrapping, array offset
wrapping and physical A20 wrapping. Each compares the entire 1 MiB RAM and
all 14 returned CPU words, including unmasked flags. Independently, each
checks ascending low-seven-bit keys, whole-byte multiplicities, preserved
registers, final flags and writes confined to the array and defined stack
frame bytes. Invalid arguments must leave both RAM and CPU unchanged.

All 57 instruction starts are covered. Seven of eight conditional outcomes
are required: the taken `JA` at `+0838` needs local index >20, which the
disjoint fixtures cannot produce (their outer index runs 0..19). That
alias-only outcome is explicitly unmeasured. Existing PR #391 cases retain
the live pointer-table alias check; they are not replaced by sort properties
that assume disjoint storage.

## Synthetic native oracle

`tools/overlay/u6a_0824_synthetic_native.py` adapts the capture pattern from
[PR #407](https://github.com/huntergdavis/poolrad-decomp/pull/407),
`tools/overlay/u6a_record_remaining_025c_audit.py`: assemble a private COM,
capture returned CPU/frame/data before DOS output changes them, then compare
C from identical synthetic inputs. Only the generator is public.

The original 155-byte body is inserted unchanged at COM offset `+0824` and
checked against the pinned source. The 128 fixtures include mixed whole-byte
values, masked ties and both incoming direction-flag states. The comparator
requires all 14 CPU words, ten frame bytes, the live far pointer and all
21 array bytes to match, without a flag mask. It does not capture full native
RAM, establish party ownership, or execute the saved-game caller.
All 128 cases pass. Ignored artifact hashes:

| Artifact | SHA-256 |
| --- | --- |
| DOSBox-X executable | `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3` |
| Generated `ORACLE.COM` | `164c581d8017595483a27749525b6281d51a6a451ebd9196cab7ca0196551e68` |
| 8324-byte `RESULT.BIN` | `97d8957494a54ac6654b2a4c8b3fca0dd4f84e69d0db91c247b686ca0e7f3a04` |

## Bounded saved-game attempt and exact blocker

The ignored run `resources/oracle-runs/u6a-0824-fullc-001` used a fresh
hash-verified copy of the original media, a private X display, the heavy
DOSBox-X debugger and Bubblewrap with networking disabled. It had an outer
600-second bound. Initial CPU was `CS:IP=1796:0012`, `DS=ES=0814`,
`SS:SP=193E:0080`; the PSP-derived load base was `0824`. Breakpoints were
armed at the three relocated public stubs `0899:0025`, `0899:002A` and
`0899:002F`, then execution resumed. The captured root image was black;
no verified saved-game menu, public-stub stop, dynamically hash-verified
unit body or leaf entry/return pair was obtained. The run expired with
status 124. Queued autotype/UI keys do not establish a selected menu route.
No guest code or registers were edited.

The exact outstanding gate is a reached caller/leaf with the original
unit body identified at its runtime segment, an entry stack returning to
`+0A51`, and entry/return RAM plus CPU captures. Synthetic native parity
cannot substitute for that evidence.

## Reproduction and capture gate

With private original media and Capstone/Unicorn installed:

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_u6a_0824_fullc_gate.py
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/test_u6a_0824_fullc_gate.py
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u6a_0824_synthetic_native.py
```

The native replay gate requires `--require-native --native PRIVATE_DIR`.
Without the directory strict mode exits 2. Ordinary stress-test success prints
`UNMEASURED` when a native capture is absent; it does not assert strict credit.

The directory must contain `entry.bin` and `return.bin` (exactly 1 MiB each),
`debugger.log`, and `manifest.json`. Manifest fields are `origin` equal to
`dosbox-x-native-breakpoints`, pinned `game_ovr_sha256` and `body_sha256`,
`debugger_log_sha256`, `entry_sha256`, `return_sha256`, and `entry_cpu` /
`return_cpu` dictionaries with all fourteen lowercase CPU field names and
unsigned 16-bit integer values. The final two EV snapshots in the hashed
debugger log must use order `CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS`
and agree exactly with those dictionaries.

The gate validates loaded body/caller bytes, entry/return IP and far frame,
an active pointer observed in the party linked list, pointer stability,
original-byte/C replay and C/native full-RAM/CPU equality. Eight focused
tests exercise valid synthetic replay plumbing and rejection of synthetic
origin, wrong caller/body, missing ownership, changed returned RAM/flags,
and missing native CLI evidence. Their temporary original-derived fixtures
stay under ignored `resources/`.

Hashes and an origin label do not authenticate how a capture was produced.
The positive plumbing test deliberately supplies a native-origin label to a
synthetic fixture, demonstrating that limitation. Review must independently
establish the capture's debugger run, reached route, emulator identity and
absence of guest edits before accepting a promotion.
