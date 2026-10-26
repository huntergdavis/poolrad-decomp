# RES0464 native executable provenance

Task t108, bitmap-release 2595 companion. Fresh-main base: `52aac3da2c6db39979b072ae49cb0efc905dbef3`.

The provider resolves DOSBox-X once, launches that returned path, and records version, executable hash and pin status. Recovered C, existing cases, stop classifications, captured windows and comparisons remain unchanged. Its audit companion remains the row checker. No credit or accounting change is made.

Normal heavy ticket 859 measured candidate `6921abe81c4a56fbb22284b1836cfe622a62edc4` against the baseline provider preserved from the stated base. Both phases passed **128 cases: 113 returns and 15 RunError204 stops**. Complete reports are identical after removing only `dosbox_x`, including capture SHA-256 `cd0e8baa51f317243b56f4d68ac6709376fba1aaf43937a00b0d20fe6f32a4c0`. Both report 14 CPU words, 1,024 stack bytes, 65,536 DS bytes and 65,536 heap bytes, zero masked bits and COM segment 0814. These are bounded native captures, not a full-RAM or natural-route claim.

Both phases used identical executable identity and four transitive C/header hashes. Candidate provider SHA-256: `a2114e7c3bd1a42adde0baf76c227a1afeeb8aad657bc6364b232dc196444879`. The resolved executable is `/usr/bin/dosbox-x`, version `DOSBox-X version 2026.01.02 SDL2`, version-command exit 1, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`. The helper records source pin `5a9c7c53481c2e933948039c33256776653f4faf` and **`pinned_build_used: false`**. Identification is not pinned-build certification.

Native `result_sha256` hashes capture bytes; it is not an original-body digest. The native report has no original-body `source_sha256` field. The unchanged audit separately validates and retains source metadata; before/after metadata is identical, including caller sites and four FreeMem spans. RES0464:2595 is 157 bytes, 52 instructions, SHA-256 `d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700`. This change adds no original-body pin.

The CSV removes exactly this provider: **184 lists/169 files becomes 183/168**, relative to the stated base. A fresh tracked-tools AST census, excluding test files, matched the CSV at the measured candidate. The final evidence commit changes only this document; its receipt rechecks one provider and four model hashes to bind the measurement and census to the final head.

The native entry point is `/home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res0464_bitmap_release_2595_audit.py --native`. The companion `_native.py` only defines functions: invoking it directly exits zero without executing cases and is not verification. The before/after runner explicitly calls `native_audit(body,image,function)` with the unchanged audit's source and compiled C library.

Media and generated programs remain under ignored resources. Private receipts: `resources/t108-batch7/resume/`, including complete before/after reports, separate original-source metadata, model hashes and source census. Final-head media and reviewer results are reported separately in the PR and task record. No natural-route, full-game, full-RAM or recovery/replacement credit claim is made.
