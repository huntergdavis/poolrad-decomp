# U19 row-14 helper, latch and host-presentation boundary

This source-only follow-on narrows the future same-run correlation after the [merged page/RGB discriminator](U19_ROW14_PAGE_RGB_SAME_RUN_DISCRIMINATOR.md) and [page-to-host bridge](U19_ROW14_PAGE_TO_HOST_SOURCE_BRIDGE.md). It supplies no reached U19 helper, display latch, host-presented pixel, or new RGB observation. Heavy native remains held.

A bounded deja search for Poolrad U19 row14 nonperturbing four-plane latch capture and host display start returned no result before it was stopped. Reused research is PR #713's conditional INT 10h-to-EGA bridge and PR #723's read-only CRTC/MODE/DRAW command audit. The new check pins private DOSBox-X revision 5a9c7c53481c2e933948039c33256776653f4faf and heavy executable SHA-256 8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92.

## Newly separated host events

| Source and lines | SHA-256 | Conditional implication |
| --- | --- | --- |
| vga_draw.cpp 4089–4148 | 86621d8c607b3c6440eb2f044445af83ae5a3ee88d07e030450620131662e9ce | The latch callback may complete an on-demand frame before replacing real_start with display_start. A host update inside that preassignment call can still be from the old page. |
| vga_draw.cpp 6190–6220 | 7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b | A vertical-timer cycle ends the prior renderer update and can return before starting a new one. Record frame start and its actual real_start separately. |
| render.cpp 472–525 | c5285bcffae637e87351d8b840652c4ecd968e11846eb9c07d1515358baf59b3 | End-update returns when no frame is active and may skip graphics output when the frame produced no output. Calling it is not evidence of presentation. |
| sdlmain.cpp 3187–3270 | c705ac880a394a0e917bfd3d4d60224fe4f6aff2e946b13168d936f7b3e1d50e | Graphics end-update can return early or choose different output backends; a function entry is not proof that the selected backend completed. |

The source lock covers three complete private files and four windows. It adds a different boundary from PR #723: that note distinguishes programmed display_start, latched real_start, and live DRAW address; this note distinguishes a latch callback from a completed **new-page** renderer/backend update. The preassignment on-demand completion is the specific old-frame trap. A same-run RGB change captured during that stage cannot be attributed to the just-latched page.

## Future authorized one-run discriminator

1. First prove the saved U19 caller reaches RES_0709 +0087, its BIOS return +008C, and helper return; retain request WORD, caller, active-page RAM before/after and the same run token. Startup helper calls remain separate.
2. If the queued start changes, record the EGA latch callback entry, any preassignment completion, the real_start assignment, and callback exit as distinct events with one monotonic host event index. Passive debugger VGA CRTC and VGA MODE snapshots can bound state but cannot timestamp the callback between CPU stops. Do not use redraw or display override commands.
3. Tag each new renderer frame start with the real_start it actually used and the relevant mode, address shift and palette identity. At frame end, retain render.updating, active, outWrite, abort, frameskip and output-backend path. Require proof that the chosen backend completed for that frame rather than an early return or old-frame completion.
4. Capture decoded RGB after that completed backend event using a separately verified passive method, then correlate its interval with a nonperturbing same-run four-plane witness at selected A000:118F. The existing four PNGs are unsynchronized and cannot supply any of these event indices.

A helper call without a latch is a request only. A latch without a later successful backend event is a latched page only. A backend event without the same-frame real_start and palette/plane witness is a presentation event without pixel attribution. If the source branch, event order, or capture side effect is unknown, stop with that typed blocker. No inferred saved-row page flip, displayed RGB delta, C parity or full-frame ETA follows.

Read-only reproduction:

    python3 tools/replay/cold_start_codeword_row14_presentation_path_audit.py --source resources/dosbox-x-16a1-private-src --heavy resources/dosbox-x-heavy-src/src/dosbox-x
    python3 tools/replay/cold_start_codeword_row14_host_state_discriminator_audit.py --source resources/dosbox-x-16a1-private-src --heavy resources/dosbox-x-heavy-src/src/dosbox-x --run resources/oracle-runs/codeword-row14-p5-001

Only source addresses, hashes and conditional event relationships are public. Original media, CPU/RAM/plane captures, host event logs, palette and RGB stay under ignored resources/.
