# Direct focused-CTest property metadata

```sh
python3 tools/ci/audit_recovered_test_properties.py \
  --registry /tmp/recovered-tests.json --json /tmp/recovered-test-properties.json
python3 tests/build/test_audit_recovered_test_properties.py
```

This independent audit consumes the passing a2cc22c recovered-test JSON
registry and verifies its CMake SHA256 and focused-test filename inventory
using reviewed b10cf51 validation. Only JSON, CMake text and filenames are
read. It does not read C bodies, run CMake/tests, access game media or grant
strict-C credit.

Supported setters are direct literal
set_tests_properties(TEST_NAMES PROPERTIES PROPERTY VALUE ...)
and set_property(TEST TEST_NAMES PROPERTY PROPERTY VALUE).
The latter accepts one value. Setters must follow their add_test definitions
and share supported top-level or single if(BUILD_TESTING) guards. Unknown
test names anywhere in those setters fail. Other set_property scopes are
outside the test audit and do not contribute to its applicability count.
The supplied one-file reader does not follow include files or evaluate
control flow, inherited values or a configured CTest graph.

For focused tests, repeated assignments to the same property fail even when
their values agree or a later setter intentionally overrides an earlier
one. Any explicitly true DISABLED assignment is flagged; a later false
override does not erase that finding. The audit's initial recognized
property subset is DISABLED, TIMEOUT, WILL_FAIL, RUN_SERIAL, LABELS and
WORKING_DIRECTORY. Other keys are unknown to this bounded policy, not
necessarily invalid CMake. Repeated --allow-property UPPERCASE_NAME options
allow deliberate additional/custom keys without changing the policy file.
Other property values are checked only for supported literal syntax;
their detailed semantics are outside this audit.

The syntax and selected keys were checked against the official CMake
[set_tests_properties](https://cmake.org/cmake/help/latest/command/set_tests_properties.html),
[set_property](https://cmake.org/cmake/help/latest/command/set_property.html)
and [test-property index](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-tests).
The [DISABLED property](https://cmake.org/cmake/help/latest/prop_test/DISABLED.html)
controls whether CTest skips a test. This audit accepts case-insensitive
true constants 1/ON/YES/TRUE/Y and false constants
0/OFF/NO/FALSE/N/IGNORE/NOTFOUND, an empty value or a -NOTFOUND suffix.
Other numeric/string forms are unsupported, rather than evaluated.

Variables, generator/list expressions, DIRECTORY/APPEND/APPEND_STRING
forms, multiple set_property values, malformed pairs and unsupported scopes
fail with exit 2. Exit 1 reports disabled focused declarations, unknown
tests/keys or duplicate focused assignments. Exit 0 means no scoped issue
was found. --root and --cmake select the matching registry tree/input;
--json records input hashes, declaration/assignment counts, policy and
allowances, lines, literal values and all issues.

At main217e809, the parsed root CMake contains 245 literal add_test names,
209 focused test names and zero direct test-property setters/assignments.
That is bounded zero applicability, not evidence that configured tests
cannot be disabled elsewhere. Eight synthetic methods cover zero
applicability, supported values, disabled forms, unknown names, duplicates,
explicit custom keys, unsupported/malformed forms and stale input.

A bounded deja query returned no completed implementation. Reused the
approved a2cc22c literal reader, b10cf51 registry validator and measured
main217e809 metadata inline; no unmerged code import or shared CMake edit.
