# Saved slot-A U3BD entry-5 first call transfer

The reached `2D14:+07E9` continuation of the [loop reset](SLOT_A_U3BD_LOOP_RESET_07C7.md)
loads three caller-frame bytes into `AL`, pushing each full `AX`. It then
pushes `CS` and near-calls merged U3BD entry 2 at `+047B`. This note and
`pr_u3bd_first_call_07e9` stop **before** executing the callee. They do not
establish the later `+080A` branch or a complete entry-5 translation.

Before debugging, `deja "Poolrad saved slot A U3BD entry5 +07E9 +047B first call successor"`
returned no matching session within its bounded search. The merged
[U3BD +07C7 loop-reset note](SLOT_A_U3BD_LOOP_RESET_07C7.md) supplied the
reached caller frame and adjacent boundary; this audit independently
rechecked original bytes and captured a new same-run native pair.

## Original and loaded boundary

| Item | Evidence |
| --- | --- |
| Original `GAME.OVR` SHA-256 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U3BD file span | `[0x2F0E0, 0x2F0F0)`, 16 bytes, SHA-256 `741850baf96f032d50cb9805332c95b3d41c0a95337a3372e755f608cd145a38` |
| Loaded span | `2D14:+07E9..+07F9`, identical to original file span in the before RAM image |
| Instructions | Eight: three `MOV AL`/`PUSH AX` pairs, `PUSH CS`, near `CALL +047B` at `+07F6` |
| Next transfer | `2D14:+047B`, with return IP `+07F9` on the stack |

The private source, save, screenshots, and 640 KiB RAM captures are under
ignored `resources/`. The run was prepared with `tools/oracle/prepare_run.sh`
as `slot-a-u3bd-successor-07e9-p5-026`, launched in isolated DOSBox-X with
`DEBUGBOX START`, and loaded slot A. A sole breakpoint at `2D14:07E9`
captured the before frame. It was replaced by a breakpoint at `2D14:047B`
for the immediate after frame. The debugger binary SHA-256 is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Native state | At `+07E9` | At `+047B` |
| --- | --- | --- |
| RAM SHA-256 | `58a58dae33cf4cd30340c17c997659b7c5eb5cb382a038af3d2dcd254b1f43fc` | `6c0df4179b35cea782d1fe87db44e645f022e926a5306d44d630b9014ee05c80` |
| `AX`, `SP`, `FLAGS` | `0000`, `3E9E`, `7297` | `0006`, `3E94`, `7297` |
| Other CPU fields | `BX=0 CX=4 DX=000D SI=3E68 DI=4 BP=3EB6 DS=14A0 ES=3DA1 SS=1D16 CS=2D14` | unchanged |

The three input bytes at `SS:[BP-0D]`, `SS:[BP-0F]`, and `SS:[BP+06]`
are `0D`, `04`, and `06`. The three pushed words are therefore `000D`,
`0004`, and `0006`; `PUSH CS` writes `2D14`, and the near call writes
return IP `07F9`. Only seven physical RAM bytes changed, at
`0x20FF4..0x20FFA`. Same-value writes inside this stack interval do not
appear in a before/after byte diff.

## Reproduce and scope

`tools/replay/u3bd_first_call_07e9_audit.py` hash-locks the original span,
loaded bytes, and both private RAM captures. It decodes eight instructions,
compiles the C slice with strict C99 warnings, and compares all 640 KiB RAM,
13 CPU fields, and IP against the same-run native stop. Synthetic CTest
cases cover preserved `AH` and flags, each stack word, 16-bit stack offset
wrap, and invalid host inputs.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/replay/u3bd_first_call_07e9_audit.py
cmake -S . -B resources/build-u3bd-first-call-07e9 -DCMAKE_C_FLAGS='-Wall -Wextra -Werror -pedantic'
cmake --build resources/build-u3bd-first-call-07e9 --target poolrad_overlay_u3bd_first_call_07e9_tests
ctest --test-dir resources/build-u3bd-first-call-07e9 -R '^poolrad_overlay_u3bd_first_call_07e9_tests$' --output-on-failure
```

The next unresolved caller boundary is after the +047B callee and the
subsequent +031F call, before the `+080A` conditional. This bounded transfer
alone does not prove which post-call branch the saved route takes.
