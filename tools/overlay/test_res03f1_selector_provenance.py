"""Negative controls for selector argument binding."""
import unittest
from types import SimpleNamespace
from res03f1_selector_provenance import check_setup


def ins(mnemonic, op_str):
    return SimpleNamespace(mnemonic=mnemonic, op_str=op_str)


class SelectorSetupTest(unittest.TestCase):
    def setUp(self):
        self.setup = [ins(*x) for x in [('mov', 'al, 5'), ('push', 'ax'),
                     ('mov', 'al, 2'), ('push', 'ax'), ('lea', 'di, [bp - 7]'),
                     ('push', 'ss'), ('push', 'di'), ('lea', 'di, [bp - 2]'),
                     ('push', 'ss'), ('push', 'di')]]

    def test_direct_and_near_setups(self):
        check_setup(self.setup, 2)
        check_setup(self.setup + [ins('push', 'cs')], 2, near=True)

    def test_register_or_wrong_constant_refused(self):
        for operand in ('al, byte ptr [bp - 5]', 'al, 1'):
            candidate = self.setup.copy()
            candidate[2] = ins('mov', operand)
            with self.assertRaisesRegex(ValueError, 'selector'):
                check_setup(candidate, 2)

    def test_wrong_pointer_or_near_frame_refused(self):
        candidate = self.setup.copy()
        candidate[5] = ins('push', 'ds')
        with self.assertRaisesRegex(ValueError, 'pointer'):
            check_setup(candidate, 2)
        with self.assertRaisesRegex(ValueError, 'PUSH CS'):
            check_setup(self.setup + [ins('push', 'ds')], 2, near=True)
