#!/usr/bin/env python3
"""Implementation of run_media_checks.sh; do not bypass its heavy-job wrapper."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    missing = [name for name in ("START.EXE", "GAME.OVR")
               if not (ROOT / "resources/DOS/POOLRAD" / name).is_file()]
    if missing:
        print("media checks NOT RUN: missing " + ", ".join(missing), file=sys.stderr)
        return 4
    # Discover the whole overlay suite so new media cases in it cannot be omitted.
    # The recovered capture-gate suite also reads original overlay bytes; its
    # captures are synthetic and this does not establish native execution.
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    import json
    import price_candidates
    import strict_candidate_queue
    queue = Path(price_candidates.DEFAULT_QUEUE)
    products = ROOT / "resources/function-accounting-current"
    missing_products = [name for name in ("inventory.json", "callgraph.json", "near.json")
                        if not (products / name).is_file()]
    if missing_products:
        print("media checks NOT RUN: missing accounting products: " +
              ", ".join(missing_products), file=sys.stderr)
        return 4
    if not queue.is_file() or strict_candidate_queue.stale_inputs(json.loads(queue.read_text()), ROOT):
        price_candidates.generate_queue(queue)
    suite = unittest.TestSuite()
    for directory, pattern in (("tools/overlay", "test_*.py"),
                               ("tests/recovered", "test_u6a_0824_fullc_gate.py")):
        loader = unittest.TestLoader()
        suite.addTests(loader.discover(str(ROOT / directory), pattern=pattern))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        print("media checks FAILED: see test failures/errors", file=sys.stderr)
        return 1
    if result.skipped:
        print("media checks INCOMPLETE: skipped coverage is not a pass", file=sys.stderr)
        for test, reason in result.skipped:
            print(f"  {test}: {reason}", file=sys.stderr)
        return 4
    print(f"media checks PASS: {result.testsRun} tests, zero skips")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
