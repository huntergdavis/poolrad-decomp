# Focused recovered-test registration metadata

Run from any directory:

```sh
python3 tools/ci/audit_recovered_test_registration.py --json /tmp/recovered-tests.json
python3 tests/build/test_audit_recovered_test_registration.py
```

This audit inventories flat tests/recovered/test_*.c filenames and checks
their direct literal executable/source/CTest registrations in one CMake
file. It reads no C bodies, executes no CMake or tests, and uses no media.
It establishes metadata consistency, not behavior or strict-C credit.

Each inventoried source must appear once across add_executable or
PUBLIC/PRIVATE target_sources registrations. Every focused executable
must have exactly one add_test name. Additional helper source paths are
allowed; multiple focused sources may intentionally share one executable.
Duplicate executable/test names across the supplied file and add_test
commands naming undeclared executables also fail.

Supported registrations are top-level or under exactly if(BUILD_TESTING),
with matching executable/source/test guards. The reader does not evaluate
the guard or discover a configured build graph. Other conditional scopes,
alternate branches, variable/generator/list expressions, file sets,
ALIAS/IMPORTED executables and nonliteral/legacy add_test forms are
configuration errors. add_test uses exactly NAME name COMMAND executable;
wrappers, arguments, properties and include files are outside this scope.
Other CMake commands are only balanced syntactically, not evaluated.
Names, paths, comments, quoting, bracket arguments, BOM and CRLF follow
the bounded reader reused from source-registration audit 4768be5.

Use --root and --cmake to select the tree and input file. Exit 0 means the
scoped metadata matches; exit 1 reports missing/dangling/duplicate sources,
duplicate names, undeclared commands or missing/multiple focused CTests;
exit 2 means unsupported or malformed input, missing files or empty inventory.
--json writes schema_version 1, kind recovered_test_registration, the raw
CMake SHA256, all declared executable/test names, source inventory, issue
lists, and registrations mapping source, executable, test_names and lines.
A documentation audit can consume this explicit JSON interface separately.

At main217e809, 209 focused sources map to 209 executables and 209 CTest
names, among 245 total CTest names. Nine synthetic test methods exercise
valid helper sources, duplicate/missing/stale metadata, malformed input
and scope restrictions. Fixture contents deliberately are not C.

A bounded deja query found no completed implementation. Reused findings:
the main217e809 filename/CMake inventory and the independently reviewed
4768be5 literal CMake reader/JSON exit-code conventions. The reader is
included here so this main-based branch has no unmerged dependency.
