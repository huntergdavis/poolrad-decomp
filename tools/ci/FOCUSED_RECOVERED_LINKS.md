# Hash/registry-bound focused direct links

```sh
python3 tools/ci/audit_focused_recovered_links.py \
  --registry /tmp/recovered-tests.json --json /tmp/focused-links.json
python3 tests/build/test_audit_focused_recovered_links.py
```

This stricter profile reuses reviewed required-link audit 42bfd19 inline.
A fresh passing schema1 registry from a2cc22c binds selected executable
names to the current CMake SHA256 and test filename inventory through
reviewed b10cf51 validation. No unmerged import is required.

Each focused recovered-test executable must directly link poolrad_recovered
once through PUBLIC/PRIVATE target_link_libraries. The default auxiliary
profile allows poolrad_formats, matching four measured current users.
Other libraries are out of scope even when declared in the supplied file;
repeat --allow-library NAME to record deliberate additional allowances.
Unknown library names and unknown direct link-target names also fail.
INTERFACE items are out of scope for this selected compile-link profile.
Duplicate required links within/across calls and missing required links fail.

Selected declarations must follow the executable definition and share its
top-level or single BUILD_TESTING guard. The required library must be one
top-level compiling definition. Dynamic/list/generator, path/option,
conditional library-selector, legacy visibility and malformed forms fail
closed. Nonselected link calls are inventoried by target/line, while their
library values are outside this audit's selected scope. Unknown target
names anywhere in the direct file remain issues.

Only JSON, CMake text and flat test filenames are read. This is direct
one-file metadata, not configured/transitive linkage, link success, test
execution, C parity or C-function credit. No C bodies, game media, compiler
or CMake execution, include traversal or shared CMake edits.

Use --root and --cmake for the matching input tree/file. Exit 0 means scoped
metadata matches the profile; exit 1 reports membership/link profile issues;
exit 2 means stale, malformed or unsupported input. JSON includes hashes,
selected targets, declarations/lines/visibility, explicit auxiliary profile,
nonselected declarations and issue lists.

At main217e809, 209 selected targets have 209 required direct links and no
issues. Ten synthetic methods reuse eight reviewed link fixtures and add
declared-but-out-of-profile libraries, nonselected-target inventory and
before-definition failures. All pass from /tmp. Bounded deja returned no
completed implementation; reused 42bfd19 checks, a2cc22c/b10cf51 registry
interface and the measured main inventory.
