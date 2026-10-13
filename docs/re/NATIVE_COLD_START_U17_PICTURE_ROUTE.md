# Native no-argument startup: resident picture call and return

One DOS v1.3 cold start reached resident image `+0x011D`, called overlay unit
`0x17` entry 1, and returned at image `+0x0122`. This is a **native route
observation**, not a C translation, a game-function count, or a claim that all
startup screens were sampled. The trace ended at the saved-game chooser without
selecting a save.

Before debugging, `deja "startup picture"` recalled the earlier static gate
analysis in [RESIDENT_PALETTE_2A0D_GATE.md](RESIDENT_PALETTE_2A0D_GATE.md).
The [resident loader map](RESIDENT_LOADER_MAP.md) supplied the MZ entry and
expansion mapping; the [oracle procedure](../oracle/README.md) supplied the
hash-verified private-copy rule. This run independently checked the live
call, stub, loaded entry, return frame, and palette breakpoint.

## Pinned private inputs

| Input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| full copied-input hash manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private heavy DOSBox-X executable, version 2026.01.02 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The manifest verified after the run. `DEBUGBOX START` supplied no program
arguments; the live PSP command-tail length at `0814:0080` was **0**.
The first `-debug` attempt only enabled verbose logging and supplied no
startup stop. All measurements below come from the fresh `DEBUGBOX START`
run `native-cold-start-picture-002`, retained only under ignored `resources/`.

## Execute route and bounded code identity

The debugger stopped at MZ entry `1796:0012`. Subtracting MZ entry paragraph
`0x0F72` gives the runtime load base `0x0824`. The original expanded resident
gate at image `+0x00FD..+0x0126` is 42 bytes, SHA-256
`962f5862d51183a237def6639d88fc51f68d3a71f5310554ddd9553ce54c83fe`.
The five-byte original call at image `+0x011D..+0x0121` hashes
`08c75e3d9d8d592c2ffb668dbca1bc96bce437d06b429d1ed30756084cebfdb4`;
its live relocated five bytes hash
`07ff87e4d174e70ee8d40c8a453bb5f873d6f32b01537f14c417680ad0d328a1`.

| Stop order | Guest CS:IP | Observed instruction or frame | `SS:SP` |
| ---: | --- | --- | --- |
| 1 | `0824:011D` | far call to `083D:0025` | `1D16:3EFE` |
| 2 | `083D:0025` | far jump to `3909:0627` | `1D16:3EFA` |
| 3 | `3909:0627` | unit `0x17` entry-1 prologue; top stack words `0122,0824` | `1D16:3EFA` |
| 4 | `0824:0122` | return to the next resident far call | `1D16:3EFE` |

The packed unit descriptor begins at image `+0x0178`; its entry-1 source is
`GAME.OVR` file `0x06FB..0x08DC` (482 bytes), SHA-256
`02adf3ddfc09a9cf558fe9735f53e78858bb4ec57471672d33be6082677f137d`.
At `3909:0627`, the loaded body hash is
`5205b089e2f8042ae894e958c60c4d515f9c6a9ab44a6e2b1bdef44e4406f4bf`.
Exactly 54 bytes differ, all within its 27 descriptor-listed relocation
words. Restoring those words to their original values reproduces the original
482-byte hash with **zero** other differences.

The live call at unit `+0x076B` targets palette setter `0C88:2A0D`.
`BP 0C88:2A0D` was armed before continuing from MZ entry and remained in the
debugger's breakpoint list at `0824:0122`; it recorded **zero hits** from
`+0x011D` through `+0x0122`. The video-mode gate byte `DS:[4A1E]` was 1 at
both endpoints. This run does not identify why the palette-call path was
skipped, nor establish whether it runs with different timing or input.

This live no-argument call contradicts the older default-path wording in
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), which classified image `+0x011D/+0x0122`
as off the default path. The earlier palette-gate note described its static
condition but left execution open; the stops above resolve reachability for
this run.

## Ordered state samples

BIOS ticks are the little-endian dword at physical `0x046C` in each private
640-KiB RAM dump. The 452-tick interval between the call and return includes
one Return key sent while the credits splash was visible. The later `L` key was
sent only after the main-menu sample. Pauses and debugger inspection make host
wall time unsuitable for guest-duration claims.

