"""Synthetic checks for the bounded U19 EGA MOV CPU/RAM capture contract."""

from copy import deepcopy
import unittest

from tools.replay.cold_start_codeword_row14_cpu_store_gate import (
    RAM_BYTES, validate_pair,
)


def cpu(ip, dl=0xA5):
    return {
        "ax": 0x1234, "bx": 0x5678, "cx": 0x0003, "dx": 0x1200 | dl,
        "si": 0x0100, "di": 0x118F, "bp": 0x0200, "sp": 0x0300,
        "cs": 0x0F2D, "ds": 0x14A0, "es": 0xA000, "ss": 0x14A0,
        "ip": ip, "flags": 0x0202,
    }


def pair(site, successor, dl=0xA5):
    before = cpu(site, dl)
    after = deepcopy(before)
    after["ip"] = successor
    after["flags"] ^= 0x0100  # Debugger trap flag is not a MOV result.
    return {"sequence": 1, "pc": f"0F2D:{site:04X}",
            "next_pc": f"0F2D:{successor:04X}",
            "before": {"cpu": before}, "after": {"cpu": after}}


class CpuStoreGateTests(unittest.TestCase):
    def setUp(self):
        self.ram = bytes(RAM_BYTES)

    def test_glyph_dl_and_zero_immediate_are_distinct_source_operands(self):
        glyph = validate_pair(pair(0x0412, 0x0415, 0xA5),
                              self.ram, self.ram)
        zero = validate_pair(pair(0x0445, 0x0449, 0xA5),
                             self.ram, self.ram)
        self.assertEqual((glyph.pc, glyph.source_byte),
                         ("0F2D:0412", 0xA5))
        self.assertEqual((zero.pc, zero.source_byte),
                         ("0F2D:0445", 0))

    def test_wrong_witness_or_cpu_side_effect_is_rejected(self):
        sample = pair(0x0438, 0x043B)
        sample["before"]["cpu"]["di"] = 0x1190
        with self.assertRaisesRegex(ValueError, "ES:DI"):
            validate_pair(sample, self.ram, self.ram)
        sample = pair(0x0438, 0x043B)
        sample["after"]["cpu"]["ax"] ^= 1
        with self.assertRaisesRegex(ValueError, "CPU register"):
            validate_pair(sample, self.ram, self.ram)
        sample = pair(0x0438, 0x043B)
        sample["after"]["cpu"]["flags"] ^= 1
        with self.assertRaisesRegex(ValueError, "status flag"):
            validate_pair(sample, self.ram, self.ram)

    def test_ram_change_and_unpinned_step_are_rejected(self):
        changed = bytearray(self.ram)
        changed[0x1234] = 1
        with self.assertRaisesRegex(ValueError, "conventional RAM"):
            validate_pair(pair(0x0451, 0x0455), self.ram, bytes(changed))
        with self.assertRaisesRegex(ValueError, "successor"):
            validate_pair(pair(0x0451, 0x0456), self.ram, self.ram)
        incomplete = pair(0x0451, 0x0455)
        del incomplete["before"]["cpu"]["ss"]
        with self.assertRaisesRegex(ValueError, "complete 16-bit"):
            validate_pair(incomplete, self.ram, self.ram)


if __name__ == "__main__":
    unittest.main()
