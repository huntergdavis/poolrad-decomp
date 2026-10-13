# Native no-argument startup: credits return, code-word prompt, menu call

One DOS v1.3 no-argument cold start returned from the credits callee at
resident `0824:0122`, entered overlay unit `0x19` entry 1, returned at
`0824:0127` with the code-word prompt visible, and entered overlay unit
`0x6C` entry 1 before the main menu appeared. This is a native guest route
observation. It does not establish code-word validation, C parity, or a new
translated-routine count.

Before tracing, `deja "Pool of Radiance code word prompt menu native resident
startup"` found no focused match; `deja "code word"` recalled earlier native
blank-submit traces. The static unit mapping in [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md)
was a lead, but its no-argument classification for `+0x011D/+0x0122` was
already contradicted by the live [picture route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md).
The independent [screen survey](NATIVE_COLD_START_SCREEN_GAPS.md) supplies
the reference RGB hashes for credits, prompt, and menu. This run checked the
caller instruction, overlay stub, loaded entry, return stack, and frames.

## Pinned inputs and boundary

| Private input or tool | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| DOS v1.3 `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| copied-input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private DOSBox-X 2026.01.02 heavy build | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The fresh private copy was `native-codeword-menu-001`. `DEBUGBOX START` gave
the game no command-line argument. The MZ entry was `1796:0012`, giving image
load base `0x0824` as in the earlier route. One Return was sent while the
verified credits frame was visible; no further key was sent before the main
menu appeared. The key's eventual consumption and code-word acceptance path
were not traced. The copied-input manifest was unchanged after the run.

## Original and live call identity

The expanded original image `+0x011D..+0x012B` spans three consecutive
five-byte far calls and hashes
`ae3facbd2b350709dd23d5f987dc35d4692fe7e08bb9ded843904c4e1773eb52`.
Their segment operands are the loader's relocation sites `+0x0120`,
`+0x0125`, and `+0x012A`. No executable bytes are included here.

| Image call | Original five-byte SHA-256 | Original operand → live decoded target | Live five-byte SHA-256 |
| --- | --- | --- | --- |
| `+0x011D` | `08c75e3d9d8d592c2ffb668dbca1bc96bce437d06b429d1ed30756084cebfdb4` | `0019:0025` → `083D:0025` | [picture route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md) |
| `+0x0122` | `54e957158779fadaefe40e9504e2ea47cb99c7425ee051e8c2c849f2940596c7` | `001C:0025` → `0840:0025` | `8eae8facca4e09951d8568742355469f03771c781568754c37883c332fdb9e6e` |
| `+0x0127` | `d4e6ab6cf10a4ac9695887086e48ff6134a89186a521864dbee11e5f8d498792` | `0078:0025` → `089C:0025` | `52970b16ec5ee73264b5f7b2039e618cbd3ba5ba602f47c1b1f69596bec0b265` |

The packed MZ descriptors, checked against `GAME.OVR`, identify runtime stub
`0x1C` as unit `0x19` (code file offset `0x09AC`, 862 bytes, code SHA-256
`f93ae8c41b823421115bf6d882588ac3577095d1f9ddace6f487a3bcae33291b`),
whose entry-1 alias `+0x08FD` resolves to loaded `+0x00FD`. Runtime stub
`0x78` is unit `0x6C` (code file offset `0x1997F`, 17,497 bytes, code
SHA-256 `232c6aa60fda1a475d7933aabc97b598fba93d8e4af4026e3551bb0ef06045a0`),
entry 1 `+0x0161`. These are unit identities, not translations of either body.

## Ordered native stops and paired screens

Every listed screen hash is SHA-256 over the private screenshot's canonical
640×400 RGB bytes. Each paused point also has a full 640-KiB RAM dump kept
under ignored `resources/`; BIOS ticks come from physical `0x046C` in that
same dump. The screen and RAM were captured before resuming that stop.

| Stop | CS:IP, instruction and caller frame | BIOS tick | RAM SHA-256 | RGB SHA-256 |
| ---: | --- | ---: | --- | --- |
| 1 | `0824:0122`, far call `0840:0025`; `SS:SP=1D16:3EFE` | 547594 | `baae0e81bd3612bbe3b1ff8b0a1f961fdcbd9255e51a18b2cfdf682b6d0e8efb` | credits `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` |
| 2 | `0840:0025`, far jump `38D3:00FD`; stack top `0127,0824` at `1D16:3EFA` | 547594 | `6103bd4c22a74758767df16c8547883ea8b8e02240980d4d470da82969f50b0c` | credits (same hash) |
| 3 | `38D3:00FD`, unit `0x19` entry prologue; same return frame | 547594 | `6103bd4c22a74758767df16c8547883ea8b8e02240980d4d470da82969f50b0c` | credits (same hash) |
| 4 | `0824:0127`, unit `0x19` returned; far call `089C:0025`; `SS:SP=1D16:3EFE` | 547598 | `2a7e419ac72e926eccc014d8dc9a3ef583c054afd5b1aaec317b5598d0186faf` | code-word prompt `6f215fa03036c3323b8e963a2d35b30c65fb768177d79fecc33c585fbdbf608d` |
| 5 | `089C:0025`, unresolved `INT 3Fh` stub; stack top `012C,0824` at `1D16:3EFA` | 547598 | `75236a1e0d1d8d1399982d83ca3112c1f6aebc97cf5723996f8ba5d603837c33` | prompt (same hash) |
| 6 | `089C:0025`, loader-patched far jump `2648:0161`; same return frame | 547599 | `1389c6bd6d9e2659c13c4e783b68b80c7a51bca1c612173ed76e8e28d238b649` | intermediate `d5d5758a44c945f5ee6e5b9551a4bcc165d759c96cada4f654ced731179cadbe` |
| 7 | `2648:0161`, unit `0x6C` entry prologue; same return frame | 547599 | `1389c6bd6d9e2659c13c4e783b68b80c7a51bca1c612173ed76e8e28d238b649` | intermediate (same hash) |
| 8 | paused menu loop `2116:0539` | 547650 | `8cdc011715c8b74e2ec4f22cbd3c3256b8dac7d8739e33f2df80b0f7f4245838` | menu `ac9ba01824990b222f43278faa287185f7b65406f525a47a44696b29645c7449` |

At stop 2, the five-byte live stub hash is
`bf0c36dda99d8b3d11ab4b874d52757a90e1186a03e3fc7271d704c26634a052`.
At stop 5, the unresolved five-byte stub hash is
`2241a958bf39f591837c97fd0e48d0c5700a12376250ff192db3299ac845aa5b`;
after the overlay loader patched it, the hash became
`1bac302142282626974e2bd5469a784b58453d05474a419230bbcf5c8f374f4a`.
The halted prompt frame differs from the survey's steady prompt RGB hash;
the private image inspection confirms the same prompt class, while this note
does not attribute the pixel differences to a specific cursor or draw step.

The paired sequence proves that unit `0x19` is reached between the credits
return and visible prompt, returns to the resident before the menu call, and
that unit `0x6C` is reached before the menu. It does not show the internal
prompt-input branch, prove which routine first drew each prompt pixel, or
establish what happens for a different code word or earlier key timing.

## Reproduction

Prepare a fresh ignored copy with `tools/oracle/prepare_run.sh`, launch the
hash-pinned heavy DOSBox-X with `DEBUGBOX START` and no game arguments as in
the [picture route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md), and arm
`BP 0824:0122` plus `BP 0824:0127` at the MZ entry. Send one Return only when
the credits RGB hash is visible. At `+0x0122`, arm `BP 0840:0025` and then
`BP 38D3:00FD`; at `+0x0127`, arm `BP 089C:0025`, which hits once before and
once after `INT 3Fh` resolves the stub, then `BP 2648:0161`. At each stop,
use `EV CS IP DS ES SS SP BP AX BX CX DX FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; move each dump before resuming and take the
paired Xvfb screenshot. After the menu appears, pause with Alt+Pause and
capture one more pair. Runtime load segments may vary in another run; resolve
the stub jump target instead of assuming the same loaded unit segment.

All screenshots, RAM dumps, game files, debugger output and captures remain
ignored under `resources/`. No game bytes, screen text, save, or media are
published.
