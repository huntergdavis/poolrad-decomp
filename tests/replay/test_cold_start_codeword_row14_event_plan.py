"""Synthetic same-run U19 CPU/EGA bundle tests; no original media."""

from pathlib import Path
import struct
import tempfile
import unittest

from tools.replay.cold_start_codeword_row14_event_plan import (
    audit_bundle, private_path,
)


def frame(cs, ip, es=0, di=0):
    return {name: f"{value:04X}" for name, value in
            zip(("cs", "ip", "es", "di"), (cs, ip, es, di))}


class EventPlanTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.frames = {
            "version": 1, "run_token": "synthetic_001",
            "frames": [frame(0x38D3, 0x02D8),
                       frame(0x0F2D, 0x0445, 0xA000, 0x118E),
                       frame(0x0F2D, 0x0412, 0xA000, 0x118F),
                       frame(0x0F2D, 0x0415, 0xA000, 0x118F)],
        }
        self.manifest = {
            "version": 1, "run_token": "synthetic_001",
            "window_start": 0x1180, "window_bytes": 0x140,
            "witness_offset": 0x118F,
            "events": [{
                "sequence": 1, "pc": "0F2D:0412", "next_pc": "0F2D:0415",
                "before": {"ram": "before.ram",
                           "planes": [f"before-{p}.bin" for p in range(4)],
                           "gc_before": [4, 0], "gc_after": [4, 0]},
                "after": {"ram": "after.ram",
                          "planes": [f"after-{p}.bin" for p in range(4)],
                          "gc_before": [4, 0], "gc_after": [4, 0]},
            }],
        }
        for name in ("before", "after"):
            ram = bytearray(655360)
            ds = 0x14A0 << 4
            struct.pack_into("<HH", ram, ds + 0x4A16, 0xA000, 0xA000)
            ram[ds + 0x4A1E] = 1
            (self.root / f"{name}.ram").write_bytes(ram)
            for plane in range(4):
                data = bytearray(0x140)
                if name == "after" and plane == 1:
                    data[0x0F] = 0x20
                (self.root / f"{name}-{plane}.bin").write_bytes(data)

    def test_one_paired_witness_event(self):
        gate = audit_bundle(self.root, self.frames, self.manifest)
        self.assertEqual((gate.selected.target_pc, gate.selected.next_pc,
                          gate.selected.candidate_hits), (0x0412, 0x0415, 2))
        self.assertEqual(gate.change.changed_witness_bits, (1,))

    def test_token_and_selected_pc_are_bound(self):
        wrong = dict(self.manifest, run_token="other_run")
        with self.assertRaisesRegex(ValueError, "same-run token"):
            audit_bundle(self.root, self.frames, wrong)
        wrong = dict(self.manifest, events=[dict(self.manifest["events"][0],
                                                pc="0F2D:0438")])
        with self.assertRaisesRegex(ValueError, "does not match selected"):
            audit_bundle(self.root, self.frames, wrong)

    def test_restore_and_four_planes_are_required(self):
        event = self.manifest["events"][0]
        before = dict(event["before"], gc_after=[4, 1])
        wrong = dict(self.manifest, events=[dict(event, before=before)])
        with self.assertRaisesRegex(ValueError, "not restored"):
            audit_bundle(self.root, self.frames, wrong)
        before = dict(event["before"], planes=event["before"]["planes"][:3])
        wrong = dict(self.manifest, events=[dict(event, before=before)])
        with self.assertRaisesRegex(ValueError, "eight distinct"):
            audit_bundle(self.root, self.frames, wrong)

    def test_samples_cannot_alias_or_change_read_map_baseline(self):
        event = self.manifest["events"][0]
        after = dict(event["after"], ram=event["before"]["ram"])
        with self.assertRaisesRegex(ValueError, "RAM files must be distinct"):
            audit_bundle(self.root, self.frames,
                         dict(self.manifest, events=[dict(event, after=after)]))
        after = dict(event["after"], planes=event["before"]["planes"])
        with self.assertRaisesRegex(ValueError, "eight distinct"):
            audit_bundle(self.root, self.frames,
                         dict(self.manifest, events=[dict(event, after=after)]))
        after = dict(event["after"], planes=[{}, "after-1.bin",
                                               "after-2.bin", "after-3.bin"])
        with self.assertRaisesRegex(ValueError, "eight distinct"):
            audit_bundle(self.root, self.frames,
                         dict(self.manifest, events=[dict(event, after=after)]))
        after = dict(event["after"], gc_before=[4, 1], gc_after=[4, 1])
        with self.assertRaisesRegex(ValueError, "baseline changed"):
            audit_bundle(self.root, self.frames,
                         dict(self.manifest, events=[dict(event, after=after)]))

    def test_missing_step_and_extra_event_rejected(self):
        wrong_frames = dict(self.frames, frames=self.frames["frames"][:-1])
        with self.assertRaisesRegex(ValueError, "STOP_MISSING_SINGLE_STEP"):
            audit_bundle(self.root, wrong_frames, self.manifest)
        wrong = dict(self.manifest, events=self.manifest["events"] * 2)
        with self.assertRaisesRegex(ValueError, "exactly one"):
            audit_bundle(self.root, self.frames, wrong)

    def test_paths_cannot_escape_private_run(self):
        with self.assertRaisesRegex(ValueError, "one private run"):
            private_path(self.root, self.root.parent / "other.json")
        self.assertEqual(private_path(self.root, self.root / "frames.json"),
                         self.root / "frames.json")

    def test_saved_page_segment_is_required(self):
        path = self.root / "after.ram"
        ram = bytearray(path.read_bytes())
        struct.pack_into("<H", ram, (0x14A0 << 4) + 0x4A18, 0xA200)
        path.write_bytes(ram)
        with self.assertRaisesRegex(ValueError, "page segments differ"):
            audit_bundle(self.root, self.frames, self.manifest)


if __name__ == "__main__":
    unittest.main()
