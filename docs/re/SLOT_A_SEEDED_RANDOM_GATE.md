# Saved slot-A seeded RANDOM gate

This is a **local C state transition** at saved ECL PC `0x9B3A`. It composes
the restored world-save windows, the modeled ECL run/dispatcher and operand
loader, and the explicit-seed RANDOM adapter. It is not a C-native one-Up
replay: the C save adapter still stops before key dispatch, and the measured
seed is injected at this already-reached PC. No full-C or game-identity count
is claimed.

Before debugging, `deja "Poolrad saved slot A ECL RANDOM PC 0x9B3A seeded script runner window2 0x980F 3140872353"` found no exact earlier session.
This gate reuses and checks the [native seed/store watch](SLOT_A_ONE_UP_NATIVE_RANDOM_SEED.md),
the [save adapter transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md), the
[RANDOM handler and SYSTEM decode](U1C_RANDOM_0234.md), the
[run/dispatcher model](U1C_ECL_RUN_3171.md), and the
[quiet `0x6DCA` load-hook classification](ECL_LOAD_HOOK_CLASSIFIER.md).
The older supplied-state search probes in the RANDOM and load-hook notes are
not substituted for the measured one-Up pre-state.

## Hash-locked input and local result

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Saved script transfer, file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| RANDOM handler, `GAME.OVR` file `0x0F7A..0x0FD7` | 94 | `a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979` |

The standalone check accepts the save only under this clone's ignored
`resources/` and only with its exact SHA-256. The save adapter deep-copies
the ten transfer spans. The gate converts each two-byte window slot to a
little-endian C word and binds the script transfer directly; logical PC
`0x9900` is script byte zero. It supplies the independently measured
pre-RANDOM `RandSeed` **3140872353**. The active DOS run's stop at this PC
also establishes that `[0x49FF]` was clear for this dispatch; the isolated
runner starts with that zero stop byte. No other DOS runtime field is
invented to reach this handler.

| Boundary | Check |
| --- | --- |
| Before dispatch | PC `0x9B3A` is opcode `0x08`; saved window-2 word `0x980F` (byte `+0x21E`) is 16. |
| Operand loader | Literal low byte 13 yields range 14; destination operand assembles `0x980F`. |
| Seeded SYSTEM chain | Seed `3140872353 → 3238563622`; returned low byte 10. This matches the native call/return watch. |
| Word store | Window-2 `+0x21E` becomes 10; the handler reports one store. |
| Bounded run exit | After one dispatch, PC is `0x9B40`; the runner fetches opcode `0x03` (COMPARE), with previous latch `0x08`, then returns its **host step limit** before executing COMPARE. |

The next fetch is part of the modeled run loop, not a DOS stop. A synthetic
script with different operands and destination checks the same composition
and verifies that a missing seed stops at the reached SYSTEM boundary
without writing the target word.

## Next missing live state

COMPARE at `0x9B40` reads script word `0x6DCA`, window-1 byte `+0x594`,
then evaluates the following IF at `0x9B46`. The saved word is zero; the
separate native coordinate watch also measured zero *after* one Up. This
clone has no pre-RANDOM RAM checkpoint for that word. The key route can
write `+0x594`, and the C save adapter explicitly stops at unit
`0x1C:+0x3395` before its key dispatch to unit `0x68` entry 2. Thus the
first missing **live state transfer** for continuing this C run is the
post-key, pre-COMPARE `0x6DCA` word (and any other key-route effects), not
the `0x6DCA` load-hook body: the merged classifier proves that hook is quiet.
The local `16→10` RANDOM transition and fetched COMPARE do not establish the
COMPARE result, later IF path, or one-Up parity.

## Reproduction

Run `bash tests/replay/check_slot_a_seeded_random_gate.sh` for a strict
C11 synthetic test. Supply this clone's ignored `resources/DOS/POOLRAD/SAVGAMA.DAT`
as its sole argument for the hash-locked numeric gate. The script compiles
with `-Wall -Wextra -Wpedantic -Werror`; it emits only addresses and numeric
state, and writes the binary only under ignored `resources/`.
