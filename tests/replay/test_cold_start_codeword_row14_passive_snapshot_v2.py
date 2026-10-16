"""Synthetic checks for one supplied U19 v2 four-plane packet."""

from copy import deepcopy
import json
import unittest

from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    MAX_PACKET_BYTES, parse_capture_v2, validate_capture_v2,
)
from tools.replay.cold_start_codeword_row14_target_filter import NEXT_PC


def record(site: int = 0x0445, ordinal: int = 897,
           observed_ip: int | None = None) -> dict:
    ip = site if observed_ip is None else observed_ip
    state = {
        "cs": "0F2D", "ip": f"{ip:04X}", "es": "A000", "di": "118F",
        "gfx": "00000000000000000000", "seq": "000000000000",
        "latch": "12345678",
    }
    return {
        "version": 2,
        "run_token": "synthetic-run",
        "arm_token": "synthetic-arm",
        "stream_id": "synthetic-stream",
        "plane_event_id": "synthetic-plane",
        "raw_sha256": "a" * 64,
        "candidate_ordinal": ordinal,
        "pc": f"0F2D:{site:04X}",
        "successor_pc": f"0F2D:{NEXT_PC[site]:04X}",
        "probe": "host_backing_word",
        "extent": "one_byte_per_plane",
        "witness": {"x": 122, "y": 112, "segment": "A000",
                    "offset": "118F", "bit": 0x20},
        "host_map": {
            "handler": "uvga_fast", "physical_address": 0xA118F,
            "page_mask": 0x1FFFF, "bank_read_full": 0,
            "memmask": 0x3FFFF, "memsize": 0x40000,
            "seq_memory_mode": 4, "gfx_miscellaneous": 0,
            "compatible_chain4": False, "ignore_odd_even": False,
            "ignore_extended_memory_bit": False, "is_vga_arch": False,
        },
        "handler_index": 0x118F,
        "write_count": 1,
        "before": {"planes": "00000000",
                   "state_before_copy": deepcopy(state),
                   "state_after_copy": deepcopy(state)},
        "after": {"planes": "00200000",
                  "state_before_copy": deepcopy(state),
                  "state_after_copy": deepcopy(state)},
    }


def raw(value: dict) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


