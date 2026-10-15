# Recovered-source registration consistency

```sh
python3 tools/ci/audit_recovered_source_registration.py \
  --json /tmp/recovered-source-registration.json
python3 tests/build/test_audit_recovered_source_registration.py
```

This standalone metadata audit compares the flat `src/recovered/*.c`
inventory with direct literal registrations in `poolrad_recovered` from
CMakeLists.txt. It reads filenames, CMake text and the exclusions manifest;
it does not read recovered C bodies, execute CMake, compile/link C, read
media or invoke binary-decode tooling. Consistent registration does not
establish parity, identity, reachability or strict-C accounting credit.

The reader recognizes add_library and PUBLIC/PRIVATE target_sources for
the selected target, including quoted paths, escaped whitespace, line
comments and bracket comments/arguments. Leading bracket newlines and
CRLF/UTF-8 BOM input normalization follow the
[official CMake language rules](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#bracket-argument),
while the report hashes the original input bytes. It normalizes relative path
spellings to detect duplicate registrations. Other targets do not satisfy
a missing registration in the selected target.

It deliberately rejects source arguments containing variables, generator/list
expressions, dollar/semicolon/wildcard characters,
absolute/escaping/nested recovered paths, conditional or function-scoped
registrations, INTERFACE/IMPORTED/ALIAS targets, FILE_SET registrations, and malformed CMake
syntax within its supported reader. It does not evaluate CMake control
flow or follow include() files. The report applies to direct registrations
in the specified CMake file, rather than claiming a complete configured
build graph. Supply the relevant explicit file through --cmake when needed.

Use --root for another repository tree, --target for another target name,
--cmake for an explicit CMake file and --exclusions for another manifest.
The checked-in manifest is intentionally empty at main217e809, where all
209 recovered C files have one direct registration each.

## Explicit exclusions

```json
{
  "schema_version": 1,
  "exclusions": [
    {"path": "src/recovered/example.c", "reason": "Concrete reason for intentionally leaving this existing file unregistered"}
  ]
}
```

Each exclusion must name one canonical flat path and a nonempty reason.
Wildcards, duplicate entries, unknown fields and unsupported schemas are
configuration errors. Exclusions for absent files are stale; exclusions
for already registered files conflict with the registry. Both fail the
audit. An exclusion never grants implementation or decompilation credit.

Exit 0 means the scoped inventory is consistent. Exit 1 reports missing,
duplicate or dangling registrations, stale exclusions, or registered
exclusions. Exit 2 reports invalid configuration, empty source inventory,
missing/ambiguous target definition, malformed input or unresolved source
arguments. --json includes ordered inventories, registration command/line
locations, exclusions and input SHA256 values; console diagnostics identify
the affected paths. No files are modified except an explicitly requested
JSON report.

Eight focused synthetic test methods cover successful literal registration
and exclusions, comments/quoted paths, registration errors, stale/conflicting
exclusions, invalid manifests, unresolved/dynamic scopes and malformed or
empty inputs. Fixture C contents are deliberately not compiled. A bounded
deja query returned no completed finding; the audit reuses the current-main
209-file literal-registration inventory and the standalone JSON/exit-code
conventions of the separately reviewed header audit, with no dependency on
that unmerged branch or any recovered-function implementation.
