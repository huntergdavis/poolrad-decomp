# Native companion entry points

Task t108, VP i28505/i28513 and reviewer i28499. VP-authorized base (i28568): `57764669ae521ffd4089e8883fadbd897101a823`.

Fourteen function-only companions from the merged t108 batches now refuse direct invocation with exit1 and an explicit matching audit.py --native command. The guard runs before optional imports, so refusal also works without Capstone, Unicorn or game media. Importing a companion through its audit does not enter the guard. No fixture, emulator launch, recovered C, capture window, mask or accounting row changes.

Scope: RES03F1 clear0137; RES0464 bitmap-release2595; RES00BA clear17E3/display1693/display1816; RES0709 clear0DD3/glyph01DF/printer0626/rect-fill00B7/screen0CAB/space05EB; U3C6 slot getters; U9E sidecar0160/sidecar-groups02FA. Working standalone native CLIs are excluded. Other companion families are outside this bounded fix.

The regression invokes every listed companion in separate normal and isolated Python processes (14 companions, 28 subcases), requires nonzero refusal and the exact actionable audit command, and rejects a dependency traceback or empty success. Baseline reproduction with the normal audit dependencies, regression failure on unmodified providers, passing regression on this candidate, and exact-head validation are recorded separately in the PR/task. None is claimed before measurement. Native execution itself is not exercised by this refusal test.

The final-head gate also executes the legitimate audit --native entry points for RES03F1 and RES0464, comparing complete reports (including executable identity) against the merged receipts:63 cases/capture SHA d1408f56bba2b644c7e2c14accd898dbfe17f930696f4dd7253f9a706b9cc289 and128 cases/capture SHA cd0e8baa51f317243b56f4d68ac6709376fba1aaf43937a00b0d20fe6f32a4c0, respectively. This is a required check, not a pass claim before execution. Other native companions are not rerun by that bounded check.