class PassiveSnapshotV2Tests(unittest.TestCase):
    def test_all_sites_accept_site_or_successor_frame_and_late_ordinals(self) -> None:
        for site, successor in NEXT_PC.items():
            for observed_ip in (site, successor):
                with self.subTest(site=site, observed_ip=observed_ip):
                    value = record(site, 2304, observed_ip)
                    result = parse_capture_v2(raw(value))
                    self.assertEqual((result.version, result.site, result.successor),
                                     (2, site, successor))
                    self.assertEqual((result.pc, result.successor_pc),
                                     (value["pc"], value["successor_pc"]))
                    self.assertEqual(result.candidate_ordinal, 2304)
                    self.assertEqual(result.changed_planes, (1,))
                    self.assertEqual(result.before_planes, bytes(4))
                    self.assertEqual(result.after_planes,
                                     bytes.fromhex("00200000"))

    def test_eight_identity_fields_stay_distinct_for_later_join(self) -> None:
        value = record()
        result = validate_capture_v2(value)
        for name in ("run_token", "arm_token", "stream_id", "plane_event_id",
                     "raw_sha256", "candidate_ordinal", "pc", "successor_pc"):
            self.assertEqual(getattr(result, name), value[name])
        swapped = record()
        swapped["run_token"], swapped["arm_token"] = (
            swapped["arm_token"], swapped["run_token"])
        self.assertNotEqual(validate_capture_v2(swapped).run_token,
                            result.run_token)

    def test_exact_packet_keys_and_version(self) -> None:
        for change in (lambda value: value.pop("stream_id"),
                       lambda value: value.update(extra="x"),
                       lambda value: value.update(version=1),
                       lambda value: value.update(version=True),
                       lambda value: value.update(version=2.0)):
            value = record()
            change(value)
            with self.assertRaises(ValueError):
                validate_capture_v2(value)

    def test_identity_types_bounds_and_canonical_pc_pair(self) -> None:
        for name, bad in (("run_token", ""), ("arm_token", "bad token"),
                          ("stream_id", "x" * 65), ("plane_event_id", True),
                          ("raw_sha256", "A" * 64),
                          ("candidate_ordinal", 0),
                          ("candidate_ordinal", 2305),
                          ("candidate_ordinal", True),
                          ("candidate_ordinal", 897.0),
                          ("pc", "0f2d:0445"),
                          ("pc", "0F2E:0445"),
                          ("successor_pc", "0F2D:0455")):
            with self.subTest(name=name, bad=bad):
                value = record()
                value[name] = bad
                with self.assertRaises(ValueError):
                    validate_capture_v2(value)

    def test_observed_frame_must_match_selected_store_across_samples(self) -> None:
        wrong_site = record(0x0451, observed_ip=0x0445)
        with self.assertRaisesRegex(ValueError, "observed write frame disagrees"):
            validate_capture_v2(wrong_site)
        for field, bad in (("cs", "0F2E"), ("es", "A001"),
                           ("di", "1190")):
            with self.subTest(field=field):
                value = record()
                for sample in (value["before"], value["after"]):
                    for key in ("state_before_copy", "state_after_copy"):
                        sample[key][field] = bad
                with self.assertRaisesRegex(ValueError, "observed write frame disagrees"):
                    validate_capture_v2(value)
        changed = record()
        for key in ("state_before_copy", "state_after_copy"):
            changed["after"][key]["ip"] = "0449"
        with self.assertRaisesRegex(ValueError, "observed write frame changed"):
            validate_capture_v2(changed)

    def test_plane_extent_host_index_and_copy_state(self) -> None:
        for change in (
            lambda value: value.update(extent="window_320_bytes_per_plane"),
            lambda value: value["before"].update(planes="0000000000"),
            lambda value: value.update(handler_index=0x1190),
            lambda value: value.update(write_count=2),
            lambda value: value["witness"].update(x=122.0),
            lambda value: value["before"]["state_after_copy"].update(
                latch="12345679"),
        ):
            value = record()
            change(value)
            with self.assertRaises(ValueError):
                validate_capture_v2(value)
        unchanged = record()
        unchanged["after"]["planes"] = unchanged["before"]["planes"]
        self.assertEqual(validate_capture_v2(unchanged).changed_planes, ())

    def test_host_vga_registers_use_source_byte_width(self) -> None:
        for name in ("seq_memory_mode", "gfx_miscellaneous"):
            for value in (-1, 256):
                with self.subTest(name=name, value=value):
                    candidate = record()
                    candidate["host_map"][name] = value
                    with self.assertRaises(ValueError):
                        validate_capture_v2(candidate)
            candidate = record()
            candidate["host_map"][name] = 255
            self.assertEqual(validate_capture_v2(candidate).backing_index, 0x118F)

    def test_raw_byte_bound_duplicate_keys_and_single_json_object(self) -> None:
        value = record()
        encoded = raw(value)
        self.assertLess(len(encoded), MAX_PACKET_BYTES)
        self.assertEqual(parse_capture_v2(encoded).candidate_ordinal, 897)
        exact_bound = encoded + b" " * (MAX_PACKET_BYTES - len(encoded))
        self.assertEqual(parse_capture_v2(exact_bound).candidate_ordinal, 897)
        with self.assertRaisesRegex(ValueError, "byte bound"):
            parse_capture_v2(exact_bound + b" ")
        duplicate = encoded.replace(b'{"version":2',
                                    b'{"version":2,"version":2', 1)
        with self.assertRaisesRegex(ValueError, "duplicate packet JSON key"):
            parse_capture_v2(duplicate)
        for malformed in (encoded + b"{}", b"\xff" + encoded,
                          encoded.replace(b'"version":2', b'"version":NaN', 1)):
            with self.assertRaises(ValueError):
                parse_capture_v2(malformed)


if __name__ == "__main__":
    unittest.main()
