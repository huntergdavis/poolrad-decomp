# Saved slot-A RANDOM-to-EXIT ECL gate

One bounded `pr_rec_ecl_run` call now executes the already-merged RANDOM,
COMPARE, IF and EXIT models from saved ECL PC `0x9B3A` to the runner return
after PC `0x9B68`. It uses the hash-pinned saved script, explicit measured
RNG seed, and selected native post-key state. All eight modeled handler
transitions match the controlled run010 checkpoints. This does **not** connect
the C key dispatch to `0x9B3A`, reproduce the complete one-Up route, or earn
strict full-C or game-identity credit.

Before this composition, `deja "Poolrad saved slot A ECL 0x9B3A RANDOM
COMPARE IF EXIT runner return run010 composition"` found no exact session.
Reused [seeded RANDOM](SLOT_A_SEEDED_RANDOM_GATE.md),
[COMPARE/IF continuation](SLOT_A_COMPARE_CONTINUATION.md),
[the EXIT gate](SLOT_A_EXIT_GATE.md),
[the controlled native AL10 branch](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md),
[the save transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md), and the
[ECL runner contract](U1C_ECL_RUN_3171.md). The ignored run010 RAM and DOS
files were read only. No original bytes are in this public tree.

## Inputs and source boundary

| Ignored input/span | Size | SHA-256 |
| --- | ---: | --- |
| slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| saved script transfer, file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| RANDOM unit `0x1C:+0234..+0291`, file `0x0F7A..0x0FD7` | 94 | `a634c69e97fe005c2041e9d18d0874d0eb8ede7f4a59833bfb642918b7c5d979` |
| COMPARE unit `0x1C:+0126..+01A2`, file `0x0E6C..0x0EE8` | 125 | `330e74c38244df80bd2af04209126a207b5ebdf5dcf97c4781d4b960e2a52404` |
| IF unit `0x1C:+0B52..+0BC7`, file `0x1898..0x190D` | 118 | `11830c28e2ca43d40e80db72b6374a46e73b4e0cc0522ba9728e7026e3718061` |
| EXIT unit `0x1C:+0001..+00F3`, file `0x0D47..0x0E39` | 243 | `9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493` |

The controlled run010's `+026C` stop has the restored seed
`3140872353`, prior/current latch `00/08`, and PC `0x9B40` **inside** the
RANDOM handler because its operand loader already advanced the PC. The C
gate deliberately starts the runner at opcode `0x08` at `0x9B3A` and lets
the merged loader repeat that advance. The body before `+026C` loads the
operands and computes the range/destination; it has not stored the RANDOM
result into a window. The C gate binds the save's script/windows, including
window-2 `+0x21E=16`, then supplies these separately measured values:

| C input | Native source and width |
| --- | --- |
| RNG seed `3140872353` | four-byte DS:`4428` at the controlled seed-restored `+026C` stop; run010 RAM SHA `3ee6a79b9a53a7401d2679fd803635ef2df144a0d3f17f5c9b29cbd376fd108c` |
| window-0 minute `+0x18E=1` | **word** in that post-key RAM, versus 0 in the saved transfer; tens/hour words remain saved 5/10 |
| window-1 `+0x594=0`, window-0 `+0x216=0`, window-0 `+0x1CC=1` | words in the save and independently in pre-RANDOM RAM |
| six compare flags `(0,1,1,0,1,0)` | bytes DS:`6F78..6F7D` in pre-RANDOM RAM |
| pre-fetch latch `0x00` | inferred from measured prior/current `00/08` inside RANDOM after the runner copies the prior latch; no handler in this slice uses the prior byte |
| stop `[49FF]=0`, box gate `[84E2]=0`, character gate `[4430]=0`, nil call-list head `[6F70:6F72]=0:0`, state `[5E24:5E25]=1:17` | **bytes** except the two call-list words, all read at the pre-RANDOM RAM stop and unchanged at EXIT entry |

