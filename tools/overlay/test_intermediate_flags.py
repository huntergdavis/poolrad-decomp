"""Synthetic controls for intermediate flag observations; no game bytes."""
# NOT_A_ROW_CHECKER: shared harness contract, not a recovered-body credit audit.
import unittest

from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CS, UC_X86_REG_EFLAGS

from intermediate_flags import ARITH, Contract, FlagObservationError, Trace, compare, hook_unicorn


def trace(events, sites=("add", "cmp"), limit=100):
    result = Trace(sites, limit)
    for site, flags in events:
        result.record(site, flags)
    return result


def original(left, right, cs=0):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, 0x10000)
    # Synthetic ADD AX,BX; CMP AX,AX; NOP. Observe BEFORE the overwrite.
    emu.mem_write(0x100, bytes.fromhex("01d839c090"))
    emu.reg_write(UC_X86_REG_AX, left)
    emu.reg_write(UC_X86_REG_BX, right)
    emu.reg_write(UC_X86_REG_CS, cs)
    emu.reg_write(UC_X86_REG_EFLAGS, 0x202)
    result = Trace(("add", "cmp"))
    hook_unicorn(emu, result, {(0, 0x102): "add", (0, 0x104): "cmp"})
    emu.emu_start(0x100, 0x105, count=10)
    return result, emu.reg_read(UC_X86_REG_EFLAGS) & 0xFFFF


class IntermediateFlagsTest(unittest.TestCase):
    def baseline(self):
        corpus = {}
        # Independent expected FLAGS, including carry, auxiliary carry, signed
        # overflow, zero and parity boundaries. All CMP AX,AX exits are identical.
        for left, right, flags in ((0, 0, 0x246), (0xFFFF, 1, 0x257),
                                   (0x7FFF, 1, 0xA96), (0x8000, 0x8000, 0xA47),
                                   (0xF, 1, 0x212)):
            observed, final = original(left, right)
            self.assertEqual(final, 0x246)
            expected = trace((("add", flags), ("cmp", 0x246)))
            self.assertEqual(compare(observed, expected), 2)
            corpus[f"{left:x}+{right:x}"] = observed, expected
        return corpus

    def test_intermediate_mutants_killed_despite_identical_final_flags(self):
        baseline = self.baseline()
        contract = Contract(("add", "cmp"), baseline)
        for site in ("add", "cmp"):
            mutants = {label: (want, trace((e.site, e.flags ^ (ARITH if e.site == site else 0))
                                          for e in got.checked()))
                       for label, (want, got) in baseline.items()}
            control = contract.require_wrong_flags(site, mutants)
            self.assertEqual(len(control["killed"]), 5)
        self.assertEqual(contract.receipt()["coverage"], {"add": 5, "cmp": 5})

    def test_final_only_observation_cannot_certify_overwritten_add(self):
        final = trace((("add", 0x246),), sites=("add",))
        contract = Contract(("add",), {"overwritten": (final, final)})
        with self.assertRaisesRegex(FlagObservationError, "survived"):
            contract.require_wrong_flags("add", {"overwritten": (final, final)})

    def test_each_arithmetic_bit_is_compared(self):
        want = trace((("add", 0x202),))
        for bit in (1, 4, 0x10, 0x40, 0x80, 0x800, 0x200):
            with self.subTest(bit=bit), self.assertRaisesRegex(FlagObservationError, "FLAGS"):
                compare(want, trace((("add", 0x202 ^ bit),)))

    def test_same_physical_address_in_different_cs_does_not_count(self):
        observed, final = original(0xFFFF, 1, cs=1)
        self.assertEqual(final, 0x246)
        self.assertEqual(observed.checked(), ())

    def test_missing_extra_and_reordered_callbacks_fail(self):
        want = trace((("add", 0x202), ("cmp", 0x246)))
        for events in ((("add", 0x202),),
                       (("add", 0x202), ("cmp", 0x246), ("cmp", 0x246)),
                       (("cmp", 0x246), ("add", 0x202))):
            with self.subTest(events=events), self.assertRaises(FlagObservationError):
                compare(want, trace(events))

    def test_callback_errors_are_retained_until_checked(self):
        for events, limit in (((("unknown", 0),), 2),
                              ((("add", 0x10000),), 2),
                              ((("add", 0), ("add", 0)), 1)):
            recorded = trace(events, limit=limit)
            with self.subTest(events=events), self.assertRaises(FlagObservationError):
                recorded.checked()

    def test_empty_corpus_and_unreached_sites_fail(self):
        for corpus in ({}, {"empty": (trace(()), trace(()))},
                       {"partial": (trace((("add", 0),)), trace((("add", 0),)))}):
            with self.subTest(corpus=corpus), self.assertRaises(FlagObservationError):
                Contract(("add", "cmp"), corpus)

    def test_bad_baseline_cannot_qualify_a_mutant(self):
        with self.assertRaises(FlagObservationError):
            Contract(("add",), {"bad": (trace((("add", 0),), ("add",)),
                                           trace((("add", ARITH),), ("add",)))})

    def test_missing_control_cannot_produce_passing_receipt(self):
        with self.assertRaisesRegex(FlagObservationError, "missing wrong-FLAGS"):
            Contract(("add", "cmp"), self.baseline()).receipt()

    def test_empty_mutant_or_changed_oracle_cannot_count(self):
        baseline = self.baseline()
        contract = Contract(("add", "cmp"), baseline)
        with self.assertRaisesRegex(FlagObservationError, "complete baseline"):
            contract.require_wrong_flags("add", {})
        mutant = dict(baseline)
        mutant[next(iter(mutant))] = (trace((("add", 0),)), trace((("add", ARITH),)))
        with self.assertRaisesRegex(FlagObservationError, "changed the original"):
            contract.require_wrong_flags("add", mutant)

    def test_missing_events_and_other_flag_changes_are_not_mutation_kills(self):
        baseline = self.baseline()
        contract = Contract(("add", "cmp"), baseline)
        for events in ((), (("add", 0x246 ^ 0x200), ("cmp", 0x246))):
            mutant = dict(baseline)
            want, _ = mutant["0+0"]
            mutant["0+0"] = want, trace(events)
            with self.subTest(events=events), self.assertRaises(FlagObservationError):
                contract.require_wrong_flags("add", mutant)

    def test_failed_control_rerun_cannot_reuse_old_success(self):
        correct = trace((("add", 0x246),), sites=("add",))
        wrong = trace((("add", 0x246 ^ ARITH),), sites=("add",))
        contract = Contract(("add",), {"zero": (correct, correct)})
        contract.require_wrong_flags("add", {"zero": (correct, wrong)})
        self.assertEqual(contract.receipt()["controls"]["add"]["events"], 1)
        with self.assertRaisesRegex(FlagObservationError, "survived"):
            contract.require_wrong_flags("add", {"zero": (correct, correct)})
        with self.assertRaisesRegex(FlagObservationError, "missing wrong-FLAGS"):
            contract.receipt()


if __name__ == "__main__":
    unittest.main()
