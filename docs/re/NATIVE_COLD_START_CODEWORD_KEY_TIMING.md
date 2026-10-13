# Native cold-start Return timing at the code-word prompt

Two fresh DOS v1.3 no-argument cold starts used identical copied inputs,
emulator build, and resident breakpoints. In run A, one Return was sent while
the credits frame was visible. In run B, no key was sent at credits; one Return
was sent only after the code-word input prompt became visible. The comparison
locates the input-consumption boundary inside overlay unit `0x19`, between
resident `0824:0122` and `0824:0127`. It does not establish code-word
validation semantics or C-native parity.

Before debugging, `deja "Pool of Radiance startup credits Return code word
prompt keyboard queue"` found no focused session; `deja "code word"` recalled
older blank-submit observations but no settled queue interpretation. The
[native call route](NATIVE_COLD_START_CODEWORD_MENU_ROUTE.md) supplies the
independently checked resident call/stub hashes and unit identity. The
[screen survey](NATIVE_COLD_START_SCREEN_GAPS.md) supplies the reference
credits and menu RGB hashes. This experiment independently measured keyboard
queue state at the live route boundaries.

## Pinned private inputs

| Input or tool | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| each copied-input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private DOSBox-X 2026.01.02 heavy executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The ignored private copies are `native-codeword-credit-key-002` and
`native-codeword-prompt-key-003`. Both copied-input manifests passed again
after the runs. `DEBUGBOX START` supplied no game argument. The shared
breakpoints were `BP 0824:0122` and `BP 0824:0127`, armed at the MZ entry.
Both credits screens matched canonical RGB SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`
before the key intervention. The runtime call at `+0x0122` enters unit
`0x19` via `0840:0025`; the next call at `+0x0127` enters the unit `0x6C`
menu path via `089C:0025` as proved in the linked route note.

## Same-stop keyboard and screen measurements

The BIOS keyboard queue head and tail are the words at physical `0x041A`
and `0x041C`. Queue slot `0x001E` is physical `0x041E`. At each stopped
point, `MEMDUMPBIN 0000 0000 A0000` produced a private 640-KiB image; BIOS
tick is the dword at physical `0x046C` from that same image. RGB hashes are
SHA-256 over a paired private 640×400 screenshot converted to RGB. These
addresses and numeric values are input-state evidence, not original game
bytes.

| Run and stop | Guest CS:IP | BIOS tick | Queue head/tail; slot `0x1E` | RAM SHA-256 | Paired RGB SHA-256 |
| --- | --- | ---: | --- | --- | --- |
| A, credits Return already sent; before unit `0x19` | `0824:0122` | 559000 | `0x1E/0x20`; `0x1C0D` queued | `a461cf203f804a50b3021c0635054ce4a2a33690f4dfc6b882f087aa9c75229f` | credits `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` |
| A, unit `0x19` returned without another key | `0824:0127` | 559004 | `0x20/0x20`; old slot still `0x1C0D` | `f4ebdedffe4dd9f9c4771d2ba8e0f3a202df5f62c34ebf5bca886d8bc6054a24` | code-word prompt draw state `d5d5758a44c945f5ee6e5b9551a4bcc165d759c96cada4f654ced731179cadbe` |
| B, no key sent; before unit `0x19` | `0824:0122` | 561845 | `0x1E/0x1E`; slot `0` | `f017c476fe757f0886b6d6f4d86c51fda05b7c6631bbff061b2b6441df4582ff` | credits (same hash) |
| B, visible input prompt before any key; paused in BIOS | `F000:CFCC` | 562816 | `0x1E/0x1E`; slot `0` | `4a56150f1e0d5133ec9cd90211e3e692d92a0c52ba00116ae508543a4ee98de3` | prompt input visible `171811b442b4984ce28e9555a8fa804127f3264db60eb08ed910600175f2920f` |
| B, one Return sent at visible prompt; unit `0x19` returned | `0824:0127` | 563016 | `0x20/0x20`; old slot `0x1C0D` | `58bef5d6dbcf0284e85b73f157ad2ab4d6d2eb48e395d470a36d01c367c23b45` | code-word prompt draw state `ca83c613387ddcf067455166015750c8d17b4f3aee070a36130393bbc050fa9c` |
| B, main menu after `+0x0127` call | `2116:06CF` | 563282 | `0x20/0x20`; old slot `0x1C0D` | `f6b4919155bf275b209d7fb289d4bc476c4dda26f5fe77e00801ac43c086600f` | menu `ac9ba01824990b222f43278faa287185f7b65406f525a47a44696b29645c7449` |

Run A reached the same canonical menu RGB hash after `+0x0127`. In both
runs, `0x1C0D` is the one Return's BIOS scan/ASCII word. The slot retains
the old word after consumption; the head/tail equality, not a cleared slot,
marks the queue empty. Absolute BIOS ticks differ between cold starts and
include debugger pauses; they are stop identifiers, not matched wall-time
measurements.

The observations show that a Return sent during credits can remain queued
when credits return, and that the only code between the `+0x0122` and
`+0x0127` stops removes it. Without that queued key, the unit reaches a
visible code-word input prompt and remains there with an empty queue until
the later Return. Both timings then reach the menu. This strongly supports
the credits Return being consumed on the prompt-input route. The exact
keyboard-read instruction, the internal branch taken after reading, and
whether any code word was validated remain untraced. The varied prompt RGB
hashes are paused/rendered draw states, not separate game screens.

## Reproduction boundary

Prepare two fresh ignored copies with `tools/oracle/prepare_run.sh`. Launch
the pinned heavy DOSBox-X under Xvfb with `DEBUGBOX START` and no game
argument, using the [native route procedure](NATIVE_COLD_START_CODEWORD_MENU_ROUTE.md)
for the private wrapper. At MZ entry set `BP 0824:0122` and
`BP 0824:0127`; then `RUN`. For A, wait for the pinned credits RGB hash,
send exactly one Return with `xdotool`, capture both resident stops using
`EV CS IP DS ES SS SP BP AX FLAGS` plus `MEMDUMPBIN 0000 0000 A0000`,
and resume to the menu. For B, send no key at credits; capture the empty
queue at `+0x0122`, resume, wait for the visible code-word input prompt,
pause with Alt+Pause, capture the pre-key guest PC and queue, resume, send
one Return, and capture `+0x0127` and the menu. Move each dump before the
next dump command and hash each paired private screenshot's RGB bytes.

All game files, screenshots, RAM images, debugger output, and captured media
remain ignored under `resources/`. This note publishes addresses, hashes,
counts, and numeric input-state observations only. No original game screen
is published or presented as C output.
