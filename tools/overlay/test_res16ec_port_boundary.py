"""Owner-media boundary regressions, discovered by run_media_checks.sh."""
import unittest

import res16ec_port_boundary as B


@unittest.skipUnless((B.A.ROOT / 'resources/DOS/POOLRAD/START.EXE').is_file() and
                     (B.A.ROOT / 'resources/DOS/POOLRAD/GAME.OVR').is_file(), 'owner media unavailable')
class PortBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = B.inputs()

    def test_original_path_matrix(self):
        for name, options, box, loaded, null in B.cases():
            with self.subTest(name=name):
                trace, stop = B.measure(B.build(name, options, self.data, null), self.data[3])
                B.validate(trace, stop, box=box, loaded=loaded, null=null)
                self.assertEqual(trace.selectors['49F3'] == 5, options.get('mode5', True))
                if not options.get('mode5', True):
                    self.assertEqual(trace.selectors['6DDA'], options.get('flag6dda', 0))

    def test_cold_stub_is_not_a_loaded_success(self):
        state = B.build('cold-control', dict(mode5=False, loaded=False), self.data)
        trace, stop = B.measure(state, self.data[3])
        self.assertEqual(stop, 'OVERLAY')
        with self.assertRaisesRegex(ValueError, 'tile count'):
            B.validate(trace, stop, box=True)

    def test_style_mutant_reaches_error_branch(self):
        # Deliberately altered guest code, NOT an original-byte parity case.
        # MOV AL,0 at 1778 becomes MOV AL,3. This exercises the actual excluded
        # error path instead of merely injecting a fake trace record.
        state = B.build('style-control', dict(mode5=False), self.data)
        state[0][B.A.physical(B.A.BASE + B.A.SEG16EC, 0x1779)] = 3
        trace, stop = B.measure(state, self.data[3])
        self.assertGreater(trace.sites[0x01C5], 0)
        self.assertEqual(stop, 'EXIT')
        with self.assertRaisesRegex(ValueError, 'box frame'):
            B.validate(trace, stop, box=True)

    def test_flag_mutant_reaches_other_display_branch(self):
        # First tile's MOV AL,0 at 03F1:0032 becomes MOV AL,1. Its next
        # MOV AL,0 still supplies the other flag: only [BP+8]'s byte changes.
        state = B.build('flag-control', dict(mode5=False), self.data)
        state[0][B.A.physical(B.A.BASE + 0x03F1, 0x0033)] = 1
        trace, stop = B.measure(state, self.data[3])
        self.assertEqual(trace.sites[0x0249], 1)
        with self.assertRaisesRegex(ValueError, 'excluded tile call site'):
            B.validate(trace, stop, box=True)

    def test_body_pin_rejects_changes(self):
        image, _ = B.A.Q.load_image()
        window = image[0x00BA * 16 + 0x16EC:0x00BA * 16 + 0x17E3]
        box = image[0x03F1 * 16:0x03F1 * 16 + 0x0137]
        tile = self.data[1][0x015D:0x02AD]
        for index, name in enumerate(B.PINS):
            with self.subTest(body=name):
                bodies = [window, box, tile]
                bad = bytearray(bodies[index])
                bad[0] ^= 1
                bodies[index] = bad
                with self.assertRaisesRegex(ValueError, f'{name} body pin mismatch'):
                    B.pin_bodies(*bodies)

    def test_real_move_alias_breaks_preservation(self):
        baseline = B.alias_state(self.data, False)
        state = B.alias_state(self.data, True)
        # The worlds differ only in the initial Move-object destination pointer.
        allowed = {B.A.physical(state[1].ds, off) for off in range(0x5E4E, 0x5E52)}
        changed = {i for i, (a, b) in enumerate(zip(baseline[0], state[0])) if a != b}
        self.assertTrue(changed)
        self.assertTrue(changed <= allowed)
        trace, stop = B.measure(baseline, self.data[3])
        B.validate(trace, stop, box=True)
        trace, stop = B.measure(state, self.data[3])
        print('Pre-registered alias observation: ' + B.json.dumps(dict(stop=stop, **trace.report()), sort_keys=True), flush=True)
        B.validate_alias(trace, stop, state)
        with self.assertRaisesRegex(ValueError, 'tile count'):
            B.validate(trace, stop, box=True)


if __name__ == '__main__':
    unittest.main()
