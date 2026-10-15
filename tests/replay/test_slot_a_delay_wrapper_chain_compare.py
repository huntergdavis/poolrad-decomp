#!/usr/bin/env python3
"""Synthetic frame checks for composed U1C DELAY and resident returns."""

import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "replay"))
import slot_a_delay_wrapper_chain_compare as chain  # noqa: E402


class DelayWrapperChainTests(unittest.TestCase):
    def setUp(self) -> None:
        self.entry = bytearray(chain.DUMP_SIZE)
        chain.put_word(self.entry, 0x14A0, 0x49ED, 0x9C28)
        self.entry[chain.physical(0x14A0, 0x49E2)] = 1
        chain.put_word(self.entry, 0x14A0, 0x84FC, 2)
        self.entry[chain.physical(0x40, 0)] = 7
        chain.put_dword(self.entry, 0x40, 0x6C, 1000)
        chain.put_word(self.entry, 0x1D16, 0x3EE4, 0x3151)
        chain.put_word(self.entry, 0x1D16, 0x3EE6, 0x3582)
        chain.put_word(self.entry, 0x1D16, 0x3EE8, 0x3EEE)

    def test_reached_full_chain_and_external_ticks(self) -> None:
        with tempfile.TemporaryDirectory(prefix="poolrad-delay-chain-test-") as tmp:
            lib = chain.build_shim(Path(tmp))
            status, ram, cpu, wrapper, counts = chain.run_chain(
                lib, bytes(self.entry), 5)
        self.assertEqual(status, 0)
        self.assertEqual(counts, (225, 451, 225, 450))
        self.assertEqual(chain.word(ram, 0x14A0, 0x49ED), 0x9C29)
        self.assertEqual(chain.dword(ram, 0x40, 0x6C), 1005)
        for offset, value in ((0x3EE2, 0x3EE8), (0x3EDE, 0x272B),
                              (0x3EE0, 0x3582), (0x3EDC, 0x3EE2),
                              (0x3EDA, 225), (0x3ED6, 0x0DD1),
                              (0x3ED8, 0x0F2D), (0x3ED4, 0x02B7)):
            self.assertEqual(chain.word(ram, 0x1D16, offset), value)
        self.assertEqual((wrapper["cs"], wrapper["ip"], wrapper["sp"],
                          wrapper["bp"]), (0x3582, 0x272B, 0x3EE2, 0x3EE2))
        self.assertEqual((cpu["cs"], cpu["ip"], cpu["sp"], cpu["bp"]),
                         (0x3582, 0x3151, 0x3EE8, 0x3EE8))
        self.assertEqual((cpu["ax"], cpu["bx"], cpu["cx"], cpu["dx"],
                          cpu["es"]), (7, 0, 0, 0, 0x40))

    def test_wrong_entry_stops_before_any_write(self) -> None:
        cpu = list(chain.ENTRY_CPU)
        cpu[chain.CPU_FIELDS.index("ip")] = 0x2720
        with tempfile.TemporaryDirectory(prefix="poolrad-delay-chain-test-") as tmp:
            lib = chain.build_shim(Path(tmp))
            status, ram, _, _, counts = chain.run_chain(
                lib, bytes(self.entry), 0, tuple(cpu))
        self.assertEqual(status, 2)
        self.assertEqual(ram, bytes(self.entry))
        self.assertEqual(counts, (0, 0, 0, 0))


if __name__ == "__main__":
    unittest.main()