The original fresh run had another RNG seed; the debugger restored only
DS:`4428..442B` to the earlier genuine-run seed before continuing natively
([controlled trace](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md)). Thus the branch
comparison below is with that controlled continuation, not an unmodified
replay. The model never injects the expected RNG result, flags, final PC, or
final window tuple; it computes them through the callbacks and tests them
afterward. The box hook, active-character pointer restore and FreeMem are
not reached on these measured gate values.

## Computed route and native checkpoints

The C callback log records input PC and prior/current opcode, post-handler
PC, flags, six watched window words, and the stop byte for every step. The
watched tuple is `(minute,tens,hour,window1+0x594,window2+0x21E,
window0+0x216)`: `(1,5,10,0,16,0)` before RANDOM, then
`(1,5,10,0,10,0)` after its single store and throughout COMPARE/IF/EXIT.
Flags remain `(0,1,1,0,1,0)`. The RNG reports range argument 14,
destination `0x980F`, result 10, seed `3140872353 -> 3238563622`, and one
word store. `0x16` IF skips the following statement twice; `0x1A` IF proceeds
to EXIT.

| C handler PC | Opcode | Computed successor | Native RAM checkpoint SHA-256 at next dispatch |
| ---: | ---: | ---: | --- |
| `0x9B3A` | `08` RANDOM | `0x9B40` | `26ea01908c6c4b9e512687ff5d9a2dfc3876d9834c27480ae7c1805b9c7ab5e8` |
| `0x9B40` | `03` COMPARE | `0x9B46` | `9c3a223bd89941cabe4bc2410f29dc21cfced283a13a3eb8a0ee6c2cb21a9bb7` |
| `0x9B46` | `16` IF | `0x9B50` | `9e7cb4f44ee120a836bbd2c5d1ce6c5c4f4c7f4ed0f7c2378b744e87d5ea7672` |
| `0x9B50` | `03` COMPARE | `0x9B56` | `c5cf5066193216df0cd480dad0de485aed18bbff448b8545c10fd40e0503d926` |
| `0x9B56` | `16` IF | `0x9B60` | `b931382d4e68b25233ec64d23412f12e8b3ca49f09e74697707f7a98d529adff` |
| `0x9B60` | `03` COMPARE | `0x9B66` | `a208851898a27a6d1034d5c795166e6c10c1a557fddcbddf49f84872161738c7` |
| `0x9B66` | `1A` IF | `0x9B67` | `bae33746a473a4b2c28aa40097095f67615cf1552641b0a6002bf02bbe4500a8` |
| `0x9B67` | `00` EXIT | `0x9B68` | EXIT-entry `7d9303e803eaf54ecf8d30db5aa339a2107546543c1678debf1eee5189196091`; runner `+31AA` `12a715b7ee9ca93844ed2edd0127a10353e4df38f0f389d245d985f32264557a`; `+31B0` `d43b75633114880a24480a73a64bc623bbadb736729d39d78d2612e94f2c2d12` |

The native post-store `+028E` RAM SHA is
`d33e3eaf1968a4f656f6df1304a4348eae620bbf0a2655bbec39178efb95a62b`.
The Python auditor verifies this and all seven dispatcher snapshots by
full RAM hash, PC, opcode pair, seed, flags, far-pointer word widths, watched
window words and reached EXIT gates. It then invokes the existing EXIT
auditor for snapshots `06..09`. EXIT sets `[442E]=1` inside its callback;
the runner clears it to 0 before returning, preserving PC `0x9B68` and
leaving all windows unchanged after RANDOM's one store.

## Reproduction and limit

`bash tests/replay/check_slot_a_random_exit_chain.sh` compiles a synthetic
four-handler script under C11 `-Wall -Wextra -Wpedantic -Werror`, including
a missing-seed case that stops before the store. Passing the ignored
slot-A save adds the full eight-dispatch C gate. A second argument naming
the ignored run010 directory adds both hash-locked native audits. Only
the focused C test and numeric/hashes are printed; build outputs stay in
ignored `resources/`.

This is a **bounded composition of models** with explicit post-key input.
The key dispatch, real runtime RNG state transfer from the save, and the
subsequent caller path are still separate gates. No C-native one-Up parity
or new routine count follows from this local script suffix.
