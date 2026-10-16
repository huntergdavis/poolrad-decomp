# U19 row-14 selected-store route join

This offline gate joins the merged
[eight-stop launch template](U19_ROW14_SELECTED_STOP_ROUTE.md) to the existing
[same-run CPU/four-plane event audit](U19_ROW14_SAME_RUN_EVENT_PLAN.md).
It adds a required U19 `+02D8` pre-call and `+02DD` post-return stop around
the selected printer store. No new native capture exists; no first writer,
page selection, RGB timing, full-frame, C, or identity claim follows.

Before implementation, `deja "Poolrad U19 row14 selected 02D8 02DD route
stop join CPU EGA event plan"` returned no indexed matching session. The
merged selected-stop route supplied the resident `+0122` → stub `+0025`
→ U19 `+00FD` mapping, and the merged event plan supplied the 128-hit
candidate cap, four-plane pair, and controller restoration check. This
join reuses those audits without repeating their native or plane logic.

## Private supplied record

The route JSON lives with the CPU frames and plane manifest in one ignored
run directory. All three documents carry the same 1–64-character run token.
The route has `version: 1`, `segments` with hexadecimal WORD strings
`resident`, `u19`, and `printer`, and eight `ordered_stops` entries in the
public template's ID order. Each entry has a zero-based `frame_index` into
the existing CPU frame stream. It names distinct `pre_ram` and `post_ram`
640-KiB captures and their lowercase SHA-256 digests. The plane manifest
already names separate immediate store-before/store-after RAM and four
planes per sample.

The checker requires strictly increasing stop indices, the CPU filter's
first pre-call and first post-return, the selected store's successor as the
immediately following CPU frame, and the first effective
`ES:DI=A000:118F` drawer hit selected by the existing target filter. It
rejects a printer return before that hit and requires a later U19 `+02DD`
stop. The supplied pre-call RAM must encode a far call from the measured
resident `+0122` to its `+0x1C` stub, a far jump from stub `+0025` to the
measured U19 `+00FD`, and a far call from U19 `+02D8` to the measured
printer `+0626`. This relates the supplied segment labels to the memory
image. The merged CPU target filter still pins U19 `38D3` and printer
`0F2D`; a differently relocated run is a typed blocker until that filter
is generalized. The resident segment is read from the supplied route.

For a separately authorized private capture, run:

```sh
python3 -m tools.replay.cold_start_codeword_row14_route_join \
  --run resources/oracle-runs/PRIVATE-RUN \
  --template docs/re/U19_ROW14_SELECTED_STOP_TEMPLATE.json \
  --route resources/oracle-runs/PRIVATE-RUN/route.json \
  --frames resources/oracle-runs/PRIVATE-RUN/cpu-frames.json \
  --manifest resources/oracle-runs/PRIVATE-RUN/plane-events.json
```

The tool verifies consistency of supplied files, not their provenance or
debugger breakpoint completeness. A shared token and RAM hashes cannot
authenticate that frames came from one DOS invocation. The selected
instruction may leave all four planes unchanged; observed plane-byte
and `0x20` bit deltas are reported by the underlying event audit only
after a real pair is captured. Keep all CPU/RAM, EGA and RGB records under
ignored `resources/`.

Synthetic gate:

```sh
python3 -m unittest tests.replay.test_cold_start_codeword_row14_route_join
```
