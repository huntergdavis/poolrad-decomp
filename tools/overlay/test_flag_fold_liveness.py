"""Synthetic bytes only; no original media. Unit tests for tools/overlay/flag_fold_liveness.py (task t44)."""
import importlib.util
from pathlib import Path
import unittest

TOOL = Path(__file__).resolve().parent / "flag_fold_liveness.py"
SPEC = importlib.util.spec_from_file_location("flag_fold_liveness", TOOL)
L = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(L)


def run(hexbytes, site=0, stops=()):
    return L.liveness(L.decode(bytes.fromhex(hexbytes), 0), site, stops)


class PinnedKillSets(unittest.TestCase):
    def test_mul_kills_only_cf_of_zf_pf(self):
        # sub ax,cx; mul dx; ret: SF and AF survive MULW (FillFlagsNoCFOF) and reach the return.
        r = run("29c8" "f7e2" "c3")
        self.assertEqual({f for f, v in r["flags"].items() if v["observed"]}, {"S", "A"})
        for f in "CZPO":
            self.assertEqual(r["flags"][f]["killed"], ["mul@0002"])
        self.assertEqual(r["verdict"], "EXPOSED")

    def test_inc_keeps_cf(self):
        # cmp ax,bx; inc ax; jb next; ret: CF passes INC, is read by JB and then reaches RET.
        r = run("39d8" "40" "7200" "c3")
        self.assertEqual(r["flags"]["C"]["observed"], ["jcc:jb@0003", "return:@0005"])
        for f in "PAZSO":
            self.assertEqual(r["flags"][f]["killed"], ["inc@0002"])

    def test_logic_writes_all_six(self):
        # sub ax,1; xor ax,ax; ret
        r = run("83e801" "31c0" "c3")
        self.assertEqual(r["verdict"], "DEAD")
        self.assertTrue(all(v["killed"] == ["xor@0003"] for v in r["flags"].values()))

    def test_shift_by_unknown_cl_kills_nothing(self):
        # sub ax,1; shl ax,cl; ret: a zero count writes no flag, so everything reaches RET.
        r = run("83e801" "d3e0" "c3")
        self.assertEqual(r["verdict"], "EXPOSED")
        self.assertTrue(all(v["observed"] == ["return:@0005"] for v in r["flags"].values()))

    def test_shift_by_known_nonzero_cl_kills_all(self):
        # sub ax,1; mov cl,4; shl ax,cl; ret
        self.assertEqual(run("83e801" "b104" "d3e0" "c3")["verdict"], "DEAD")

    def test_shift_by_known_zero_cl_kills_nothing(self):
        # sub ax,1; mov cl,0; shl ax,cl; ret
        self.assertEqual(run("83e801" "b100" "d3e0" "c3")["verdict"], "EXPOSED")

    def test_rotate_writes_only_cf_of(self):
        # sub ax,1; rol ax,1; ret: ROL kills CF/OF; SZAP reach RET.
        r = run("83e801" "d1c0" "c3")
        self.assertEqual({f for f, v in r["flags"].items() if v["observed"]}, {"S", "Z", "A", "P"})


