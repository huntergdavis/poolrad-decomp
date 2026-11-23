"""Conformance: the C++ reader header and the Python reader must produce identical transcripts (synthetic fixtures)."""
import os
import shutil
import subprocess
import unittest
from pathlib import Path

from tests.hybrid import conformance_transcript as ct

ROOT = Path(__file__).resolve().parent.parent.parent
CPP_DIR = ROOT / "tools" / "hybrid" / "dosbox-x"


@unittest.skipUnless(shutil.which("g++"), "g++ not available")
class ConformanceTest(unittest.TestCase):
    def test_cpp_and_python_transcripts_match(self):
        build = Path(os.environ.get("CA_BUILD", str(ROOT / "scratch" / "conformance")))
        build.mkdir(parents=True, exist_ok=True)
        exe = build / "conformance_main"
        subprocess.run(["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O1", "-I", str(CPP_DIR),
                        str(CPP_DIR / "conformance_main.cpp"), "-o", str(exe)], check=True)
        cpp = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout
        py = ct.transcript()
        self.assertEqual(cpp.splitlines(), py.splitlines())
        # Counts that follow from the scenario design: 13 letter-plans of 7 keys, plus attempt-two (14), repeat (7),
        # guard-break (2 then abort), never-buffers (1 then abort), buffer-not-empty (7), non-prompt-then-prompt (7).
        self.assertEqual(sum(1 for x in py.splitlines() if x.startswith("SEND|")), 129)
        self.assertEqual(sum(1 for x in py.splitlines() if x.endswith("|completed")), 18)


if __name__ == "__main__":
    unittest.main()