| Observed point | BIOS tick | Private RAM SHA-256 | Canonical 640×400 RGB SHA-256 |
| --- | ---: | --- | --- |
| before image `+0x011D` call | 1,374,442 | `3de44fb40c3490f5e540d7ec1ffff7e764326eb6badcfc1638144ac08bfb45d6` | `351a8054b23144894668c6a51d5dbba82f56523ef43cc45b6f4d7060a969c31a` |
| loaded unit `+0x0627` entry | 1,374,442 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` | — |
| credits splash during unit call | between 1,374,442 and 1,374,894 | — | `15e405dac036c9530d5f87ccb67c9427012e6860195ada76b56193ed03c557dc` |
| image `+0x0122` return, credits frame | 1,374,894 | `6e0ae646dc59e13992f760e0a899c7482d62b62b375f7d1f2934e514f4a4a3fe` | `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` |
| main menu, paused at `2116:0566` | 1,375,067 | `4e60dcc86ee706f332f285c08d6672d6524f8e9bd50c113f09df81f7016b73da` | `ac9ba01824990b222f43278faa287185f7b65406f525a47a44696b29645c7449` |
| saved-game chooser, paused at `2116:06D3` | 1,375,209 | `1f6edef9cfe80c674566c946195db97c436568550fff56f8c48e46adadd487de` | `341eb227760736f978d4bddcdc82e94abc6d26505e9b04ed661c2e83c190931f` |

Each RGB hash is `sha256(Image.open(private_png).convert("RGB").tobytes())`.
The sampled frame changed by 140,268 pixels from the pre-call frame to the
credits splash, 161,160 from that splash to the return frame, 63,564 from the
return frame to the menu, and 1,908 from the menu to the chooser. These are
screen-state differences across observations, not attribution to one callee.

The code-word prompt, a distinct loading screen, and a separate title frame
were **not captured** in this run. The sampling gap between `+0x0122` and the
menu, and the uncaptured period before `+0x011D`, prevent a claim that those
screens were skipped or reached. No music or
sound cue was independently observed; audio is **unknown**. No saved game was
loaded, no C-native parity was tested, and no routine count changes.

## Reproduction boundary

Use `tools/oracle/prepare_run.sh` to make a new ignored private copy. The
private heavy build used here is expected at the path below and must match
the hash above. Launch with `DEBUGBOX START` (not `-debug`):

```sh
oracle_run_dir=$(tools/oracle/prepare_run.sh native-cold-start-picture-NEW)
mkdir -p "$oracle_run_dir/.config" "$oracle_run_dir/.cache" \
  "$oracle_run_dir/.local/share" "$oracle_run_dir/.dosbox-saves"
export XDG_CONFIG_HOME="$oracle_run_dir/.config"
export XDG_CACHE_HOME="$oracle_run_dir/.cache"
export XDG_DATA_HOME="$oracle_run_dir/.local/share"
export SDL_AUDIODRIVER=dummy
xvfb-run -a -s '-screen 0 1280x1024x24' \
  bwrap --unshare-net --ro-bind / / --dev-bind /dev /dev \
  --bind "$oracle_run_dir" "$oracle_run_dir" --chdir "$oracle_run_dir" \
  "$PWD/resources/dosbox-x-heavy-src/src/dosbox-x" \
  -defaultconf -defaultmapper -nogui -fastlaunch \
  -savedir "$oracle_run_dir/.dosbox-saves" -time-limit 600 \
  -c "mount c $oracle_run_dir" -c 'c:' -c 'cd \POOLRAD' \
  -c 'DEBUGBOX START'
```

At the MZ entry, issue:

```text
BP 0824:011D
BP 0824:0122
BP 0C88:2A0D
RUN
```

At `+0x011D`, add `BP 083D:0025`; there add `BP 3909:0627`. At each stop,
record `EV CS IP DS ES SS SP BP AX BX CX DX FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`, moving each dump before the next. In the Xvfb
emulator window, send one Return key while the credits splash is visible.
After the `+0x0122` stop, resume; pause with Alt+Pause at the main menu,
sample it, resume, send `L`, and pause at the chooser. Keep the screenshots,
RAM images, emulator trace, media, and any audio under ignored `resources/`.
This public note contains no original image, screen text, executable bytes,
or audio.
