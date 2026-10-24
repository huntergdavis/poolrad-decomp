"""resident_ds_operands.py: the 16EC-closure factor-2 cells reproduce in one command. Skipped without the media."""
import unittest

import resident_ds_operands as R

MEDIA = R.ROOT / 'resources/DOS/POOLRAD'


@unittest.skipUnless((MEDIA / 'START.EXE').exists(), 'game media not present')
class ResidentDsOperandsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = R.load_image()

    def cells(self, seg, start, end):
        return R.ds_cells(R.operands(self.image, seg, start, end))

    def test_16ec_reads_exactly_the_mode_byte_and_6dda(self):
        self.assertEqual(self.cells(0x00BA, 0x16EC, 0x17E3), {0x49F3, 0x6DDA})

    def test_0768_touches_exactly_the_text_cursor(self):
        self.assertEqual(self.cells(0x0709, 0x0768, 0x09E3), {0x5E24, 0x5E25})

    def test_box_and_prompt_touch_no_ds_cell(self):
        self.assertEqual(self.cells(0x03F1, 0x0000, 0x0137), set())
        self.assertEqual(self.cells(0x0709, 0x0BF5, 0x0C3F), set())

    def test_16ec_calls_in_order(self):
        calls = [text for _, text in R.operands(self.image, 0x00BA, 0x16EC, 0x17E3) if 'call' in text]
        self.assertEqual(calls, ['lcall 0xaf8, 0xb25', 'lcall 0x3f1, 0x137', 'call 0x1816', 'lcall 0x709, 0x768',
                                 'lcall 0x3f1, 0', 'lcall 0x3f1, 0x137', 'call 0x1816', 'lcall 0x709, 0x768',
                                 'lcall 0x709, 0xdbe', 'call 0x17e3'])


if __name__ == '__main__':
    unittest.main()
