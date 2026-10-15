# Direct recovered compile metadata

```sh
python3 tools/ci/audit_recovered_compile_metadata.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-compile-metadata.json
python3 tests/build/test_audit_recovered_compile_metadata.py
```

The selected targets are poolrad_recovered plus the focused executable names
from a passing a2cc22c JSON registry. Reviewed b10cf51 validation binds the
registry to the current CMake SHA256 and flat test filename inventory.
Only JSON, CMake text and filenames are read; no C bodies, configured build,
compiler/tests execution, game media or strict-C inference.

This tool audits direct target_compile_options/target_compile_definitions
declarations in one specified file. Each selected target must have one
literal compiling add_library/add_executable definition. Declarations must
follow that definition and preserve its supported top-level or single
BUILD_TESTING guard. An additional exact
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang") guard is supported as recorded
conditional metadata. The tool never evaluates either guard. Other scopes,
alternate branches and unresolved expressions fail closed.

PUBLIC/PRIVATE/INTERFACE groups, declaration order, option BEFORE markers
and raw literal values are preserved. Definitions accept macro identifiers,
optional leading -D and optional literal values. Empty definition strings
and standalone -D are recorded as supplied, not counted as emitted macros.
The official [compile-options command](https://cmake.org/cmake/help/latest/command/target_compile_options.html)
and [compile-definitions command](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)
document visibility and argument semantics; this report does not reproduce
their configured property processing.

Variables, generator/list expressions, SHELL groups, wildcard/bracket
characters, control characters, unsupported visibility forms and malformed
macro identifiers fail with exit 2. The bounded reader does not follow
include files, directory-wide options, source properties, compiler defaults,
dependency usage requirements or other transitive/configured settings.
Raw item counts are declaration metadata, not final compiler argument counts.
It does not normalize, deduplicate or infer whether a compiler accepts a flag.

Declared nonselected targets are listed by command/line, with their values
outside audit scope. A direct compile command naming a target absent from
the file's add_library/add_executable declarations is an exit 1 issue.
Exit 0 means selected direct metadata is within the supported bounds;
exit 2 covers malformed/unsupported/stale input. Use --root and --cmake for
the matching tree/file. JSON includes input hashes, selected targets,
declaration sequence, scope/visibility/literal items and excluded commands.

Measured main217e809 baseline: 210 selected targets (library + 209 focused
executables), one selected option declaration with three PRIVATE items
-Wall/-Wextra/-Wpedantic under the recorded GNU|Clang guard, zero selected
compile-definition declarations. Three declarations are outside selection:
poolrad_formats options and two replay-probe definition calls. This is
bounded direct metadata, not evidence of effective/transitive flags.

Eight synthetic methods cover conditional guards, BEFORE/order, raw
definitions, visibility groups, zero metadata, unknown/nonselected targets,
unsupported inputs, scope mismatch, order errors and stale registries.
Bounded deja returned no completed implementation. Reused the reviewed
a2cc22c reader and b10cf51 registry validator inline, plus measured main
metadata; no unmerged code import or shared CMake edit.
