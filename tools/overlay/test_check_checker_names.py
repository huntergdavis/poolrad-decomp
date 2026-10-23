#!/usr/bin/env python3
"""The undefined-name gate: both outcomes proven on synthetic checkers, and the registered checkers on main pass."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_checker_names as gate


class NameGateTest(unittest.TestCase):
    def write(self, **files):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        for name, text in files.items():
            (Path(folder.name) / name).write_text(text)
        return Path(folder.name)

    def test_a_hook_reading_an_unimported_constant_fails(self):
        # The #1437 shape: the Unicorn hook reads a register constant the module never imports.
        root = self.write(**{"a.py": "from unicorn.x86_const import UC_X86_REG_CX\n"
                                     "def compare(uc):\n"
                                     "    def on_instruction(uc, address, size, user):\n"
                                     "        return uc.reg_read(UC_X86_REG_DX), uc.reg_read(UC_X86_REG_CX)\n"
                                     "    return on_instruction\n"})
        self.assertEqual(gate.undefined(root / "a.py"), [("compare.on_instruction", "UC_X86_REG_DX")])
        self.assertEqual(gate.main([str(root / "a.py")]), 1)

    def test_bound_names_pass(self):
        root = self.write(**{"base.py": "SIZE = 1 << 20\n",
                             "a.py": "from base import *\n"
                                     "import os\n"
                                     "LIMIT = 3\n"
                                     "def setup():\n"
                                     "    global ENTRY\n"
                                     "    ENTRY = 0x057A\n"
                                     "def run(x):\n"
                                     "    for k in range(LIMIT):\n"
                                     "        x += k + ENTRY + SIZE + len(os.sep)\n"
                                     "    return [y for y in (x,) if y], helper()\n"
                                     "def helper():\n"
                                     "    try:\n"
                                     "        return __file__\n"
                                     "    except ValueError as error:\n"
                                     "        return error\n"})
        self.assertEqual(gate.undefined(root / "a.py"), [])
        self.assertEqual(gate.main([str(root / "a.py")]), 0)

    def test_every_registered_checker_on_this_tree_passes(self):
        self.assertEqual(gate.main([]), 0, "a registered checker reads a name it never binds: run check_checker_names.py")


if __name__ == "__main__":
    unittest.main()
