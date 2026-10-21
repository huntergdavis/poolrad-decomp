"""price_row_closures must treat DECLARED boundary endpoints as terminals.

Synthetic image only (no owner media): a gateway body at 0899:0000 far-calls the
declared CRT boundary 0A40:02FD, the declared SYSTEM boundary 0AF8:16BC, an
uncovered SYSTEM entry 0AF8:0567 and the driver at 08AE:00DC. The boundary bodies
behind 0A40:02FD hold code that must never be walked or priced.
"""
import unittest

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

import price_row_closures as P


def far(seg, off):
    return bytes([0x9A, off & 0xFF, off >> 8, seg & 0xFF, seg >> 8])


class DeclaredBoundaryTerminalTest(unittest.TestCase):
    def pricer(self):
        image = bytearray(1 << 20)
        gateway = far(0x0A40, 0x02FD) + far(0x0AF8, 0x16BC) + far(0x0AF8, 0x0567) + far(0x08AE, 0x00DC) + b"\xCB"
        image[0x899 * 16:0x899 * 16 + len(gateway)] = gateway
        # Behind the declared boundary: a body that would add 0A40:0143 if it were walked.
        behind = far(0x0A40, 0x0143) + b"\xCB"
        image[0xA40 * 16 + 0x2FD:0xA40 * 16 + 0x2FD + len(behind)] = behind
        image[0xA40 * 16 + 0x143] = 0xCB
        pricer = P.Pricer.__new__(P.Pricer)
        pricer.rows, pricer.image, pricer.md = {}, bytes(image), Cs(CS_ARCH_X86, CS_MODE_16)
        pricer.strict, pricer.covered, pricer.cache = set(), set(), {}
        return pricer

    def test_declared_boundaries_are_terminals_not_priced(self):
        pricer = self.pricer()
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other=set(), bad=set(), dispatch=set(),
                   declared=set())
        pricer.resident(0x899, 0x0000, acc)
        self.assertEqual(set(acc["bodies"]), {"RES_0899_0000"})
        self.assertEqual(acc["declared"], {"RES_0A40_02FD", "RES_0AF8_16BC"})
        self.assertNotIn("RES_0A40_0143", acc["bodies"])
        self.assertEqual(acc["unmapped"], {0x0567})
        self.assertEqual(acc["driver"], {0x00DC: {"RES_0899_0000"}})

    def test_overlay_target_with_declared_readiness_is_recorded_not_walked(self):
        pricer = self.pricer()
        pricer.rows = {"OVL_0001_0000": {"id": "OVL_0001_0000", "bytes": 10, "callee_readiness": {"targets": [
            {"kind": "resident", "id": "RES_0A40_02FD", "readiness": "declared_boundary"}]}}}
        acc = pricer.price("OVL_0001_0000")
        self.assertEqual(acc["bodies"], {"OVL_0001_0000": 10})
        self.assertEqual(acc["declared"], {"RES_0A40_02FD"})


if __name__ == "__main__":
    unittest.main()
