#!/usr/bin/env python3
"""Capture one signature-verified keyboard-poll arrival after loading slot A.

Run in private Xvfb through run_parity.sh, using the opt-in checkpoint build.
The turn case is a newly captured fixture, not a claim that nobody has ever
executed the same state. A save-game identity is not full machine identity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import time

from r1_driver import Driver


class ParityDriver(Driver):
    def __init__(self, run, case, total):
        data = dict(version=2, name='parity-' + case, start='final', terminal='final',
                    font_text=True, navigation_states={'at15_4': [15, 4], 'final': [15, 4]},
                    states=[dict(name='final', gate=['15,4'], source='digits', key=None,
                                 expect_next=[], timeout_s=300, max_visits=1)])
        super().__init__(run, data, total)
        self.route.records['at15_4'].update(key='Right' if case == 'turn' else None,
                                          expect_next=['final'])
        self.case = case
        self.capture = None

    def log(self, event, **fields):
        super().log(event, **fields)
        if event == 'final':
            self.checkpoint()

    def checkpoint(self):
        prefix = self.run / 'checkpoint'
        prefix.with_suffix('.arm').touch()
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            try:
                capture = json.loads(prefix.with_suffix('.json').read_text())
                status = Path(f'/proc/{capture["pid"]}/stat').read_text().rsplit(')', 1)[1].split()[0]
                if status == 'T':
                    self.capture = capture
                    break
            except (FileNotFoundError, json.JSONDecodeError):
                pass
            time.sleep(0.05)
        else:
            raise RuntimeError('checkpoint did not stop at requested arrival within 30 seconds')
        self.command(['import', '-window', self.window, str(self.run / 'checkpoint.png')])
        pixels = self.command(['convert', str(self.run / 'checkpoint.png'), '-alpha', 'off',
                               '-depth', '8', 'rgb:-'], capture_output=True).stdout
        (self.run / 'checkpoint.rgb').write_bytes(pixels)
        geometry = self.command(['identify', '-format', '%w %h', str(self.run / 'checkpoint.png')],
                                capture_output=True, text=True).stdout
        self.capture['frame'] = dict(format='RGB24', width=int(geometry.split()[0]),
                                    height=int(geometry.split()[1]),
                                    sha256=hashlib.sha256(pixels).hexdigest(),
                                    scope='last presented window while guest stopped; no new-frame fence')
        self.capture['case'] = self.case
        prefix.with_suffix('.json').write_text(json.dumps(self.capture, indent=2) + '\n')
        # The emulator is our disposable child process group. SIGTERM remains
        # pending while stopped; continue it so normal atexit summaries flush.
        os.killpg(self.emu.pid, signal.SIGTERM)
        os.killpg(self.emu.pid, signal.SIGCONT)
        self.emu.wait(timeout=20)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('case', choices=['loaded', 'turn'])
    parser.add_argument('--total', type=int, default=900)
    args = parser.parse_args()
    driver = ParityDriver(args.run.resolve(), args.case, args.total)
    try:
        driver.run_route()
    finally:
        if driver.emu and driver.emu.poll() is None:
            os.killpg(driver.emu.pid, signal.SIGCONT)
        driver.close()


if __name__ == '__main__':
    main()
