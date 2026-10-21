"""price_row_closures must treat DECLARED boundary endpoints as terminals.

Synthetic image only (no owner media): a gateway body at 0899:0000 far-calls the
declared CRT boundary 0A40:02FD, the declared SYSTEM boundary 0AF8:16BC, an
uncovered SYSTEM entry 0AF8:0567, the driver at 08AE:00DC and the DOS unit at
0A32:000B (an "other" far target that must be recorded with its caller). The boundary bodies
behind 0A40:02FD hold code that must never be walked or priced.
"""
import unittest
import unittest.mock

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

import price_row_closures as P


def far(seg, off):
    return bytes([0x9A, off & 0xFF, off >> 8, seg & 0xFF, seg >> 8])


class DeclaredBoundaryTerminalTest(unittest.TestCase):
    def pricer(self):
        image = bytearray(1 << 20)
        gateway = far(0x0A40, 0x02FD) + far(0x0AF8, 0x16BC) + far(0x0AF8, 0x0567) + far(0x08AE, 0x00DC) + far(0x0A32, 0x000B) + b"\xCB"
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
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other={}, bad=set(), dispatch=set(),
                   declared=set())
        pricer.resident(0x899, 0x0000, acc)
        self.assertEqual(set(acc["bodies"]), {"RES_0899_0000"})
        self.assertEqual(acc["declared"], {"RES_0A40_02FD", "RES_0AF8_16BC"})
        self.assertNotIn("RES_0A40_0143", acc["bodies"])
        self.assertEqual(acc["unmapped"], {0x0567})
        self.assertEqual(acc["driver"], {0x00DC: {"RES_0899_0000"}})
        self.assertEqual(acc["other"], {(0x0A32, 0x000B): {"RES_0899_0000"}})

    def test_overlay_target_with_declared_readiness_is_recorded_not_walked(self):
        pricer = self.pricer()
        pricer.rows = {"OVL_0001_0000": {"id": "OVL_0001_0000", "bytes": 10, "callee_readiness": {"targets": [
            {"kind": "resident", "id": "RES_0A40_02FD", "readiness": "declared_boundary"}]}}}
        acc = pricer.price("OVL_0001_0000")
        self.assertEqual(acc["bodies"], {"OVL_0001_0000": 10})
        self.assertEqual(acc["declared"], {"RES_0A40_02FD"})



