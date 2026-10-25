"""Synthetic negative controls for t100's argument-origin measurement."""
from hashlib import sha256
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from res03f1_02e8_identity import argument_setup


class ArgumentOriginTest(unittest.TestCase):
    def fixture(self, scalar=None, tail=None):
        scalar = scalar if scalar is not None else b'\xb0\x01\x50' * 6
        tail = tail if tail is not None else bytes.fromhex('8d7eff 16 57 bf1000 0e 57 9a0b0bf80a')
        code = bytearray(128)
        code[16:19] = b'\x02ab'  # Synthetic Pascal string, not game content.
        code[100 - len(scalar + tail):100] = scalar + tail
        return code

    def test_adjacent_cs_literal_is_identified_without_emitting_its_text(self):
        result = argument_setup(self.fixture(), 100)
        self.assertEqual(result['literal_length'], 2)
        self.assertEqual(result['literal_sha256'], sha256(b'\x02ab').hexdigest())
        self.assertEqual(result['immediate_scalar_low_bytes'], 6)
        self.assertNotIn('literal_text', result)

    def test_ds_source_is_not_mistaken_for_cs_constant(self):
        tail = bytes.fromhex('8d7eff 16 57 bf1000 1e 57 9a0b0bf80a')
        with self.assertRaisesRegex(ValueError, 'CS-literal'):
            argument_setup(self.fixture(tail=tail), 100)

    def test_nonconstant_source_pointer_is_refused(self):
        tail = bytes.fromhex('8d7eff 16 57 8b7eff 0e 57 9a0b0bf80a')
        with self.assertRaisesRegex(ValueError, 'CS-literal'):
            argument_setup(self.fixture(tail=tail), 100)

    def test_frame_coordinate_is_reported_as_such(self):
        scalar = b'\xb0\x01\x50' * 3 + bytes.fromhex('8a46a2 50') + b'\xb0\x01\x50' * 2
        result = argument_setup(self.fixture(scalar=scalar), 100)
        self.assertEqual(result['immediate_scalar_low_bytes'], 5)
        self.assertEqual(result['frame_scalar_low_bytes'], 1)

    def test_truncated_literal_is_refused(self):
        code = self.fixture()
        code[16] = 255
        with self.assertRaisesRegex(ValueError, 'truncated Pascal'):
            argument_setup(code, 100)


if __name__ == '__main__':
    unittest.main()
