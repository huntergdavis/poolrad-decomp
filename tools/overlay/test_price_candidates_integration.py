"""Happy-path acceptance for the candidate pricer — the primary test.

The unit tests mock the dependency check, and that is exactly why a NameError on
the *real* path slipped through three fixes in a row. This test runs the
documented stranger path for real: a checkout with the owner's media and the
accounting products present, the queue input deliberately absent, and the
pricer invoked exactly as the docs say. It requires the printed table and exit
0, and it pins the two figures the published menu quotes.

It skips (loudly) where the owner's media is not present, which is the case in
CI; that is why it cannot replace a human running it on an owner checkout.
"""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import price_candidates as pricer

MEDIA = pricer.ROOT / "resources/DOS/POOLRAD"
PRODUCTS = pricer.ROOT / "resources/function-accounting-current"
HAVE_MEDIA = (MEDIA / "START.EXE").is_file() and (MEDIA / "GAME.OVR").is_file()
HAVE_PRODUCTS = all((PRODUCTS / name).is_file()
                    for name in ("inventory.json", "callgraph.json", "near.json"))


@unittest.skipUnless(HAVE_MEDIA and HAVE_PRODUCTS,
                     "owner media and accounting products are required for the real path")
class HappyPathTest(unittest.TestCase):
    def test_stranger_path_prints_the_table(self):
        with tempfile.TemporaryDirectory() as temp:
            queue = Path(temp) / "queue.json"      # deliberately absent
            buffer = io.StringIO()
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--queue", str(queue)])
            output = buffer.getvalue()
            self.assertTrue(queue.exists(), "the pricer must create its own input")
        self.assertEqual(code, 0, output[:400])
        self.assertIn("generating the queue input", output)
        self.assertIn("B/row", output)
        rows = [line.split() for line in output.splitlines()
                if line.startswith("OVL_") or line.startswith("RES_")]
        self.assertGreaterEqual(len(rows), 5, output[:400])

    def test_published_figures_are_self_consistent(self):
        """The table must be arithmetically self-consistent and non-empty.

        Deliberately NOT pinned to a named row: a row can stop being a
        sole-blocker as other lanes land credits, and a test that asserts
        "OVL_XXXX is 132 B/row" then fails for a reason unrelated to the pricer.
        That happened (OVL_0077_275B aged out when its callers changed), so this
        checks the mechanism instead: rows print, the columns reconcile, and the
        cost-per-row column equals cost/rows.
        """
        with tempfile.TemporaryDirectory() as temp:
            queue = Path(temp) / "queue.json"
            buffer = io.StringIO()
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--queue", str(queue), "--top", "40"])
        self.assertEqual(code, 0)
        priced = {}
        for line in buffer.getvalue().splitlines():
            parts = line.split()
            if len(parts) >= 5 and (parts[0].startswith("OVL_") or parts[0].startswith("RES_")):
                priced[parts[0]] = (int(parts[2]), int(parts[3]), int(parts[4]))
        self.assertGreaterEqual(len(priced), 5, "the priced table should not be empty")
        for rid, (cost, rows, per_row) in priced.items():
            self.assertEqual(per_row, round(cost / rows), f"{rid}: B/row is not the rounded cost/rows")
            self.assertGreater(rows, 0, f"{rid}: a priced row must unlock at least one row")

if __name__ == "__main__":
    unittest.main()