class NeverCreditClassesTest(unittest.TestCase):
    """The pricer honours every never-credit readiness class (VP i20300): members are ready terminals, never priced."""

    def pricer(self, members=frozenset()):
        image = bytearray(1 << 20)
        base = 0x802 * 16
        image[base + 0x0100:base + 0x0104] = b"\xe8\xfd\xfe\xcb"         # 0802:0100 CALL 0000; RETF
        image[base + 0x0000:base + 0x0006] = far(0x0AF8, 0x0567) + b"\xcb"  # 0802:0000 calls an uncovered endpoint
        image[0x8AE * 16 + 0x00DC:0x8AE * 16 + 0x00DD] = b"\xcb"
        pricer = P.Pricer.__new__(P.Pricer)
        pricer.rows, pricer.image, pricer.md = {}, bytes(image), Cs(CS_ARCH_X86, CS_MODE_16)
        pricer.strict, pricer.covered, pricer.cache = set(), set(), {}
        pricer.runtime_residents = frozenset(members)
        pricer.driver_members = frozenset()
        return pricer

    def test_near_called_resident_runtime_member_is_a_terminal(self):
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other={}, bad=set(), dispatch=set(), declared=set())
        self.pricer().resident(0x802, 0x0100, acc)
        self.assertEqual(set(acc["bodies"]), {"RES_0802_0100", "RES_0802_0000"})   # without the class: walked, priced
        self.assertEqual(acc["unmapped"], {0x0567})
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other={}, bad=set(), dispatch=set(), declared=set())
        self.pricer({"RES_0802_0000"}).resident(0x802, 0x0100, acc)
        self.assertEqual(set(acc["bodies"]), {"RES_0802_0100"})
        self.assertEqual(acc["runtime_residents"], {"RES_0802_0000"})
        self.assertEqual(acc["unmapped"], set())

    def test_row_whose_only_unready_callee_is_a_member_prices_as_ready(self):
        pricer = self.pricer({"RES_0802_0000"})
        pricer.rows = {"OVL_0001_0000": {"id": "OVL_0001_0000", "bytes": 10, "callee_readiness": {"targets": [
            {"kind": "resident", "id": "RES_0802_0000", "readiness": "resident_runtime"}]}}}
        acc = pricer.price("OVL_0001_0000")
        self.assertEqual(acc["bodies"], {"OVL_0001_0000": 10})
        self.assertEqual((acc["unmapped"], acc["runtime_residents"]), (set(), set()))

    def test_direct_driver_internal_target_is_not_priced(self):
        # Before t30's fix the pricer walked a direct driver_internal callee and priced it (11 rows, +617..727 B).
        pricer = self.pricer()
        pricer.rows = {"OVL_0001_0000": {"id": "OVL_0001_0000", "bytes": 10, "callee_readiness": {"targets": [
            {"kind": "resident", "id": "RES_08AE_00DC", "readiness": "driver_internal"}]}}}
        self.assertEqual(pricer.price("OVL_0001_0000")["bodies"], {"OVL_0001_0000": 10})

    def test_committed_table_is_loaded_and_pinned(self):
        self.assertEqual(P.load_runtime_residents(), frozenset({"RES_0802_0175"}))  # 0175 joined (par-e)

    def test_real_rows_lose_exactly_the_driver_members_they_used_to_price(self):
        """On the owner media and a fresh export: rows reaching driver_internal members were overpriced by the members
        they walked - directly (expand) or through a non-member's near calls (resident). 073B lost all eight members
        (727 B); 0036_002D lost 137C+13B1 (110 B) reached through the deferred 01A5. Pinned as the delta, which is
        stable; the absolute prices move whenever unrelated bodies are credited (recorded with their base in the doc)."""
        from pathlib import Path
        root = Path(__file__).resolve().parents[2]
        exe, export = root / "resources/DOS/POOLRAD/START.EXE", root / "resources/strict-candidate-queue/queue.json"
        if not exe.is_file() or not export.is_file():
            self.skipTest("owner media or queue export absent")
        import json
        queue = json.loads(export.read_text())
        if P.sq.stale_inputs(queue, root):
            self.skipTest("queue export is stale; regenerate it")
        pricer = P.Pricer(queue, exe.read_bytes())
        old_ok = ("strict", "resident_covered", "runtime_covered", "declared_boundary")
        for row, over in (("OVL_0068_073B", 727), ("OVL_0036_002D", 110)):
            with self.subTest(row):
                fixed = pricer.price(row)
                self.assertFalse(set(fixed["bodies"]) & pricer.driver_members)    # 01A5 (deferred) may still price
                with unittest.mock.patch.object(P, "OK", old_ok), \
                        unittest.mock.patch.object(pricer, "driver_members", frozenset()):
                    before = pricer.price(row)
                self.assertEqual(sum(before["bodies"].values()) - sum(fixed["bodies"].values()), over)

class RealDisplayRowTest(unittest.TestCase):
    """The real 0802:0193 row on the owner media (t30 side ii): 0175 as a member is a terminal, priced at exactly its 30 B."""

    def walk(self, members):
        from pathlib import Path
        exe = Path(P.ROOT) / "resources/DOS/POOLRAD/START.EXE"
        if not exe.is_file():
            self.skipTest("owner media absent")
        pricer = P.Pricer.__new__(P.Pricer)
        pricer.rows, pricer.image = {}, bytes(P.unpack(exe.read_bytes())[0])
        pricer.md, pricer.cache = Cs(CS_ARCH_X86, CS_MODE_16), {}
        pricer.strict = {r["id"] for r in P.csv.DictReader(open(P.ROOT / "docs/re/strict_full_c_addresses.csv"))}
        pricer.covered, pricer.driver_members = set(P.sq.RUNTIME_COVERED_ENDPOINTS), frozenset()
        pricer.runtime_residents = frozenset(members)
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other={}, bad=set(), dispatch=set(), declared=set())
        pricer.resident(0x802, 0x0193, acc)
        return acc

    def test_0175_member_is_a_terminal_in_the_real_0193_walk(self):
        before, after = self.walk(()), self.walk({"RES_0802_0175"})
        # 0040 is strict since its acceptance (par-e), so it is no longer priced on either side; only 0175's own 30 B leave.
        self.assertEqual(before["bodies"], {"RES_0802_0193": 90, "RES_0802_0175": 30, "RES_0802_0105": 112})
        self.assertEqual(after["bodies"], {"RES_0802_0193": 90, "RES_0802_0105": 112})
        self.assertEqual(after["runtime_residents"], {"RES_0802_0175"})
        for key in ("strict", "unmapped", "bad", "declared"):
            self.assertEqual(before[key], after[key], key)
        self.assertEqual(after["strict"], {"RES_0899_0059", "RES_0802_0040"})


if __name__ == "__main__":
    unittest.main()
