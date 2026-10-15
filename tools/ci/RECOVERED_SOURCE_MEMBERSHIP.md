# Hash-bound flat recovered-source membership

```sh
python3 tools/ci/audit_recovered_source_membership.py \
  --expected-cmake-sha256 RECORDED_SHA256 --json /tmp/recovered-membership.json
python3 tests/build/test_audit_recovered_source_membership.py
```

Supply an independently recorded SHA256 of the intended CMake input.
The required --expected-cmake-sha256 binding detects stale inputs before
inventory comparison. Use --root and --cmake for another tree/file.

This stricter membership audit reuses reviewed 4768be5 source-inventory
findings and the a2cc22c literal reader. Unlike the earlier registration
audit, every selected library source argument must normalize to one flat
src/recovered/*.c member, and no exclusions apply. Headers, nested files,
other source directories, absolute paths and escaping paths are explicitly
out of scope. Missing, nonexistent and duplicate members fail, including
duplicate aliases such as ./src/recovered/a.c. Registrations on other
targets do not satisfy selected library membership.

The selected poolrad_recovered library must be one top-level direct
add_library definition. Literal PUBLIC/PRIVATE target_sources may follow
it. Dynamic/list/generator forms, conditional membership, file sets,
INTERFACE/IMPORTED/ALIAS targets, malformed syntax and unsupported forms
fail closed. The reader does not evaluate CMake or follow include files;
this is one-file direct metadata, not a configured build-graph claim.

Only CMake text and flat recovered filenames are read; no C bodies,
compilation, tests, game media or binary tooling. This check does not grant
strict-C or function credit. Exit 0 means scoped membership matches;
exit 1 reports missing/nonexistent/duplicate/out-of-scope entries; exit 2
reports invalid or stale hashes, unsupported/malformed inputs, missing
library definition or empty inventory. JSON records actual/expected hashes,
inventory, registration command/line/argument locations and issue lists.

Measured baseline main217e809: 209 filenames, 209 unique members and 209
entries; no issues. The recorded CMake SHA256 is
62a81af95391d3b703bf9500bd0a715f2dc0077f0d81cab354f599a8c7134f19.
Eight synthetic methods cover valid literal/bracket/BOM/CRLF inputs,
missing/nonexistent/duplicate/out-of-scope members, stale hashes, other-target
isolation, unsupported/scoped forms, malformed commands and empty inputs.

Bounded deja returned no completed implementation. Reused the approved
4768be5 inventory/path findings and a2cc22c literal reader inline, while
adding strict source bounds and mandatory expected-hash validation.
This independent main-based branch has no unmerged module dependency.
