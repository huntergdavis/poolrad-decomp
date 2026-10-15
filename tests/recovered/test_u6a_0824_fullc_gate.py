#!/usr/bin/env python3
"""Synthetic capture fixtures test gate mechanics, never native execution.

The positive fixture deliberately supplies the native-origin label; this
demonstrates why independently reviewing capture origin remains necessary.
All original-derived fixture RAM is temporary and under ignored resources/.
"""
import contextlib
import ctypes
from hashlib import sha256
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import check_u6a_0824_fullc_gate as gate


class CaptureGate(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = gate.base.load_source()
        cls.build = tempfile.TemporaryDirectory(prefix="u6a-gate-build-")
        cls.library = gate.base.build_library(Path(cls.build.name))

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def setUp(self):
        root = gate.base.ROOT / "resources/oracle-runs/u6a-0824-gate-tests"
        root.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=root)
        self.addCleanup(self.tmp.cleanup)
        self.directory = Path(self.tmp.name)
        memory, cpu = gate.base.make_case(self.source, list(range(20, -1, -1)))
        gate.base.put_word(memory, cpu.ss, cpu.sp, 0x0a51)
        gate.base.put_word(memory, cpu.ss, (cpu.sp + 2) & 0xffff, cpu.cs)
        gate.base.put_word(memory, cpu.ds, 0x5d96, 0x0100)
        gate.base.put_word(memory, cpu.ds, 0x5d98, 0x4000)
        a = gate.base.physical(cpu.cs, 0x0a4d)
        memory[a:a+4] = gate.base.OVR.read_bytes()[0x1827e:0x18282]
        ram = (ctypes.c_uint8 * gate.base.RAM_SIZE).from_buffer_copy(memory)
        returned = gate.base.Cpu.from_buffer_copy(cpu)
        self.library.pr_u6a_sort_0824(ram, len(ram), ctypes.byref(returned))
        self.memories = [bytearray(memory), bytearray(ram)]
        self.cpus = [{n: getattr(c, n) for n, _ in gate.base.Cpu._fields_}
                     for c in (cpu, returned)]
        self.origin = "dosbox-x-native-breakpoints"

    def write(self):
        names = "CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS".split()
        log = "".join("LOG: EV of '" + " ".join(names) + "' is:\nLOG: " +
                      " ".join(f"{regs[n.lower()]:x}" for n in names) + "\n"
                      for regs in self.cpus).encode()
        (self.directory / "debugger.log").write_bytes(log)
        manifest = dict(origin=self.origin, game_ovr_sha256=gate.base.OVR_SHA,
                        body_sha256=gate.base.SPAN_SHA,
                        debugger_log_sha256=sha256(log).hexdigest())
        for phase, memory, cpu in zip(("entry", "return"), self.memories, self.cpus):
            (self.directory / f"{phase}.bin").write_bytes(memory)
            manifest[f"{phase}_sha256"] = sha256(memory).hexdigest()
            manifest[f"{phase}_cpu"] = cpu
        (self.directory / "manifest.json").write_text(json.dumps(manifest))

    def validate(self):
        self.write()
        with contextlib.redirect_stdout(io.StringIO()):
            gate.native(self.directory, self.source, self.library)

    def test_synthetic_fixture_replay_plumbing(self):
        self.validate()

    def test_synthetic_origin_rejected(self):
        self.origin = "synthetic-com"
        with self.assertRaisesRegex(ValueError, "not labelled native"):
            self.validate()

    def test_wrong_caller_rejected(self):
        self.memories[0][gate.base.physical(self.cpus[0]["cs"], 0x0a4d)] ^= 1
        with self.assertRaisesRegex(ValueError, "caller bytes"):
            self.validate()

    def test_missing_party_ownership_rejected(self):
        gate.base.put_word(self.memories[0], self.cpus[0]["ds"], 0x5d96, 0)
        gate.base.put_word(self.memories[0], self.cpus[0]["ds"], 0x5d98, 0)
        with self.assertRaisesRegex(ValueError, "party-list node"):
            self.validate()

    def test_wrong_body_rejected(self):
        self.memories[0][gate.base.physical(self.cpus[0]["cs"], 0x0824)] ^= 1
        with self.assertRaisesRegex(ValueError, "loaded body"):
            self.validate()

    def test_changed_return_ram_rejected(self):
        self.memories[1][0x12345] ^= 1
        with self.assertRaisesRegex(ValueError, "returned RAM"):
            self.validate()

    def test_changed_return_flags_rejected(self):
        self.cpus[1]["flags"] ^= 1
        with self.assertRaisesRegex(ValueError, "returned CPU"):
            self.validate()

    def test_missing_native_cli_rejected(self):
        result = subprocess.run([sys.executable, str(Path(gate.__file__)),
                                 "--require-native"], capture_output=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn(b"strict gate requires --native", result.stderr)


if __name__ == "__main__":
    unittest.main()
