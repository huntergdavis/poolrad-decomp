"""Synthetic host address cases for the bounded U19 plane resolver."""

import unittest

from tools.replay.cold_start_codeword_row14_internal_plane_resolver import HostMap, planar_index


class PlanarIndexTests(unittest.TestCase):
    def base(self, **changes):
        values = dict(handler="uvga", physical_address=0xA118F,
                      page_mask=0xFFFF, bank_read_full=0, memmask=0x1FFFFF,
                      memsize=0x200000, seq_memory_mode=4, gfx_miscellaneous=0)
        values.update(changes)
        return HostMap(**values)

    def test_unmodified_unchained_target(self):
        self.assertEqual(planar_index(self.base()), 0x118F)
        self.assertEqual(planar_index(self.base(handler="uvga_fast")), 0x118F)

    def test_bank_and_compatible_chain4(self):
        self.assertEqual(planar_index(self.base(bank_read_full=0x10000)), 0x1118F)
        self.assertEqual(planar_index(self.base(bank_read_full=0x10000,
                                                compatible_chain4=True)), 0x118F)

    def test_odd_even_generic_differs_from_fast(self):
        state = self.base(physical_address=0xA4001, gfx_miscellaneous=2,
                          seq_memory_mode=0)
        self.assertEqual(planar_index(state), 1)
        self.assertEqual(planar_index(self.base(physical_address=0xA4001,
                                                gfx_miscellaneous=2,
                                                seq_memory_mode=0,
                                                handler="uvga_fast")), 0x4001)

    def test_extended_memory_hobit(self):
        self.assertEqual(planar_index(self.base(physical_address=0xB0001,
                                                gfx_miscellaneous=2,
                                                seq_memory_mode=2,
                                                page_mask=0x1FFFF)), 0x10001)
        self.assertEqual(planar_index(self.base(physical_address=0xB0001,
                                                gfx_miscellaneous=2,
                                                seq_memory_mode=0,
                                                page_mask=0x1FFFF)), 0x10000)

    def test_reject_unsupported_or_out_of_bounds(self):
        with self.assertRaisesRegex(ValueError, "unsupported"):
            planar_index(self.base(handler="chained"))
        with self.assertRaisesRegex(ValueError, "outside"):
            planar_index(self.base(memsize=0x4000))


if __name__ == "__main__":
    unittest.main()
