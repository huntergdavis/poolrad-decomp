# Saved slot-A post-Up outer UI: captured first printer page

**Classification:** bounded capture-order audit and one existing C printer
model page check. The link from outer `+0DAE` to the printer capture is
**unobserved**. This is not a native successor stop, a complete post-Up UI
route, or a full-C parity/count claim.

Before this audit, `deja "Poolrad post-Up outer UI +0DAE next printer page
captures order"` timed out under the host-load hold. Reused the
[outer-return preparation](SLOT_A_POST_UP_0DAE_OUTER_RETURN_PREP.md),
[direct post-EXIT caller trace](SLOT_A_POST_EXIT_NATIVE_RETURN.md), and the
merged [printer page model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md). No new DOS
session was started.

## Pinned ignored inputs and capture identities

| Input | SHA-256 |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Saved slot A | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `2D14:0DAE` native RAM, 655,360 bytes | `b8d808d1469791646d57003438b094f0d9fee0459442021c3747a5635025bc8c` |
| First-printer entry RAM, 655,360 bytes | `5c3aa481884391318a14a9cfe4e76b1446027b72b368065df606c2a37c5c0c23` |
| First-printer return RAM, 655,360 bytes | `24b7c6335e01ec918c823935a52342278ddb81c3150e06d67d07b10dcc63d1e8` |
| Private first-printer C probe input, 33,675 bytes | `53fd15433e954797e4b2ef165d7eac15c0accea3db040d956bfe9dedf2042318` |

The outer stop is from ignored run `slot-a-post-up-exit-p9-119`; the paired
printer snapshots are from a **different** ignored run,
`slot-a-post-up-first-ui-writer-p9-111`. A second independent run,
`slot-a-post-up-next-ui-p9-112`, has the same first-printer entry and return
page hashes, though its full-RAM hashes differ. Neither capture includes a
same-run stop at the predicted outer successor.

## Bounded order table

| Order / boundary | Status | EGA page evidence |
| --- | --- | --- |
| `2D14:0DAE` outer epilogue input | Native PC and RAM measured | No EGA plane capture at this stop; page unknown. |
| `2E35:022E` | Predicted from the measured `SS:3EE0..3EE4` stack words and original `RETF 6`; no native destination stop | Unknown. |
| `2E35:029C` | Static `+022E` jump target, then a static far epilogue; not a measured post-Up PC | Unknown. |
| `3582:3497` | Predicted by the same outer stack; independently observed on another exactly-one-Up trace after a view call, **not joined to this `+0DAE` run** | No page capture at the independent stop; unknown. |
| First-printer entry, private `p9-111` capture | Paired native page/RAM image; exact caller and intervening branch path not logged by these images | 32,000-byte page SHA-256 `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44`. |
| First-printer return, same private capture | Paired image after the call | 32,000-byte page SHA-256 `a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac`. |

No observation here identifies the transition between `+0DAE` and the
printer entry or attributes any intermediate page write. The distinct
[first-view title printer](SLOT_A_VIEW4_TITLE_PAGE.md) is a four-cell row-2
call; it must not be substituted for this 19-cell row-15 capture.

The private probe metadata is length 19, column 17, row 15, color 10.
Its embedded entry page equals the four native entry planes. Across the
printer pair, page value changes by plane are **0, 105, 0, 105**; plane-1
and plane-3 changed offsets lie within `4817..5099` of their 8,000-byte
planes. The existing `pr_slot_a_printer_page_apply` model reports 19 cells,
608 stores, and 210 changed values. Its complete 32,000-byte output equals
the native return page. This is page-output parity for that paired printer
boundary only; RAM, guest PC, caller stack, and the route from `+0DAE` are
outside the C fixture.

## Reproduction and next source-only audit

The focused CTest uses invented glyph data at the measured row/column/color,
checks the affected rectangle and its neighbors, and checks a repeated call.
The private audit hash-locks every input above, checks the four-plane native
delta, and runs the same C model against the saved probe input. It launches
no emulator and publishes no media bytes.

```sh
cmake -S . -B resources/build-post-up-05fb-p9 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-05fb-p9 --target \
  poolrad_slot_a_post_up_outer_ui_first_printer_tests
ctest --test-dir resources/build-post-up-05fb-p9 \
  -R '^poolrad_slot_a_post_up_outer_ui_first_printer_tests$' --output-on-failure
python3 tools/replay/slot_a_post_up_outer_ui_capture_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --save resources/oracle-runs/slot-a-post-up-first-ui-writer-p9-111/POOLRAD/SAVGAMA.DAT \
  --outer-run resources/oracle-runs/slot-a-post-up-exit-p9-119 \
  --printer-run resources/oracle-runs/slot-a-post-up-first-ui-writer-p9-111 \
  --repeat-run resources/oracle-runs/slot-a-post-up-next-ui-p9-112 \
  --fixture resources/build-post-up-05fb-p9/poolrad_slot_a_post_up_outer_ui_first_printer_tests
```

Queued disjoint source-only follow-on: audit the original unit `0x3B0`
`+048B` call into `+01DA` and its prompt-row source pointer construction,
using [the pre-input prompt provenance](SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md)
as a lead. The existing evidence belongs to the **pre-Up** first view;
without a post-Up caller stop, this follow-on must leave post-Up prompt
reachability and page effect unknown.

Original binaries, save, probe input/output, RAM dumps, and EGA planes remain
ignored under `resources/`.
