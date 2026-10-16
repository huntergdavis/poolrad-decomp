"""Synthetic eight-stop U19 route join tests; no original game data."""

from hashlib import sha256
from pathlib import Path
import struct
import tempfile
import unittest

from tools.replay.cold_start_codeword_row14_route_join import (
    STOP_IDS, audit_route,
)


def frame(cs, ip, es=0, di=0):
    return {key: f"{value:04X}" for key, value in
            zip(("cs", "ip", "es", "di"), (cs, ip, es, di))}


class RouteJoinTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.template = {
            "version": 1, "status": "SOURCE_ONLY_TEMPLATE_NO_NATIVE",
            "candidate_hit_cap": 128,
            "witness": {"segment": "A000", "offset": "118F", "bit": "20"},
            "ordered_stops": [{"id": key} for key in STOP_IDS],
        }
        self.frames = {
            "version": 1, "run_token": "synthetic_join",
            "frames": [frame(0x0824, 0x0122), frame(0x0840, 0x0025),
                       frame(0x38D3, 0x00FD), frame(0x38D3, 0x02D8),
                       frame(0x0F2D, 0x0626),
                       frame(0x0F2D, 0x0445, 0xA000, 0x118E),
                       frame(0x0F2D, 0x0412, 0xA000, 0x118F),
                       frame(0x0F2D, 0x0415, 0xA000, 0x118F),
                       frame(0x38D3, 0x02DD)],
        }
        self.route = {
            "version": 1, "run_token": "synthetic_join",
            "segments": {"resident": "0824", "u19": "38D3",
                         "printer": "0F2D"},
            "ordered_stops": [{"id": key, "frame_index": index} for
                              key, index in zip(STOP_IDS, (0, 1, 2, 3, 4, 6, 7, 8))],
            "pre_ram": "route-pre.ram", "post_ram": "route-post.ram",
        }
        self.manifest = {
            "version": 1, "run_token": "synthetic_join",
            "window_start": 0x1180, "window_bytes": 0x140,
            "witness_offset": 0x118F,
            "events": [{
                "sequence": 1, "pc": "0F2D:0412", "next_pc": "0F2D:0415",
                "before": {"ram": "store-before.ram",
                           "planes": [f"before-{p}.bin" for p in range(4)],
                           "gc_before": [4, 0], "gc_after": [4, 0]},
                "after": {"ram": "store-after.ram",
                          "planes": [f"after-{p}.bin" for p in range(4)],
                          "gc_before": [4, 0], "gc_after": [4, 0]},
            }],
        }
        for name in ("route-pre", "route-post", "store-before",
                     "store-after"):
            ram = bytearray(655360)
            ds = 0x14A0 << 4
            struct.pack_into("<HH", ram, ds + 0x4A16, 0xA000, 0xA000)
            ram[ds + 0x4A1E] = 1
            if name == "route-pre":
                self.put_far(ram, 0x0824, 0x0122, 0x9A, 0x0840, 0x0025)
                self.put_far(ram, 0x0840, 0x0025, 0xEA, 0x38D3, 0x00FD)
                self.put_far(ram, 0x38D3, 0x02D8, 0x9A, 0x0F2D, 0x0626)
            path = self.root / f"{name}.ram"
            path.write_bytes(ram)
            if name in ("route-pre", "route-post"):
                self.route[f"{name.replace('route-', '')}_ram_sha256"] = (
                    sha256(ram).hexdigest())
        for side in ("before", "after"):
            for plane in range(4):
                data = bytearray(0x140)
                if side == "after" and plane == 1:
                    data[0x0F] = 0x20
                (self.root / f"{side}-{plane}.bin").write_bytes(data)

    @staticmethod
    def put_far(ram, segment, offset, opcode, target_segment, target_offset):
        at = (segment << 4) + offset
        struct.pack_into("<BHH", ram, at, opcode, target_offset,
                         target_segment)

    def audit(self, route=None, frames=None, manifest=None):
        return audit_route(self.root, self.template,
                           self.route if route is None else route,
                           self.frames if frames is None else frames,
                           self.manifest if manifest is None else manifest)

    def test_complete_structural_join(self):
        gate = self.audit()
        self.assertEqual(gate.run_token, "synthetic_join")
        self.assertEqual(gate.selected_frame_index, 6)
        self.assertEqual(gate.event.selected.candidate_hits, 2)
        self.assertEqual(gate.event.change.changed_witness_bits, (1,))

    def test_missing_or_reordered_route_stops(self):
        wrong = dict(self.route, ordered_stops=self.route["ordered_stops"][:-1])
        with self.assertRaisesRegex(ValueError, "eight ordered stop"):
            self.audit(route=wrong)
        stops = [dict(stop) for stop in self.route["ordered_stops"]]
        stops[2]["frame_index"] = 4
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            self.audit(route=dict(self.route, ordered_stops=stops))
        stops = [dict(stop) for stop in self.route["ordered_stops"]]
        stops[3]["frame_index"] = 2
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            self.audit(route=dict(self.route, ordered_stops=stops))

    def test_wrong_selected_successor_and_earlier_witness(self):
        frames = dict(self.frames, frames=list(self.frames["frames"]))
        frames["frames"][7] = frame(0x0F2D, 0x043B, 0xA000, 0x118F)
        with self.assertRaisesRegex(ValueError, "STOP_WRONG_SINGLE_STEP"):
            self.audit(frames=frames)
        frames = dict(self.frames, frames=list(self.frames["frames"]))
        frames["frames"][5] = frame(0x0F2D, 0x0445, 0xA000, 0x118F)
        with self.assertRaisesRegex(ValueError, "STOP_WRONG_SINGLE_STEP"):
            self.audit(frames=frames)

    def test_return_before_witness_and_mismatched_token(self):
        frames = dict(self.frames, frames=list(self.frames["frames"]))
        frames["frames"][5] = frame(0x38D3, 0x02DD)
        with self.assertRaisesRegex(ValueError, "STOP_PRINTER_RETURN"):
            self.audit(frames=frames)
        with self.assertRaisesRegex(ValueError, "run tokens differ"):
            self.audit(route=dict(self.route, run_token="other"))

    def test_route_cannot_skip_first_pre_call_or_post_return(self):
        frames = dict(self.frames, frames=[frame(0x38D3, 0x02D8)] +
                      self.frames["frames"])
        stops = [dict(stop, frame_index=stop["frame_index"] + 1) for
                 stop in self.route["ordered_stops"]]
        with self.assertRaisesRegex(ValueError, "first caller"):
            self.audit(route=dict(self.route, ordered_stops=stops), frames=frames)
        frames = dict(self.frames, frames=self.frames["frames"] +
                      [frame(0x38D3, 0x02DD)])
        stops = [dict(stop) for stop in self.route["ordered_stops"]]
        stops[7]["frame_index"] = 9
        with self.assertRaisesRegex(ValueError, "first U19 return"):
            self.audit(route=dict(self.route, ordered_stops=stops), frames=frames)

    def test_relocated_target_filter_segments_are_typed_blocker(self):
        wrong = dict(self.route, segments=dict(self.route["segments"],
                                               u19="38D4"))
        with self.assertRaisesRegex(ValueError, "target filter pins"):
            self.audit(route=wrong)

    def test_pre_call_memory_mapping_and_distinct_capture_files(self):
        wrong = dict(self.route, post_ram="route-pre.ram")
        with self.assertRaisesRegex(ValueError, "distinct pre-call"):
            self.audit(route=wrong)
        wrong = dict(self.route, pre_ram="store-before.ram")
        with self.assertRaisesRegex(ValueError, "route and store RAM"):
            self.audit(route=wrong)
        ram_path = self.root / "route-pre.ram"
        ram = bytearray(ram_path.read_bytes())
        self.put_far(ram, 0x0840, 0x0025, 0xEA, 0x38D4, 0x00FD)
        ram_path.write_bytes(ram)
        wrong = dict(self.route, pre_ram_sha256=sha256(ram).hexdigest())
        with self.assertRaisesRegex(ValueError, "does not connect"):
            self.audit(route=wrong)

    def test_ram_digest_and_private_path_guard(self):
        wrong = dict(self.route, pre_ram_sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "SHA-256 differs"):
            self.audit(route=wrong)
        wrong = dict(self.route, post_ram="../other.ram")
        with self.assertRaisesRegex(ValueError, "one private run"):
            self.audit(route=wrong)


if __name__ == "__main__":
    unittest.main()
