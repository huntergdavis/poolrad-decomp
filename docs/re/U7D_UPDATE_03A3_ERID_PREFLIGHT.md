# U7D +03A3 saved-slot-A E/R/I-or-D preflight

This is the source-only gate after the merged static route packet
[U7D_UPDATE_03A3_NATURAL_CALL_GATE.md](U7D_UPDATE_03A3_NATURAL_CALL_GATE.md).
It identifies the exact state and input checkpoints required before a native
U7D +03A3 frame can be claimed. It executes no game, changes no C routine,
and provides no natural, full-function, or strict-C credit. The targeted
deja query for this route returned no indexed session; the merged route,
[U1C_LOOP_3387.md](U1C_LOOP_3387.md), and
[SLOT_A_NATIVE_KEY_CALL_RETURN.md](SLOT_A_NATIVE_KEY_CALL_RETURN.md)
supplied the prior boundaries. The original route checker was rerun against
the ignored DOS v1.3 GAME.OVR (206105 bytes,
SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614).

## Ordered guards and source ownership

| Stop or input | Original condition / transfer | Existing C owner and limit |
| --- | --- | --- |
| U1C +3395 input | Unit U68 entry 2 returns the live key; +33BD must be able to read window-1 word +0594. | The merged bounded U1C +3387 model has an entry2_key hook. The saved-state adapter does not finish its preceding load, party, and initial-script effects. |
| U1C +33BD..+33EA | After upcase, key E (0x45) reaches +2DD3 only while DS:[49FF] is zero. A window-1 value above one bypasses the first key comparison, but the second comparison still tests E; at most one is not a required E guard. | The bounded U1C model has near_2dd3 as a fallible hook. This is not the 61-byte +2DD3 body. |
| U1C +2DEA | +2DD3 far-calls U6A public entry 1. | No merged full-state U1C +2DD3 or complete U6A entry-1 body is wired to the gameplay hook. |
| U6A +1F98/+1FA2 | In the entry-1 menu, R (0x52) calls +077A; a separate +13F1 menu has another R call at +149D. | Existing U6A party-before/after and +0824 models are bounded fragments; neither is this complete input/menu-to-+077A path. |
| U6A +07FD | +077A far-calls U7D public entry 2. | No merged complete full-state +077A call path or U7D entry-2 menu composition. |
| U7D +0D1C and +0813 | Entry 2 calls +0721; I (0x49) reaches +0813 CALL +03A3 after its indexed count update. | The merged full-state C owner begins only at +03A3. |
| U7D +0829/+0836, +083A, +055A | D (0x44) calls +047A at either menu site, then can reach +083A CALL +03A3; +047A can itself call +03A3 at +055A. Entry 2 can call +047A at +0D9D. | +047A identity and full-state composition remain unresolved. Keep the three predecessor call sites distinct. |
| U7D +03A3..+03D4 | The caller composes the merged +02F2 callee and outer RETF. | pr_u7d_update_03a3 is the exact C target; controlled full-state cases exist, but no naturally reached frame. |

The merged static checker pins the U1C E comparison/call, U6A R
comparisons/calls and far target, U7D public-entry/menu calls, and the
three +03A3 predecessor near calls against the original overlay.
It passes in this clone. A static route cannot establish input timing,
menu guards, loaded segments, callback effects, or a same-run +03A3
entry/return frame.

## Checkpoint and input gate

The checked-in slot-A one-Up watch used START.EXE SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
and SAVGAMA.DAT SHA-256
abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1.
It observed a +3395 input call and +33BD window-1 word zero, but the
returned key was Up (0x48), not E. Its later script checkpoint cannot be
substituted for an E/R/I-or-D run. This clone has GAME.OVR and START.EXE;
the cited slot-A save and +3395/+33BD replay snapshots are absent from
its ignored resources. No exact E-input checkpoint is locally verified.

Before any bounded native attempt, pin the actual saved-slot-A file and
pre-input +3395 or +33BD checkpoint by size and hash; record loaded U1C,
U6A and U7D CS, DS, SS, SP/BP, window-1 far pointer/extent and word
+0594, DS:[49FF], local key and prompt state. Record each input event
and its timing: E (or e after upcase), then R in the reached U6A menu,
then I or D in the reached U7D menu. At every transfer, preserve the
actual branch and caller frame. A redirected menu or nonreturn is an
explicit stop, not evidence of +03A3 reach.

A same-run +03A3 stop must record whether the predecessor was +055A,
+0813 or +083A, with expected *ordinary* return IP +055D, +0816 or
+083D respectively; the loaded CS and live return words still decide
the actual destination. Capture all 14 CPU words and 1 MiB A20-off RAM
at +03A3 and after the actual outer return. Compare the C caller from
that entry state against the native return, including every RAM byte,
all CPU words, inner +02F2 RETF4, stack offset wrap and physical aliases.
The current host native hold prohibits running this plan. Natural reach
and native full-state parity remain unproved.

## Local verification

The merged tests/recovered/check_overlay_u7d_03a3_natural_route.py
returned PASS on the original GAME.OVR; no emulator was used. This
note is a preflight record, not an additional function translation.
