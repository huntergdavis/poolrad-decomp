# Selected direct include-directory and feature metadata

```sh
python3 tools/ci/audit_recovered_include_features.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-include-features.json
python3 tests/build/test_audit_recovered_include_features.py
```

Selected targets are poolrad_recovered and the focused executable names in
a fresh passing a2cc22c schema1 registry. Reviewed b10cf51 checks bind that
registry to the current CMake SHA256 and flat test filename inventory.
The standalone reader/validator is reused inline from approved 641fc9b;
no unmerged module import is required.

Audit only direct target_include_directories and target_compile_features
calls in one supplied file. Selected targets must have unique compiling
definitions; declarations must follow them and preserve their top-level
or single BUILD_TESTING guard. One additional exact GNU|Clang compiler-ID
guard is supported as unevaluated conditional metadata. Other guards,
alternate branches and unresolved forms fail closed.

PUBLIC/PRIVATE/INTERFACE visibility, call order and supported SYSTEM and
BEFORE/AFTER include modifiers are preserved. Include items may be literal
relative paths that normalize within the source tree, or exactly
${CMAKE_CURRENT_SOURCE_DIR}/include. That sole variable template is
recorded by name/suffix, never expanded or evaluated; arbitrary variables,
different suffixes and generator expressions remain unsupported. Include
paths are not opened and their existence/contents are not checked.

Feature items must be literal identifier strings. This validates the
bounded spelling, not CMake's known-feature catalog or compiler support.
The official [include-directory command](https://cmake.org/cmake/help/latest/command/target_include_directories.html)
and [compile-feature command](https://cmake.org/cmake/help/latest/command/target_compile_features.html)
document the command syntax. This tool does not discover effective include
search paths, required compiler flags, language-standard defaults or
transitive/configured usage requirements. Nonselected direct calls are
listed by target/line, with their arguments outside audit scope.

Unknown direct target names fail with exit 1. Unsupported variables,
generator/list expressions, absolute/escaping paths, malformed modifiers,
empty visibility groups, order/scope errors and stale registries fail with
exit 2. Exit 0 means scoped metadata fits the supported bounds. Use --root
and --cmake for the matching tree/file. JSON records input hashes,
declarations/lines/modifiers/guards, item form/visibility, selected targets,
nonselected commands and all issues.

Measured main217e809 baseline: 210 selected targets; one PUBLIC recovered
library include declaration using the exact bounded variable template;
zero selected compile-feature declarations. Two include calls belong to
formats and a nonfocused replay executable. Zero direct feature calls
does not establish any configured compiler-feature requirement.

Eight synthetic methods cover bounded variables, literals/modifiers,
visibility/features, zero metadata, unknown/nonselected targets, unsupported
forms, genuine post-definition guard mismatches, order errors and stale
registries. No C bodies, CMake/compiler/tests execution, game media,
shared CMake edits or C-function credit. Bounded deja returned no completed
implementation; reused reviewed readers/validators and measured main inputs.
