# Native companion entry points

Task t108, VP i28505/i28513 and reviewer i28499. Base: `c060a4f98313cf0790bfb12851ea4eb69970e83a`.

Thirteen function-only companions from the merged t108 batches now refuse direct invocation with exit1 and an explicit matching audit.py --native command. The guard runs before optional imports, so refusal also works without Capstone, Unicorn or game media. Importing a companion through its audit does not enter the guard. No fixture, emulator launch, recovered C, capture window, mask or accounting row changes.

Scope: RES03F1 clear0137; RES00BA clear17E3/display1693/display1816; RES0709 clear0DD3/glyph01DF/printer0626/rect-fill00B7/screen0CAB/space05EB; U3C6 slot getters; U9E sidecar0160/sidecar-groups02FA. Working standalone native CLIs are excluded. RES0464 bitmap-release remains on reviewed PR1601 until integration; other companion families are outside this bounded fix.

The regression invokes every listed companion in a separate isolated Python process without site packages, requires nonzero refusal and the exact actionable audit command, and rejects a dependency traceback or empty success. Baseline reproduction with the normal audit dependencies, regression failure on unmodified providers, passing regression on this candidate, and exact-head validation are recorded separately in the PR/task. None is claimed before measurement. Native execution itself is not exercised by this refusal test.
