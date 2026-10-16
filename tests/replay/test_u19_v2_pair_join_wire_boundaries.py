"""Synthetic raw-wire checks for the source-only U19 v2 pair join."""

import json
import unittest

from tests.replay import test_cold_start_codeword_row14_v2_pair_join as fixtures
from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    MAX_PACKET_BYTES,
)
from tools.replay.cold_start_codeword_row14_v2_pair_join import join_pair_v2


def duplicate_member(packet, path, name, duplicate_value):
    """Insert a second member in exactly one nested object on the wire."""
    def encode(value, remaining):
        if not remaining:
            assert isinstance(value, dict) and name in value
            original = json.dumps(value, separators=(",", ":"))
            return (original[:-1] + "," + json.dumps(name) + ":" +
                    json.dumps(duplicate_value, separators=(",", ":")) + "}")
        assert isinstance(value, dict) and remaining[0] in value
        members = []
        for key, child in value.items():
            encoded = (encode(child, remaining[1:]) if key == remaining[0]
                       else json.dumps(child, separators=(",", ":")))
            members.append(json.dumps(key) + ":" + encoded)
        return "{" + ",".join(members) + "}"

    return encode(packet, path).encode("utf-8")


class PairJoinWireBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.recording = fixtures.recording()
        cls.producer = fixtures.produce_packet(
            cls.recording.raw_jsonl, cls.recording.manifest, fixtures.SOURCE,
            enabled=True,
        )
        cls.plane = fixtures.plane_record(cls.producer["selected_event"])

    def join_wire(self, payload):
        # The audited recorder inputs and derived producer stay unchanged.
        return join_pair_v2(
            self.recording.raw_jsonl, self.recording.manifest, fixtures.SOURCE,
            self.producer, payload, enabled=True,
        )

    def test_exact_raw_byte_boundary_preserves_the_valid_join(self):
        compact = fixtures.raw(self.plane)
        self.assertEqual(MAX_PACKET_BYTES, 4096)
        self.assertLess(len(compact), MAX_PACKET_BYTES)
        expected = self.join_wire(compact)
        exact = compact + b" " * (MAX_PACKET_BYTES - len(compact))
        self.assertEqual(len(exact), 4096)
        self.assertEqual(json.loads(exact), self.plane)
        result = self.join_wire(exact)
        self.assertEqual(result, expected)
        self.assertEqual(result.producer_status, self.producer["status"])
        for name, value in self.producer["selected_event"].items():
            with self.subTest(identity=name):
                self.assertEqual(getattr(result, name), value)
        self.assertIs(result.native_coverage_proved, False)

        overflow = exact + b" "
        self.assertEqual(len(overflow), 4097)
        self.assertEqual(json.loads(overflow), self.plane)
        with self.assertRaisesRegex(ValueError, "byte bound"):
            self.join_wire(overflow)

    def test_nested_duplicate_members_reject_at_raw_parsing(self):
        # All eight nested objects are checked with equal and conflicting
        # values. Equal duplicates preserve otherwise valid packet semantics.
        cases = [
            (("host_map",), "physical_address", 0xA118E),
            (("witness",), "bit", 64),
            (("before",), "planes", "01000000"),
            (("after",), "planes", "00200001"),
        ]
        for phase in ("before", "after"):
            for copy in ("state_before_copy", "state_after_copy"):
                cases.append(((phase, copy), "latch", "12345679"))
        self.assertEqual(len(cases), 8)

        # Establish that these unchanged audited inputs form a valid pair.
        control = self.join_wire(fixtures.raw(self.plane))
        self.assertIs(control.native_coverage_proved, False)
        for path, name, conflict in cases:
            target = self.plane
            for part in path:
                target = target[part]
            original = target[name]
            self.assertNotEqual(original, conflict)
            for kind, value in (("equal", original), ("conflicting", conflict)):
                with self.subTest(path=path, member=name, kind=kind):
                    payload = duplicate_member(self.plane, path, name, value)
                    self.assertLess(len(payload), MAX_PACKET_BYTES)
                    decoded = json.loads(payload)
                    if kind == "equal":
                        self.assertEqual(decoded, self.plane)
                    else:
                        self.assertNotEqual(decoded, self.plane)
                    with self.assertRaisesRegex(
                            ValueError, "duplicate packet JSON key"):
                        self.join_wire(payload)


if __name__ == "__main__":
    unittest.main()
