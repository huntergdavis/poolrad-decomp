# RES03F1 native executable provenance

Task t108, RES03F1 clear0137 companion. Base: `52aac3da2c6db39979b072ae49cb0efc905dbef3`.

The native provider resolves DOSBox-X once through the shared helper, launches that returned path, and records executable version/hash/pin status. Its audit companion remains the row checker. Recovered C, cases, masks, captured windows and tracing are unchanged. This is provenance evidence, not conversion credit.

Normal heavy ticket 845 measured candidate `1d55c0cd37a04e0cdb1d736321aece9e278a2e1f` against the baseline provider preserved from the stated main revision. Both phases passed **63 cases**, with complete reports identical after removing only `dosbox_x`. The 16 transitive C/header hashes and executable identity match between phases. The candidate provider hash is `0d79de399113fcad48e30e8194d927953d227528d76ec3ab875d3cf45b8cee48`.

Both reports cover 14 CPU words, 256 stack bytes, 65,536 DS bytes and 262,144 EGA plane bytes, with 1,769 traced OUT events and zero flag masks. Both report five pre-call-code cases, 22 pre-OUT-port cases and two packed-pre-write cases. Configuration: EGA, mode 0Dh, normal core, 386 CPU, COM segment 0814. Capture SHA-256 is `d1408f56bba2b644c7e2c14accd898dbfe17f930696f4dd7253f9a706b9cc289` in both phases.

The resolved executable is `/usr/bin/dosbox-x`, version `DOSBox-X version 2026.01.02 SDL2`, version-command exit 1, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`. The helper records source pin `5a9c7c53481c2e933948039c33256776653f4faf` and **`pinned_build_used: false`**. This identifies the installed binary; it does not certify a pinned build.

The native report has `result_sha256` for captured output; it does not emit an original-body `source_sha256` field. The existing audit source checks are a separate mechanism. Both phases preserved identical audit source metadata for all five bodies. For 03F1:0137, that separate metadata records 433 bytes, 188 instructions and SHA-256 `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc`. This change adds no original-body pin or pinned-build certification.

The CSV removes exactly this provider: 184 lists/169 files becomes **183/168**. A fresh tracked-tools AST census, excluding test files, matched the CSV at the measured candidate. The final evidence commit changes only this document; its receipt rechecks the one provider and 16 model hashes to bind the measurement and census to the final head.

Original media and generated programs remain under ignored resources. Private receipts: `resources/t108-batch6/resume/`, including before/after reports, separate source metadata, model hashes and `source-census.json`. Final-head media and reviewer gates are reported separately in the PR and task record. No natural-route/full-game/full-RAM claim or recovery/replacement credit is made.
