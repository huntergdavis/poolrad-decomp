#!/usr/bin/env python3
"""The declared AF-after-shift split is exact: it allows AF alone, only after a non-zero shift (no media needed)."""
# NOT_A_ROW_CHECKER: unit test of the shared pinned_shift_flags helper; it checks no registered row
import unittest

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as U

from pinned_shift_flags import AF, LastFlagWriter, multiply_flags, shift_af_split


def run(code, cl=0):
    """Execute `code` at 0:0100 and return the tracker's verdict after the last instruction."""
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 0x10000)
    emu.mem_write(0x100, code)
    emu.reg_write(U.UC_X86_REG_CX, cl)
    writer = LastFlagWriter()
    emu.hook_add(UC_HOOK_CODE, lambda uc, address, size, user: writer.step(uc, address))
    emu.emu_start(0x100, 0x100 + len(code))
    return writer.shift_nonzero


class SplitTest(unittest.TestCase):
    def test_tracker_follows_the_last_flag_writer(self):
        self.assertTrue(run(bytes.fromhex("d1e7")))              # shl di,1
        self.assertTrue(run(bytes.fromhex("d3e0"), cl=3))        # shl ax,cl (cl=3)
        self.assertFalse(run(bytes.fromhex("d3e0"), cl=0))       # shl ax,cl (cl=0): flags untouched
        self.assertTrue(run(bytes.fromhex("d1e7d3e0"), cl=0x20))  # count & 1Fh = 0 keeps the earlier shift
        self.assertFalse(run(bytes.fromhex("d1e701c0")))         # shl di,1 ; add ax,ax
        self.assertTrue(run(bytes.fromhex("d1e789c3")))          # shl di,1 ; mov bx,ax (no flag write)

    def test_split_allows_af_only_after_a_shift(self):
        self.assertIsNone(shift_af_split(0x0213, 0x0203, True))
        self.assertIsNone(shift_af_split(0x0213, 0x0213, True))   # Unicorn already had AF set
        self.assertIsNotNone(shift_af_split(0x0203, 0x0203, True))  # C must SET AF after a shift
        self.assertIsNotNone(shift_af_split(0x0213, 0x0203, False))  # no shift: exact
        self.assertIsNotNone(shift_af_split(0x0217, 0x0203, True))  # any other bit still compared
        self.assertEqual(AF, 0x0010)

    def test_pinned_mulw_bits(self):
        self.assertEqual(multiply_flags(0x0090, 0x0000, 0x0000), 0x0090 | 0x0040 | 0x0004)  # SF/AF kept, ZF, PF even
        self.assertEqual(multiply_flags(0x0000, 0x0001, 0x0000), 0x0841)  # CF=OF=1; ZF from AX alone (AX=0); PF odd
        self.assertEqual(multiply_flags(0x08D5, 0x0003, 0x0001), 0x0010 | 0x0080 | 0x0801)  # ZF from AX only

    def test_tracker_settles_a_word_mul(self):
        emu = Uc(UC_ARCH_X86, UC_MODE_16)
        emu.mem_map(0, 0x10000)
        emu.mem_write(0x100, bytes.fromhex("f7e390"))   # mul bx ; nop
        emu.reg_write(U.UC_X86_REG_AX, 0x1234)
        emu.reg_write(U.UC_X86_REG_BX, 0x0100)
        writer = LastFlagWriter()
        emu.hook_add(UC_HOOK_CODE, lambda uc, address, size, user: writer.step(uc, address))
        emu.emu_start(0x100, 0x103)
        self.assertEqual(writer.rule, "mul")
        self.assertEqual(writer.division, multiply_flags(0x0002, 0x0012, 0x3400))


if __name__ == "__main__":
    unittest.main()
