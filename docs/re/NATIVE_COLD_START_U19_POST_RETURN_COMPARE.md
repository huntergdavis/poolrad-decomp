# Native unit 0x19 continuation after one Return

One DOS v1.3 no-argument cold start delivered exactly one Return at the pinned
credits frame. The BIOS queue held one `0x1C0D` word at resident `0824:0122`.
After the [measured keyboard read](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md),
unit `0x19` returned from line input at live `38D3:0302`. It then compared an
empty input Pascal string with a selected six-character record field. The
runtime comparison returned **unequal**, but two original NOP instructions
and an unconditional jump led directly to the unit epilogue and resident
`0824:0127`. This is the first reached comparison boundary after the read;
it is **not** evidence of an effective code-word rejection branch, general
validation semantics, C-native parity, or a new game-function count.

Before debugging, `deja "Pool of Radiance unit 0x19 0x709 0AFD post Return
validation prompt 38D3 0302"` found no focused match; a narrower
`deja "0x709 0AFD"` returned only this current conversation. The [preceding
read trace](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md) supplied the verified
`0F2D:0AFD → 38D3:0302` lead. The [resident string evidence](RESIDENT_STRING_0356.md)
had already decoded `0xAF8:0x0B25` as capped Pascal-string copy and
`0xAF8:0x0C10` as length-and-content equality. This run independently
rehashed those original helper bodies and checked the reached caller frame,
input lengths, flags, jump target, and resident return.

## Pinned inputs and original code

| Private input or tool | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| copied-input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private DOSBox-X 2026.01.02 heavy executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The clean ignored run is `resources/oracle-runs/native-codeword-return-007`.
The manifest verified before and after the run. Unit `0x19`'s original
`GAME.OVR` code starts at file `0x09AC`, spans 862 bytes, and hashes
`f93ae8c41b823421115bf6d882588ac3577095d1f9ddace6f487a3bcae33291b`.
The 87-byte original continuation from unit `+0x0302` through its `+0x0358`
far return hashes
`ab7f8de4771ca1b46456b970bebb4ea3dd4ea3c2810c91b6abaabd89adeb4f93`.
The corresponding live unit was loaded at segment `0x38D3`; original
runtime segment `0xAF8` was relocated to live `0x131C`.

| Original span | Length | SHA-256 | Reached interpretation |
| --- | ---: | --- | --- |
| unit `+0x030B..+0x030F` | 5 | `6d7fadfa225a8fcbe50f8aa9bd23a82b01605ad36537649b8c460d85459776a1` | far call to `0xAF8:0x0B25`; maximum `0x28` pushed |
| unit `+0x0327..+0x032B` | 5 | `f97789de843be90137fed8b436c40cbc9cbf214dfe74a56f37ae447222ddb69b` | far call to `0xAF8:0x0C10` |
| unit `+0x032C..+0x032F` | 4 | `c1b3cc54d4ebf79b7af91fec26f6671fe0e3368fe00f158796423fb057bd2907` | two NOPs; unconditional jump to `+0x0355` |
| unit `+0x0330..+0x0354` | 37 | `9881d435c41a1b58108ad19e0f9af7bd63a7cf64681a96b48fc0b953e6e02a25` | adjacent block skipped in this run |
| unit `+0x0355..+0x0358` | 4 | `d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a` | far-return epilogue |
| original START file `0x0B79B..0x0B7BE` | 36 | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` | runtime `0xAF8:0x0B25` body |
| original START file `0x0B886..0x0B8B0` | 43 | `11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58` | runtime `0xAF8:0x0C10` body |

The live `+0x0327` call hash is
`28e67eebe78b1491d61c7e12081f7312690c422763d9f537a990e1f8acd38957`
after segment relocation. Live `+0x032C..+0x032F` still hashes
`c1b3cc54d4ebf79b7af91fec26f6671fe0e3368fe00f158796423fb057bd2907`.
No game bytes are reproduced here.

## Exactly-one-Return live path

At the pinned credits RGB SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`,
one **focused XTEST** Return was delivered. Physical BIOS queue words
`0x041A/0x041C` were `0x001E/0x0020` at resident `0824:0122`, and slot
`0x041E` held `0x1C0D`. At `38D3:0302`, the queue was `0x0020/0x0020`;
the old slot word remained. There were no additional guest keys. Guest BIOS
tick was 616911 at `+0x0122` and 616915 from `+0x0302` through `+0x0127`.

