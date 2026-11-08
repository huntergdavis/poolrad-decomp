"""Controls for the native-entry comparator: a synthetic checkpoint made from the #1558 world, plus its decoders.

The private capture is not required: the test writes checkpoint.{json,ram,vram} from a synthetic 16EC world (the
synthetic layout at load base 1000h), runs the comparator on it, and requires the same outcome the #1558 case has.
A wrong plane interleave, register mapping, return-frame or stub reading would change that outcome."""
import json
import tempfile
import unittest
from pathlib import Path

import res16ec_native_entry as N
import res16ec_port_boundary as B

A = N.A


def write_checkpoint(directory, state, planes):
    m, c, _, _, _, _ = state
    (directory / 'checkpoint.ram').write_bytes(bytes(m))
    vram = bytearray(4 * 65536)
    for p in range(4):
        vram[p:4 * 65536:4] = planes[p]
    (directory / 'checkpoint.vram').write_bytes(bytes(vram))
    regs = dict(eax=c.ax, ebx=c.bx, ecx=c.cx, edx=c.dx, esi=c.si, edi=c.di, ebp=c.bp, esp=c.sp, cs=c.cs, eip=c.ip, ds=c.ds, es=c.es, ss=c.ss, fs=0, gs=0, flags=c.flags)
    (directory / 'checkpoint.json').write_text(json.dumps(dict(schema=1, id='RES_00BA_16EC', arrival=1, instruction=12345, routed_returns=0, routed_stops=0, registers=regs,
                                                                video=dict(mode=2, display_start=0, real_start=0, scan_len=20, width=320, height=400, vmemsize=4 * 65536))))


class SyntheticChecks(unittest.TestCase):
    def test_plane_interleave(self):
        vram = bytes(((4 * a + p) * 7 + p) & 0xFF for a in range(65536) for p in range(4))
        planes = N.planes_from_vram(vram)
        self.assertEqual(len(planes), 4)
        for p in range(4):
            self.assertEqual(planes[p][5], vram[4 * 5 + p])
            self.assertEqual(len(planes[p]), 65536)
        with self.assertRaisesRegex(ValueError, 'shorter'):
            N.planes_from_vram(vram[:1000])

    def test_rebind_and_restore(self):
        before = (A.BASE, A.RETURN_TO, A.OVSEG)
        previous = N.rebind(0x0824, (0x08DE, 0x2193), 0x1234)
        self.assertEqual((A.BASE, A.RETURN_TO, A.OVSEG), (0x0824, (0x08DE, 0x2193), 0x1234))
        N.restore(previous)
        self.assertEqual((A.BASE, A.RETURN_TO, A.OVSEG), before)


@unittest.skipUnless((A.ROOT / 'resources/DOS/POOLRAD/START.EXE').is_file() and (A.ROOT / 'resources/DOS/POOLRAD/GAME.OVR').is_file(),
                     'owner media unavailable')
class SyntheticCheckpoint(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = B.inputs()

    def checkpoint(self, name, **options):
        state = B.build(name, options, self.data)
        planes = [bytearray([0x31 + p * 0x27]) * 65536 for p in range(4)]      # the #1558 world's EGA fill
        directory = Path(tempfile.mkdtemp(prefix='nc3-synthetic-'))
        write_checkpoint(directory, state, planes)
        return directory

    def test_synthetic_cold_box_and_mode5_entries(self):
        with tempfile.TemporaryDirectory(prefix='nc3-lib-') as work:
            library = A.library(work)
            for name, options, expect in (('cold', dict(mode5=False, flag6dda=0, loaded=False), ('OVERLAY', 6)),
                                          ('mode5', dict(mode5=True), ('RETURNED', -1))):
                meta, ram, planes, cpu, hashes = N.load_checkpoint(self.checkpoint(name, **options))
                facts = N.entry_facts(ram, cpu)
                self.assertEqual(facts['load_base'], A.BASE)
                self.assertEqual(facts['return_to'], A.RETURN_TO)
                self.assertEqual(facts['stub_loaded'], options.get('loaded', True))
                result = N.compare_checkpoint(ram, planes, cpu, facts, library, name=name)
                self.assertEqual((result['stop'], result['callee']), expect)
                self.assertEqual(result['selectors']['49F3'] == 5, options.get('mode5', True))

    def test_wrong_register_or_id_is_refused(self):
        directory = self.checkpoint('refuse', mode5=True)
        meta = json.loads((directory / 'checkpoint.json').read_text())
        meta['registers']['eip'] = 0x16ED
        (directory / 'checkpoint.json').write_text(json.dumps(meta))
        with self.assertRaisesRegex(ValueError, 'IP is not 16EC'):
            N.load_checkpoint(directory)
        meta['registers']['eip'] = 0x16EC; meta['id'] = 'RES_00BA_1816'
        (directory / 'checkpoint.json').write_text(json.dumps(meta))
        with self.assertRaisesRegex(ValueError, 'not a RES_00BA_16EC'):
            N.load_checkpoint(directory)


if __name__ == '__main__':
    unittest.main()
