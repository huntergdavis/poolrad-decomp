# Documentation references to registered recovered tests

```sh
python3 tools/ci/audit_recovered_test_documentation.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-test-docs.json
python3 tests/build/test_audit_recovered_test_documentation.py
```

Generate the registry with audit_recovered_test_registration.py from the
separately reviewed focused-test audit a2cc22c. This independent tool consumes
JSON schema_version 1, kind recovered_test_registration; it imports no
unmerged module and never reads C bodies or executes CMake, C or tests.
It reads the registry, CMake text only for its hash, focused-test filenames,
and selected Markdown files. No game media or binary tooling is involved.

Default documentation scope is root README.md and docs/**/*.md. Repeat --doc
to select explicit in-tree Markdown paths; --root and --cmake select the
matching repository and CMake file. A passing registry is required. The
input CMake SHA256 and current tests/recovered/test_*.c filename inventory
must still match it; malformed, inconsistent or stale registries fail with
exit 2 instead of reporting misleading documentation results.

Coverage means an exact textual mention of a registered source, executable
or CTest name in the selected files. Source mentions use the flat
tests/recovered/test_NAME.c spelling with ASCII filename characters.
Declared target/test names are recognized literally; undeclared
poolrad_NAME_tests tokens and focused source paths are stale references.
Partial regex prefixes such as poolrad_overlay_u6a do not count as exact
coverage or stale test names. URLs are skipped, while their visible link
labels may still provide exact references.

Inline Markdown links to focused-test source paths are also resolved
relative to the containing document (or the repository root for leading
slash paths). Incorrect local destinations are reported separately,
including a link that mentions an existing source but uses the wrong
relative directory. External links are not fetched. This is a bounded
reference scan, not a complete Markdown parser: reference-style link
definitions, escaped/nested destinations and fuzzy or regex test selectors
are outside the hyperlink check. Exact references in prose and code blocks
are included. A reference does not establish execution, parity, strict-C
credit, documentation quality or relevance to a particular function.

Exit 0 means every focused source has a reference and no stale reference or
broken supported source link was found. Exit 1 reports those issues; exit 2
reports invalid inputs or an empty document selection. JSON records input
and per-document SHA256 values, reference document/line locations, missing
source/executable/test mappings, stale references and broken destinations.
It does not modify documentation or registry files.

Measured baseline at main217e809: 417 Markdown files, 68 of 209 focused
sources referenced, 141 lacking exact references, zero stale references
and zero broken supported source links. Exit 1 is the expected baseline
report; it is a documentation coverage gap, not a test-registration or C
implementation failure.

Eight synthetic test methods cover valid targets/source links, missing and
stale references, broken relative links, external URLs/partial names,
stale registry hashes/inventories, malformed registries, document selection
and raw document hashes. A bounded deja query returned no completed
implementation. Reused the a2cc22c explicit JSON registry and measured
main217e809 source/test inventory; this branch has no code dependency on it.