| Stop | Live observation | Queue head/tail | Private RAM SHA-256 |
| --- | --- | --- | --- |
| `0824:0122` | one Return queued | `001E/0020` | `27e326129dfa0fa441076d4aca97411fcc5d6e71bb63443eb96a8d5bc2aad16e` |
| `38D3:0302` | line-input callee returned; local copy follows | `0020/0020` | `8ef790ba3bbf762d1e3664f059add2cb2a1f0adb2583b9323b726d49914d464d` |
| `38D3:0327` | far call to `131C:0C10`; argument frame below | `0020/0020` | `ec70a66389597fb95113612a811cdf0099e5df6e048c567ae3c08fcadcf78258` |
| `38D3:032C` | comparison returned; `FLAGS=0x7297`, `ZF=0` | `0020/0020` | `800ded30ca478285c48f9f160fce51473f7f8f97fb90c91748778d79e2d90079` |
| `38D3:032E` | after both NOPs, `FLAGS=0x7297`, `SS:SP=1D16:3DB6` | `0020/0020` | same as `+0x032C` |
| `38D3:0355` | unconditional jump target and unit epilogue | `0020/0020` | — |
| `0824:0127` | unit returned; `SS:SP=1D16:3EFE` | `0020/0020` | same as `+0x032C` |

At `38D3:0327`, the comparison argument words at `1D16:3DAE` were far
pointers `14A0:00BF` and `1D16:3ECF`. The unit's selected record index was
9, so its `0x15 * index + 2` field offset was `0x00BF`. The selected record
field's Pascal length was 6, and its length-plus-content SHA-256 was
`26a1c233dfdba1f6c429ef42cf2874f5f562fd47cd382b34cf3910fec890c31e`.
The input local's Pascal length was 0, SHA-256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.
The comparison's `ZF=0` is consistent with these unequal lengths. The
record's text is kept private.

The live PC then visited `+0x032C`, `+0x032E`, `+0x0355`, and resident
`+0x0127` in that order. Thus the comparison result did not choose the
adjacent `+0x0330..+0x0354` block on this Return route. The paired private
RGB SHA-256 at `+0x0302`, `+0x0327`, `+0x032C`, and `+0x0127` was
`8d252322504d47f53340236ff68a61a72ea9cad518a9bc4b89e028e04f0b53d5`.
Those sampled frames establish no screen change at these stops; they do not
rule out transient rendering between them. The next resident call at
`+0x0127` belongs to the separately traced menu route.

## Reproduction boundary

Prepare a fresh ignored copy with `tools/oracle/prepare_run.sh` and launch
the pinned private heavy DOSBox-X with `DEBUGBOX START`, as in the [native
startup procedure](NATIVE_COLD_START_U17_PICTURE_ROUTE.md). At MZ entry,
arm `BP 0824:0122` and run. When the credits RGB hash above matches, focus
the DOSBox X window and send exactly one `xdotool key --clearmodifiers Return`
without `--window`. At `+0x0122`, verify the one-entry BIOS queue before
arming `BP 38D3:0302`, `BP 38D3:0327`, `BP 38D3:032C`,
`BP 38D3:032E`, `BP 38D3:0355`, and `BP 0824:0127`. At each reached stop,
record `EV CS`, `EV IP`, `EV SS`, `EV SP`, `EV BP`, `EV FLAGS`, and
`MEMDUMPBIN 0000 0000 A0000`; move each dump before the next one.

An earlier diagnostic `native-codeword-return-006` used a window-directed
host key command. Its queue was empty at `+0x0122` and advanced by two slots
by `+0x0302`; that attempt is **excluded** from the one-Return evidence.
The clean run used one focused XTEST event and showed exactly one queued slot
before input and one consumed slot after it. Original media, RAM images,
screenshots, and emulator traces remain ignored under `resources/`.
