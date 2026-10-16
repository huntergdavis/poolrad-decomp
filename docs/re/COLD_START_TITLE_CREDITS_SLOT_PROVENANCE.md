# Saved title-to-credits stable-slot provenance

This read-only check pins the existing native cold-start capture's title and
credits stable-slot references and the sampled gap between them. It does not
run the game, decode a new video, create a screenshot, establish an instruction
transition or natural DOS timing, or add C-to-native frame parity. The merged
[synthetic two-frame fixture](COLD_START_TITLE_CREDITS_TRANSITION_FIXTURE.md)
intentionally has a title frame and a credits frame with no intervening
samples; this note records why that fixture boundary must not be read as a
native adjacent-frame boundary.

Before the check, bounded `deja "Poolrad cold start title credits stable
slots 359 433 443 595 CFR frame groups provenance MD5 transition"` failed
because `index.db.lock` was read-only. This check reuses the saved capture
identity and CFR/passthrough distinction in
[native cold-start screen gaps](NATIVE_COLD_START_SCREEN_GAPS.md), plus the
existing [title PNG](COLD_START_TITLE_ASSET_PROVENANCE.md) and
[credits PNG](COLD_START_CREDITS_SYNTHETIC_RGB.md) hashes. PM cleared the
private-artifact checker scope against p9's U17 printer-source ownership
before edits. All private artifacts remain under ignored `resources/`.

## Pinned saved artifacts

The [provenance checker](../../tools/replay/cold_start_title_credits_slot_provenance.py)
reads only these files from the already captured
`native-cold-start-gap-004` run:

| Saved file | Bytes | SHA-256 |
| --- | ---: | --- |
| `frame-groups.json` | 12,580 | `e3c2c3532f2ea068905b173860d701afef71f48fd1a2e561cd1b012ef695ef33` |
| `frame-md5-cfr.txt` | 146,614 | `30c7d30caa8c7e6f8a33e9196c65c18777098a2cdb39b3cac953139b6c8f80d7` |
| `frame-md5-passthrough.txt` | 146,214 | `ef6db3cf4616fe6c57ce0cbbe22e7cab082a74ec77a54714fec69c58da837dff` |
| `stable-frames/0359-0433.png` | 23,906 | `cea3d2c06f84c4c2dffaae493442649fe00bbe2bdb8e49324d46a883fda0bb99` |
| `stable-frames/0443-0595.png` | 11,051 | `5d434e35fdfcbdc62970ed61d365763497be64234ed98c491b3dcb72cd33f295` |

The manifest contains 89 groups covering exactly CFR slots 0–1829 with no
gap or overlap. Each saved PNG is 640×400 RGB and decodes to 768,000 RGB24
bytes. The check compares its decoded SHA-256 with the matching manifest
group and its RGB MD5 with every CFR record in that group. The MD5 record
match aligns existing sample records; the pinned SHA-256 checks supply file
and decoded-frame identity.

| Stable reference | CFR sample slots | Group count | Decoded RGB24 SHA-256 |
| --- | ---: | ---: | --- |
| Game title | 359–433 | 75 | `3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8` |
| Credits | 443–595 | 153 | `624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698` |

Slots 434–442 are **nine distinct singleton groups**. Each has its own
manifest RGB SHA-256 and distinct CFR RGB MD5; none equals the title or
credits stable reference. This proves only that the saved CFR export includes
intervening sampled images. It does not identify a guest instruction for
any sample or establish that every native intermediate image was sampled.

The CFR export has **1,830** records and the passthrough export **1,825**.
All 1,825 shared PTS records have identical RGB MD5 and 768,000-byte frame
size. CFR slots **279, 280, 593, 962, 1582** are absent from passthrough;
each CFR record there duplicates its immediately preceding RGB MD5.
Slot 593 lies inside the 443–595 credits stable group, so that group has
153 CFR sample slots and 152 encoded passthrough frames. These are video
sample indices under a 1/15 time base, not guest clock ticks or natural
on-screen durations. This checker does not rehash or decode the original
continuous video; its existing capture identity is documented in the
native screen-gap note.

## Focused reproduction

Run from a clone that can read the ignored existing capture:

```sh
python3 -B tools/replay/cold_start_title_credits_slot_provenance.py \
  --run resources/oracle-runs/native-cold-start-gap-004
```

The focused read-only check passed with `groups=89`, `cfr_slots=1830`,
`passthrough_frames=1825`, `shared_pts_match=1825`,
`gap_singletons=9`, and `duplicate_previous=5`. It uses Pillow to decode
only the two saved stable PNGs. It neither creates new media nor changes the
existing C renderers or any full-C game-function count.
