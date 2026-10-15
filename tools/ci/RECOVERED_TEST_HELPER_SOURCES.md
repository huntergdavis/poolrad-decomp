# Direct focused-test auxiliary file membership

```sh
python3 tools/ci/audit_recovered_test_helper_sources.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-test-helpers.json
python3 tests/build/test_audit_recovered_test_helper_sources.py
```

This audit extends reviewed a2cc22c filename registration evidence to the
additional file arguments on focused executable definitions. It consumes
a fresh passing schema1 registry, using b10cf51 validation to bind its test
names and filename inventory to the current CMake SHA256. The reviewed
641fc9b literal reader/validator is included inline.

Each selected executable must have one top-level or single BUILD_TESTING
definition. Additional PUBLIC/PRIVATE target_sources must follow it and
share that exact guard. The registered test sources must remain present.
Every direct file argument is normalized and checked for duplicates within
its executable, existence as a file and containment in the source tree.
A helper shared by different executables is allowed and counted once in
each executable's declared membership.

Supported file paths are relative .c/.h arguments under src/, tests/ or
include/. Absolute/escaping paths, other suffixes/directories and symlinks
resolving outside those source directories are out of scope. Resources are
excluded before any file existence check. Dynamic/list/generator/file-set
forms, INTERFACE source groups, malformed commands and unsupported guards
fail closed. This is one-file direct metadata; include-file evaluation and
configured source expansion are outside scope.

Only JSON, CMake text, filenames, path/symlink metadata and file existence
are read. C bodies, header contents, compilation, CMake/tests execution,
game media and strict-C credit are excluded. File presence is not evidence
of implementation behavior or whether a build compiles that argument.

Use --root and --cmake for the matching input tree/file. Exit 0 means
scoped file membership matches; exit 1 reports duplicate, nonexistent,
out-of-scope or missing registered sources; exit 2 means stale, malformed
or unsupported input. JSON includes hashes, complete direct membership
locations, auxiliary occurrence/target/path counts and explicit issues.

At main217e809: 209 focused executables have 215 direct file arguments,
including six auxiliary occurrences across three executables and three
unique replay-helper paths. No issues. Nine synthetic methods cover valid
.c/.h arguments, sharing, duplicate aliases with exact diagnostic counts,
missing/escaping/absolute/resource paths, guard/order errors, dynamic and
malformed forms, symlink boundaries and stale registry inputs.

Bounded deja returned no completed implementation. Reused reviewed
a2cc22c registration evidence, b10cf51 freshness checks and 641fc9b reader,
plus the measured current-main helper inventory. No unmerged module import
or shared CMake edit.