class PathsAndObservers(unittest.TestCase):
    def test_branch_split(self):
        # sub ax,1; jz L; xor ax,ax; ret; L: ret. ZF is read by JZ; on the taken path every flag reaches the second RET.
        r = run("83e801" "7403" "31c0" "c3" "c3")
        self.assertEqual(r["flags"]["Z"]["observed"], ["jcc:je@0003", "return:@0008"])
        self.assertEqual(r["flags"]["C"], dict(killed=["xor@0005"], observed=["return:@0008"]))
        self.assertEqual(r["verdict"], "EXPOSED")

    def test_branch_only_verdict(self):
        # cmp ax,bx; jne L; L: cmp ax,cx; ret: only the JNE sees the first CMP's ZF.
        r = run("39d8" "7500" "39c8" "c3")
        self.assertEqual(r["verdict"], "BRANCH")

    def test_host_stop_before_instruction(self):
        # sub ax,1; mov bx,1; xor ax,ax; ret: dead, unless the model can stop before the MOV.
        code = "83e801" "bb0100" "31c0" "c3"
        self.assertEqual(run(code)["verdict"], "DEAD")
        r = run(code, stops=[0x0003])
        self.assertEqual(r["verdict"], "EXPOSED")
        self.assertTrue(all(v["observed"] == ["stop:@0003"] for v in r["flags"].values()))

    def test_stop_on_the_killer_still_observes(self):
        # The stop is checked BEFORE the instruction's own effect: a stop at the XOR sees SUB's flags.
        self.assertEqual(run("83e801" "31c0" "c3", stops=[0x0003])["verdict"], "EXPOSED")

    def test_calls_and_pushf_observe(self):
        self.assertEqual(run("83e801" "e80000" "31c0" "c3")["flags"]["C"]["observed"], ["call:@0003"])
        self.assertEqual(run("83e801" "9a00000000" "31c0" "c3")["flags"]["C"]["observed"], ["call:@0003"])
        self.assertEqual(run("83e801" "9c" "31c0" "c3")["flags"]["O"]["observed"], ["read:pushf@0003"])

    def test_adc_reads_then_kills(self):
        # sub ax,1; adc bx,0; ret: ADC reads CF and then writes all six.
        r = run("83e801" "83d300" "c3")
        self.assertEqual(r["flags"]["C"], dict(killed=["adc@0003"], observed=["read:adc@0003"]))
        self.assertEqual(r["flags"]["Z"], dict(killed=["adc@0003"], observed=[]))

    def test_leaving_the_span_observes(self):
        self.assertEqual(run("83e801" "eb10")["flags"]["S"]["observed"], ["leaves@0015"])

    def test_closed_loop_without_exit_is_dead(self):
        self.assertEqual(run("83e801" "ebfe")["verdict"], "DEAD")


# codex-poolrad2's synthetic regressions (inbox i21658, from its u3c6 10A7 slice), copied verbatim: (name, hex, base,
# site, host stops, first-exposure witnesses [(address, flags)], verdict). A witness requires each named flag to be
# observed at that address; the set of observed flags must equal the witnesses' union, so a wrong kill fails either way.
CODEX_POOLRAD2 = [
    ('mul_preserves_prior_af_sf', '01d8f7e1e80000cb', 0x0000, 0x0000, [], [(4, ['AF', 'SF'])], 'exposed'),
    ('inc_preserves_carry_before_ja', '01d84077009c', 0x0000, 0x0000, [], [(3, ['CF'])], 'exposed'),
    ('branch_split_taken_path_exposes_carry', '01d840740239c09c', 0x0000, 0x0000, [], [(7, ['CF'])], 'exposed'),
    ('full_overwrite_before_return_dead', '4039c0cb', 0x0000, 0x0000, [], [], 'dead'),
    ('host_stop_before_partial_overwrite', '39c040cb', 0x117d, 0x117d, [4479], [(4479, ['AF', 'CF', 'OF', 'PF', 'SF', 'ZF'])], 'exposed'),
    ('host_stop_before_full_overwrite', '39c031c0cb', 0x11dd, 0x11dd, [4575], [(4575, ['AF', 'CF', 'OF', 'PF', 'SF', 'ZF'])], 'exposed'),
    ('mul_then_full_overwrite_dead', '01d8f7e139c0cb', 0x0000, 0x0000, [], [], 'dead'),
    ('adc_reads_carry_before_overwrite', '01d811d8cb', 0x0000, 0x0000, [], [(2, ['CF'])], 'exposed')
]


class CodexPoolrad2Fixtures(unittest.TestCase):
    def test_fixtures(self):
        for name, hexbytes, base, site, stops, witnesses, verdict in CODEX_POOLRAD2:
            with self.subTest(name):
                r = L.liveness(L.decode(bytes.fromhex(hexbytes), base), site, stops)
                self.assertEqual(r["verdict"], verdict.upper(), name)
                observed = {f for f, v in r["flags"].items() if v["observed"]}
                self.assertEqual(observed, {f[0] for _, flags in witnesses for f in flags}, name)
                for address, flags in witnesses:
                    for f in flags:
                        self.assertTrue(any(o.endswith(f"@{address:04X}") for o in r["flags"][f[0]]["observed"]),
                                        f"{name}: {f} not observed at {address:04X}: {r['flags'][f[0]]['observed']}")


class FailClosed(unittest.TestCase):
    def test_unlisted_instruction_raises(self):
        with self.assertRaises(NotImplementedError):
            run("83e801" "f7e9" "c3")          # IMUL: no pinned rule recorded in the census


if __name__ == "__main__":
    unittest.main()
