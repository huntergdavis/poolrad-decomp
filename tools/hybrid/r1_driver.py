#!/usr/bin/env python3
"""Disposable original/observer R1 route; run through run_r1.sh in private Xvfb.

Startup/OCR geometry and combat controls reuse walk_driver.py. Route records
name their input gate, key, permitted next gates, timeout and visit bound.
Screenshots/OCR and measured timings stay in the ignored oracle run directory.
"""
import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time


def matches(gate, observation):
    text = observation.get(gate.get('source', 'prompt'), '').upper()
    return bool(text) and bool(re.search(gate['pattern'], text)) and not (
        gate.get('exclude') and re.search(gate['exclude'], text))


class Route:
    def __init__(self, data):
        self.data = data
        self.records = {r['name']: r for r in data['records']}
        if len(self.records) != len(data['records']):
            raise ValueError('duplicate gate names')
        for r in self.records.values():
            if r['timeout'] <= 0 or r['max_visits'] <= 0:
                raise ValueError('gate budgets must be positive')
            for name in r['expected_next']:
                if name not in self.records:
                    raise ValueError('unknown next gate: ' + name)
        self.allowed = [data['start']]
        self.visits = {}

    def choose(self, observation):
        return next((self.records[n] for n in self.allowed
                     if matches(self.records[n]['gate'], observation)), None)

    def advance(self, record):
        name = record['name']
        count = self.visits.get(name, 0) + 1
        if count > record['max_visits']:
            raise RuntimeError('visit budget exhausted: ' + name)
        self.visits[name] = count
        self.allowed = record['expected_next']
        key = record['key']
        return key[(count - 1) % len(key)] if isinstance(key, list) else key


class Driver:
    def __init__(self, run, data, total):
        self.run, self.route, self.total = run, Route(data), total
        self.frames = run / 'r1-frames'
        self.frames.mkdir()
        self.started = time.monotonic()
        self.events = []
        self.emu = None
        self.window = ''
        self.frame_number = 0
        self.shutdown = 'not started'

    def log(self, event, **fields):
        row = dict(event=event, elapsed_s=round(time.monotonic()-self.started, 3),
                   wall_ms=time.time_ns()//1000000, **fields)
        print(json.dumps(row), flush=True)
        with (self.run / 'r1-events.jsonl').open('a') as out:
            out.write(json.dumps(row)+'\n')
        if event == 'gate':
            self.events.append(row)

    def command(self, args, **kwargs):
        return subprocess.run(args, check=True, timeout=20, **kwargs)

    def key(self, key):
        self.command(['xdotool', 'windowfocus', '--sync', self.window])
        self.command(['xdotool', 'key', '--delay', '120', key])

    def observe(self):
        self.frame_number += 1
        path = self.frames / f'{self.frame_number:05d}.png'
        crop = self.frames / 'prompt.png'
        self.command(['import', '-window', 'root', str(path)])
        self.command(['convert', str(path), '-crop', '660x26+190+562',
                      '-scale', '200%', str(crop)])
        def ocr(image, mode, extra=()):
            result = self.command(['tesseract', str(image), 'stdout', '--psm', mode,
                                   *extra], capture_output=True, text=True)
            return ' '.join(result.stdout.upper().split())
        observation = {'prompt': ocr(crop, '7'), 'frame': str(path.relative_to(self.run))}
        if any(self.route.records[n]['gate'].get('source') == 'digits'
               for n in self.route.allowed):
            observation['digits'] = ocr(path, '11', ['-c', 'tessedit_char_whitelist=0123456789,NSWE:'])
        return observation

    def run_route(self):
        with (self.run / 'launch.log').open('w') as out:
            self.emu = subprocess.Popen([
                'tools/oracle/launch.sh', str(self.run), '-set', 'cpu core=normal',
                '-set', 'dosbox quit warning=false', '-time-limit', str(self.total+60)],
                stdout=out, stderr=subprocess.STDOUT, start_new_session=True)
        until = time.monotonic()+60
        while time.monotonic() < until:
            windows = subprocess.run(['xdotool', 'search', '--name', 'DOSBox'],
                                     capture_output=True, text=True, timeout=10).stdout.split()
            if windows:
                self.window = windows[-1]
                break
            if self.emu.poll() is not None:
                raise RuntimeError('emulator exited before opening window')
            time.sleep(1)
        if not self.window:
            raise RuntimeError('no DOSBox window within 60 seconds')
        began = time.monotonic()
        previous = None
        while self.route.allowed:
            now = time.monotonic()
            if now-self.started >= self.total:
                raise RuntimeError('total route timeout')
            if self.emu.poll() is not None:
                raise RuntimeError('emulator exited during route')
            timeout = max(self.route.records[n]['timeout'] for n in self.route.allowed)
            if now-began >= timeout:
                raise RuntimeError('gate timeout; expected '+','.join(self.route.allowed))
            time.sleep(2)
            obs = self.observe()
            record = self.route.choose(obs)
            signature = (record['name'], obs.get(record['gate'].get('source', 'prompt'))) if record else None
            self.log('poll', expected=self.route.allowed, **obs)
            if not record or signature != previous:
                previous = signature
                continue
            key = self.route.advance(record)
            self.log('gate', name=record['name'], key=key,
                     wait_s=round(time.monotonic()-began, 3),
                     visit=self.route.visits[record['name']], expected_next=self.route.allowed, **obs)
            if key:
                self.key(key)
            began = time.monotonic()
            previous = None

    def close(self):
        if not self.emu:
            return
        if self.emu.poll() is None and self.window:
            try:
                self.key('ctrl+F9')
                self.emu.wait(timeout=10)
                self.shutdown = 'Ctrl+F9'
            except (subprocess.SubprocessError, OSError):
                pass
        if self.emu.poll() is None:
            os.killpg(self.emu.pid, signal.SIGTERM)
            self.shutdown = 'SIGTERM'
            try:
                self.emu.wait(timeout=20)
            except subprocess.TimeoutExpired:
                os.killpg(self.emu.pid, signal.SIGKILL)
                self.emu.wait(timeout=10)
                self.shutdown = 'SIGKILL; observer report may be incomplete'
        self.log('shutdown', method=self.shutdown, launch_rc=self.emu.returncode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--route', type=Path, default=Path(__file__).with_name('routes')/'r1.json')
    parser.add_argument('--total', type=int, default=2400)
    args = parser.parse_args()
    driver = Driver(args.run.resolve(), json.loads(args.route.read_text()), args.total)
    success, error = False, None
    def interrupted(signum, frame):
        raise RuntimeError('driver interrupted by signal '+str(signum))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    try:
        driver.run_route()
        success = True
    except Exception as exc:
        error = str(exc)
        driver.log('failure', error=error)
    finally:
        driver.close()
        (driver.run / 'r1-summary.json').write_text(json.dumps(dict(
            success=success, error=error, gates=driver.events,
            shutdown=driver.shutdown, visits=driver.route.visits), indent=2)+'\n')
    return 0 if success else 1


if __name__ == '__main__':
    raise SystemExit(main())
