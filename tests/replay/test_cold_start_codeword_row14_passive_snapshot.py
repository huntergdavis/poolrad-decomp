"""Synthetic checks for a future private U19 passive plane probe record."""

from copy import deepcopy
import unittest

from tools.replay.cold_start_codeword_row14_passive_snapshot import validate_capture


STATE = {
    "cs": "0F2D", "ip": "0445", "es": "A000", "di": "118F",
    "gfx": "00000000000000000000", "seq": "000000000000",
    "latch": "12345678",
}


def record() -> dict:
    return {
        "version": 1,
        "run_token": "synthetic-U19",
        "plane_event_id": "synthetic-witness-1",
        "arm_token": "synthetic-arm-1",
        "store_ordinal": 7,
        "probe": "host_backing_word",
        "extent": "one_byte_per_plane",
        "site": 0x0445,
        "successor": 0x0449,
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
        "before": {
            "planes": "00000000",
            "state_before_copy": deepcopy(STATE),
            "state_after_copy": deepcopy(STATE),
        },
        "after": {
            "planes": "00200000",
            "state_before_copy": deepcopy(STATE),
            "state_after_copy": deepcopy(STATE),
        },
    }


class PassiveSnapshotTests(unittest.TestCase):
    def test_one_write_with_one_plane_change(self) -> None:
        result = validate_capture(record())
        self.assertEqual((result.site, result.successor, result.backing_index),
                         (0x0445, 0x0449, 0x118F))
        self.assertEqual(result.changed_planes, (1,))
        self.assertEqual((result.run_token, result.plane_event_id,
                          result.arm_token,
                          result.store_ordinal),
                         ("synthetic-U19", "synthetic-witness-1",
                          "synthetic-arm-1", 7))
        self.assertEqual(result.before_planes, bytes.fromhex("00000000"))
        self.assertEqual(result.after_planes, bytes.fromhex("00200000"))

    def test_observer_must_preserve_latch_and_controller(self) -> None:
        for field, changed in (("latch", "12345679"),
                               ("gfx", "01000000000000000000"),
                               ("seq", "010000000000")):
            with self.subTest(field=field):
                candidate = record()
                candidate["before"]["state_after_copy"][field] = changed
                with self.assertRaisesRegex(ValueError, "observation changed guest state"):
                    validate_capture(candidate)

    def test_handler_index_is_not_guessed_from_guest_offset(self) -> None:
        candidate = record()
        candidate["host_map"]["bank_read_full"] = 0x1000
        with self.assertRaisesRegex(ValueError, "handler index disagrees"):
            validate_capture(candidate)

    def test_selected_successor_and_single_write_required(self) -> None:
        candidate = record()
        candidate["successor"] = 0x0455
        with self.assertRaisesRegex(ValueError, "wrong selected store successor"):
            validate_capture(candidate)
        candidate = record()
        candidate["write_count"] = 2
        with self.assertRaisesRegex(ValueError, "exactly one selected write"):
            validate_capture(candidate)

    def test_opaque_event_join_needs_bounded_ordinal(self) -> None:
        candidate = record()
        candidate["store_ordinal"] = 129
        with self.assertRaisesRegex(ValueError, "join key"):
            validate_capture(candidate)
        candidate = record()
        candidate["arm_token"] = ""
        with self.assertRaisesRegex(ValueError, "join key"):
            validate_capture(candidate)

    def test_word_extent_cannot_be_labeled_as_full_plane_window(self) -> None:
        candidate = record()
        candidate["extent"] = "window_320_bytes_per_plane"
        with self.assertRaisesRegex(ValueError, "fixed one-word"):
            validate_capture(candidate)


if __name__ == "__main__":
    unittest.main()
