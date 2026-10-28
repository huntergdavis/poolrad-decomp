# Native companion entry points

Task t108, VP i28505/i28513 and reviewer i28499. VP-authorized base (i28568): `57764669ae521ffd4089e8883fadbd897101a823`.

The initial fourteen function-only companions from the merged t108 batches refuse direct invocation with exit1 and an explicit matching audit.py --native command. The guard runs before optional imports, so refusal also works without Capstone, Unicorn or game media. Importing a companion through its audit does not enter the guard. No fixture, emulator launch, recovered C, capture window, mask or accounting row changes.

Scope: RES03F1 clear0137; RES0464 bitmap-release2595; RES00BA clear17E3/display1693/display1816; RES0709 clear0DD3/glyph01DF/printer0626/rect-fill00B7/screen0CAB/space05EB; U3C6 slot getters; U9E sidecar0160/sidecar-groups02FA. Working standalone native CLIs are excluded. Other companion families are outside this bounded fix.

The regression invokes every listed companion in separate normal and isolated Python processes (14 companions, 28 subcases), requires nonzero refusal and the exact actionable audit command, and rejects a dependency traceback or empty success. Baseline reproduction with the normal audit dependencies, regression failure on unmodified providers, passing regression on this candidate, and exact-head validation are recorded separately in the PR/task. None is claimed before measurement. Native execution itself is not exercised by this refusal test.

The final-head gate also executes the legitimate audit --native entry points for RES03F1 and RES0464, comparing complete reports (including executable identity) against the merged receipts:63 cases/capture SHA d1408f56bba2b644c7e2c14accd898dbfe17f930696f4dd7253f9a706b9cc289 and128 cases/capture SHA cd0e8baa51f317243b56f4d68ac6709376fba1aaf43937a00b0d20fe6f32a4c0, respectively. This is a required check, not a pass claim before execution. Other native companions are not rerun by that bounded check.

## Three-companion follow-up

Base `f4ca5c3c0d4af610cd1349bc948f6ce8f38d84cf`. This batch adds early refusals to `ovl_0025_0341_native.py`, `system_freemem_0254_native.py` and `u38_roll_gate_2915_native.py`, bringing the regression to 17 companions in normal and isolated modes (34 subcases). The first must name `ovl_0025_0341_cpu_audit.py --native`; the other two name their matching `_audit.py --native`. The regression carries an explicit override for the first command.

Only the OVL0025 companion defines top-level `def native(`. FreeMem and U38 expose `native_audit`, outside the earlier 68-module filtered population. Do not subtract three from that subset's 54 unguarded modules. Remaining work stays in t108.

C, imported native functions, fixtures and capture contracts are unchanged. The two bare-launcher inventory rows move down three lines; membership is unchanged. OVL0025 uses an absolute executable path and has no row in that inventory. Baseline/candidate legitimate native reports, exact-command red/green regression, original AST preservation, fresh census and final-head media results are recorded separately in the PR/task. No pass is claimed before measurement. Media/generated outputs stay under ignored resources; no credit requested.
