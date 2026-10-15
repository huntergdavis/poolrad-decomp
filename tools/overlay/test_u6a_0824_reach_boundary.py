#!/usr/bin/env python3
"""Synthetic-only tests for the native-boundary audit; no media required."""
from hashlib import sha256
from pathlib import Path
import tempfile
import unittest

import u6a_0824_reach_boundary_audit as audit


def ev(values, ansi=False):
    gap = b"\r\x1b[49d" if ansi else b"\nLOG: "
    return (b"EV of 'CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS' is:" + gap +
            " ".join(f"{v:x}" for v in values).encode() + b"\r")


class BoundaryTests(unittest.TestCase):
    def setUp(self):
        self.values = [0x1796, 0x12, 0x814, 0x814, 0x193e, 0x80, 0x91c,
                       0, 0, 0xff, 0x814, 0x12, 0x80, 0x7202]
        self.events = [
            dict(seconds=1.0, kind="debug-command", command="BP 0899:0025"),
            dict(seconds=2.0, kind="debug-command", command="RUN"),
        ]
        self.log = b"DEBUG: Set breakpoint at 0899:0025\n" + ev(self.values)

    def test_plain_and_ansi_cpu_match(self):
        self.assertEqual(audit.parse_cpu(ev(self.values)),
                         audit.parse_cpu(ev(self.values, ansi=True)))
        self.assertEqual(len(audit.cpu_bytes(audit.parse_cpu(self.log)[0])), 28)

    def test_curses_redraw_is_repeated_record_not_extra_identity(self):
        rows = audit.parse_cpu(ev(self.values, True) * 2)
        self.assertEqual(len(rows), 2)
        self.assertEqual(len({audit.cpu_bytes(row) for row in rows}), 1)

    def test_incomplete_or_overwide_cpu_rejected(self):
        with self.assertRaisesRegex(ValueError, "no complete"):
            audit.parse_cpu(ev(self.values[:-1]))
        with self.assertRaisesRegex(ValueError, "no complete"):
            audit.parse_cpu(ev(self.values[:-1]) + b"DEBUG: Set breakpoint at 0899:0025\n")
        values = self.values.copy()
        values[0] = 0x10000
        with self.assertRaisesRegex(ValueError, "16 bits"):
            audit.parse_cpu(ev(values))

    def test_nonhit_requires_armed_confirmed_bp(self):
        cpus = audit.parse_cpu(self.log)
        self.assertEqual(audit.nonhit_window(self.events, cpus, self.log), 2.0)
        with self.assertRaisesRegex(ValueError, "not armed"):
            audit.nonhit_window(self.events[1:], cpus, self.log)
        with self.assertRaisesRegex(ValueError, "confirmation"):
            audit.nonhit_window(self.events, cpus, ev(self.values))

    def test_physical_alias_of_target_rejects_nonhit(self):
        cpu = audit.parse_cpu(self.log)[0]
        cpu.update(cs=0x089a, ip=0x0015)  # Same physical address as 0899:0025.
        with self.assertRaisesRegex(ValueError, "stop observed"):
            audit.nonhit_window(self.events, [cpu], self.log)

    def test_bp_deletion_requires_narrower_window(self):
        events = self.events + [dict(seconds=3, kind="debug-command", command="BPDEL *")]
        with self.assertRaisesRegex(ValueError, "narrower"):
            audit.nonhit_window(events, audit.parse_cpu(self.log), self.log)

    def test_invalid_timeline_rejected(self):
        for value in (-1, float("nan")):
            events = [dict(self.events[0], seconds=value), self.events[1]]
            with self.assertRaisesRegex(ValueError, "timeline"):
                audit.nonhit_window(events, audit.parse_cpu(self.log), self.log)

    def test_capture_hash_and_size_are_both_required(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "synthetic.bin"
            path.write_bytes(b"synthetic")
            digest = sha256(path.read_bytes()).hexdigest()
            self.assertEqual(audit.read_locked(path, digest, 9), b"synthetic")
            with self.assertRaisesRegex(ValueError, "size"):
                audit.read_locked(path, digest, 10)
            with self.assertRaisesRegex(ValueError, "hash"):
                audit.read_locked(path, "0" * 64, 9)

    def test_incomplete_cpu_record_rejected(self):
        cpu = audit.parse_cpu(self.log)[0]
        cpu.pop("flags")
        with self.assertRaisesRegex(ValueError, "CPU record"):
            audit.cpu_bytes(cpu)


if __name__ == "__main__":
    unittest.main()
