# Standalone recovered-header compilation

Run the source-only audit from any working directory:

```sh
python3 tools/ci/audit_recovered_headers.py --json /tmp/recovered-headers.json
python3 tests/build/test_audit_recovered_headers.py
```

Each `include/poolrad/recovered/*.h` gets a separate synthetic translation
unit containing only its include and a minimal main function. No preceding
header can supply a missing dependency. The compiler receives C11,
-Wall, -Wextra, -Wpedantic, -Werror and -fsyntax-only. This checks syntax,
self-contained declarations and warnings; it performs no linking or program
execution and reads no game media or binary-decode artifacts.

The default compiler is CC, or cc when CC is unset. `--cc` accepts a compiler
command with optional wrapper arguments, parsed without shell execution.
CFLAGS is not added implicitly. Use `--include-root` for another public
include tree, `--jobs` for concurrency (default 4), and `--timeout` for a
positive finite per-invocation bound (default 10 seconds). GCC/Clang-style
compiler flags and a --version command are required.

Exit 0 means every discovered header compiled. Exit 1 means at least one
header failed or timed out. Exit 2 means configuration/tooling failed,
including an empty header inventory or unavailable compiler. Console
diagnostics are abbreviated; `--json` records ordered per-header statuses,
full compiler diagnostics, header SHA256 values, the compiler version and
exact compile arguments. Paths with spaces are supported.

The focused tests use temporary synthetic headers to demonstrate independent
translation units and warnings-as-errors, plus missing-compiler, empty-tree
and timeout failures. At main 217e809, all 209 recovered headers pass with Ubuntu
GCC 15.2.0. That inventory describes public headers only; compilation does
not establish behavioral parity, function identity, reachability or strict-C
accounting credit.

Prior findings: the bounded deja query produced no completed finding. The
audit reuses the repository's C11 requirement and warning conventions from
[CMakeLists.txt](../../CMakeLists.txt), adding standalone translation units,
-Werror and bounded syntax-only compiler invocations.
