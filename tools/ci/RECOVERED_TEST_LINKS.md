# Required-library metadata for focused recovered tests

```sh
python3 tools/ci/audit_recovered_test_links.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-test-links.json
python3 tests/build/test_audit_recovered_test_links.py
```

This standalone audit consumes the passing schema_version 1 registry from
focused-test audit a2cc22c. It verifies the raw CMake SHA256 and current flat
tests/recovered/test_*.c filename inventory using reviewed b10cf51 validation.
It reads only JSON, CMake text and filenames; no C bodies, CMake execution,
test execution, media or strict-C inference.

Every focused executable must link poolrad_recovered exactly once through
literal PUBLIC or PRIVATE target_link_libraries groups. INTERFACE occurrences
are reported and do not satisfy that requirement. Required links duplicated
within or across declarations fail. Focused link declarations must follow
their executable definition and share its supported top-level or single
if(BUILD_TESTING) scope. The required library must have one top-level
non-ALIAS/non-IMPORTED/non-INTERFACE add_library definition.

Unknown target_link_libraries target names anywhere in the specified CMake
file are reported. Focused library names must be declared in that file or
explicitly supplied with repeated --allow-library options, for example m
for an intentional external system library. The audit cannot discover
libraries declared by included files. Variables, generator/list expressions,
legacy visibility forms, empty groups, debug/optimized selectors, options
and library paths are unsupported configuration input. Other CMake commands
are balanced by the bounded reader but not evaluated.

Use --root and --cmake to choose the matching registry tree/input file.
Exit 0 means the scoped required-link metadata matches; exit 1 reports
missing/duplicate/INTERFACE required links or unknown targets/libraries;
exit 2 means malformed, unsupported or stale inputs. JSON includes raw input
hashes, focused targets, every checked declaration's lines and library
visibility, explicit external allowances and issue lists. This is a direct
metadata check, not a configured build/link success claim.

At main217e809: 209 focused targets, 209 direct declarations, 209 required
PRIVATE links, no issues. Four targets additionally link declared
poolrad_formats. Eight synthetic methods cover valid links, missing and
duplicate links, unknown names, external allowances, INTERFACE handling,
malformed/unsupported forms, stale registries and invalid required-library
definitions.

A bounded deja query returned no completed implementation. Reused the
approved a2cc22c literal reader and b10cf51 registry freshness validator
inline, plus their measured current-main inventory. No unmerged code import
or shared CMake modification is required.
