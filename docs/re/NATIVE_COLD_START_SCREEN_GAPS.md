# Native cold-start screen gaps: bounded continuous sample

This is a DOS v1.3 **native observation** on the no-argument startup path. It
does not translate a routine, add a game-function count, or establish C-native
parity. All video, screenshots, WAV data, RAM, and original game files remain in
ignored `resources/`.

Before debugging, `deja "cold start"` recalled the earlier session for
[the unit `0x17` startup route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md). That
note's verified MZ load-base and live call/return stops provided the address
anchors below. The present run independently stopped at MZ entry and, in a
separate interrupted capture, at resident `0824:011D`.

## Pinned inputs and capture boundary

| Private input/tool | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| copied-input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| private heavy DOSBox-X 2026.01.02 executable | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The final continuous run was an ignored fresh copy named
`native-cold-start-gap-004`. `DX-CAPTURE /A DEBUGBOX START` entered the debugger
at `1796:0012`; the no-argument `START` command was then released with `RUN`.
No save was selected. One Return was sent while credits were visible, followed
by the menu's exit key and its Yes confirmation. The game returned to the DOS
shell. A private `640×400` X window at `(320,312)` was sampled at 15 fps from
before `RUN` through the DOS return. The resulting 1,830-frame, 122.000-second
FFV1 file hashes `6e0e2ba53d71820b482c4abdeae9ecc55d4bd5394e0cbf4ef999e4e4527af7e4`.

The separate corrected gate run `native-cold-start-gap-003` stopped at
`0824:011D`, the resident far call to `083D:0025` established in the earlier
note. Its private RAM dump hashes
`42cea3847502069dba98cd1993a97e56c50bee7003f7e52ce9f50b3196ab8bc2`
and contains BIOS tick **510,470**. Its full-window capture directly showed a
loading message at this stop, RGB SHA-256
`be0ff6f19aa525049a12a3794c3fe4475ce7549030ed6d5e8f7d7cb41635100f`.
That run's X display ended before the later sequence; all later screen order
below comes from the uninterrupted final run. The final run had no breakpoint
at `+0x011D`, so the video timestamps are **not** guest instruction timestamps.

## Continuous visual order

Each digest is SHA-256 of the decoded frame's `RGB24` pixel bytes. Rows are
stable frame groups. Short transition frames between them were retained in the
private capture and are not classified as separate screens. Times are video
times (`frame / 15`), not guest BIOS ticks or native DOS durations.

| Frames | First video second | Directly observed state | RGB SHA-256 |
| ---: | ---: | --- | --- |
| 0–126 | 0.000 | blank initial display | `351a8054b23144894668c6a51d5dbba82f56523ef43cc45b6f4d7060a969c31a` |
| 133–215 | 8.867 | loading message | `be0ff6f19aa525049a12a3794c3fe4475ce7549030ed6d5e8f7d7cb41635100f` |
| 216–242 | 14.400 | blank transition | `351a8054b23144894668c6a51d5dbba82f56523ef43cc45b6f4d7060a969c31a` |
| 248–352 | 16.533 | publisher splash | `15e405dac036c9530d5f87ccb67c9427012e6860195ada76b56193ed03c557dc` |
| 359–433 | 23.933 | distinct game title frame | `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8` |
| 443–595 | 29.533 | credits screen | `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` |
| 599–838 | 39.933 | code-word prompt | `a9dc46ae3435d144cb26bb6afdd47e729de7719ce325bcc64e56fedca428aed5` |
| 841–1236 | 56.067 | main menu | `ac9ba01824990b222f43278faa287185f7b65406f525a47a44696b29645c7449` |
| 1237–1745 | 82.467 | exit confirmation | `952021ae915bbb816e6c5dd3c0edc9c1dd6589ed9122a17fc0431d334b6ca780` |

The code-word prompt persisted for **240 sampled frames** (16.000 video
seconds) between credits and the menu. The capture proves its display on this
route; it does not prove its input validation or why the menu followed. No
screen in this table is attributed to a specific unit instruction beyond the
separately observed `+0x011D` gate.

The earlier startup note called digest `15e405…` a credits splash. The full
window sample identifies it as the **publisher splash**; digest `624f69…` is
the credits screen. This corrects the screen label, not the earlier route,
loader, call frame, or bytes.

## Audio observation and limits

The private `DX-CAPTURE /A` mixer WAV finalized when the game exited. It is
48 kHz, stereo, 16-bit PCM, 109.281 seconds, SHA-256
`e2035d08de096f55eb337ff032154d92989c4cfe2419d4f496eea5d193c3478c`.
All **10,490,976 decoded samples** are zero (peak 0, RMS 0). The SDL output
driver was `dummy`. This measures silence in this emulator mixer capture; it
does not establish whether original hardware or another configuration plays a
cue. No audio cue was independently heard.

The display sampling interval is 1/15 second, so a shorter distinct frame may
have fallen between samples. No save, gameplay loop, or C-native path was
reached. No routine count changes.

## Private reproduction

Use `tools/oracle/prepare_run.sh` to make a new ignored copy and verify its
manifest. Launch the pinned heavy build in a private 1280×1024 X display with
the same isolated `bwrap`/XDG setup as the earlier route note, replacing the
last command with `-c 'DX-CAPTURE /A DEBUGBOX START'`. At the entry debugger
stop, start a full-window FFV1 capture with:

```sh
ffmpeg -nostdin -f x11grab -draw_mouse 0 -framerate 15 \
  -video_size 640x400 -i :DISPLAY+320,312 -c:v ffv1 \
  resources/oracle-runs/NEW-RUN/continuous-15fps.mkv
```

Replace `DISPLAY` and `NEW-RUN` with the private display number and ignored
run name. Issue `RUN` at `1796:0012`, send one Return when credits appear, and
select Exit to DOS followed by Yes at the menu. Decode the video to `RGB24`
and group consecutive frames by SHA-256 to reproduce the table. Keep all
captured media and decoded frames under ignored `resources/`.
