#!/usr/bin/env python3
"""The assert() census counts exactly the right files, and the CMake gate and canary are in place (no build needed)."""
# NOT_A_ROW_CHECKER: unit test of count_assert_tests.py and of the CMake NDEBUG gate; it checks no registered row
import tempfile
import unittest
from pathlib import Path

import count_assert_tests as counter

ROOT = Path(__file__).resolve().parents[2]


class CountTest(unittest.TestCase):
    def test_exact_set(self):
        with tempfile.TemporaryDirectory() as tmp:
            tests = Path(tmp) / "tests" / "recovered"
            tests.mkdir(parents=True)
            (tests / "a.c").write_text("#include <assert.h>\nint main(void){assert(1);return 0;}\n")
            (tests / "b.c").write_text("#define CHECK(x) do{}while(0)\nint main(void){CHECK(1);return 0;}\n")
            (tests / "c.c").write_text("#include <assert.h>\n/* assert(0) only in a comment */\nint main(void){return 0;}\n")
            (tests / "d.c").write_text("int f(void){return 0;} // assert(x)\n")
            files, based = counter.census(tmp)
            self.assertEqual([p.name for p in files], ["a.c", "b.c", "c.c", "d.c"])
            self.assertEqual([p.name for p in based], ["a.c"])

    def test_cmake_gate_and_canary_present(self):
        cmake = (ROOT / "CMakeLists.txt").read_text()
        self.assertIn("target_compile_options(${_poolrad_target} PRIVATE -UNDEBUG)", cmake)
        self.assertIn("add_test(NAME poolrad_assert_canary_tests COMMAND poolrad_assert_canary_tests)", cmake)
        canary = (ROOT / "tests/recovered/test_assert_canary.c").read_text()
        self.assertIn("#ifdef NDEBUG\n#error", canary)


if __name__ == "__main__":
    unittest.main()
