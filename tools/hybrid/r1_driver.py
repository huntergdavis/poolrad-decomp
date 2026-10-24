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
import hashlib
import shutil
import signal
import subprocess
import time


class RouteFailure(RuntimeError):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


def canonical_start(next_state):
    def state(name, gate, key, expected, **extra):
        return dict(name=name, gate=gate, key=key, expect_next=expected,
                    timeout_s=300, max_visits=1, **extra)
    return [state('code', ['CODE'], 'Return', ['menu']),
            state('menu', ['FUNCTION'], 'l', ['slots']),
            state('slots', ['LOAD', 'LODB'], 'a', ['at15_4'],
                  required_any=['WHICH', 'GAME']),
            state('at15_4', ['15,4'], 'Up', ['at14_4'], source='digits'),
            state('at14_4', ['14,4'], None, [next_state], source='digits')]


def matches(record, observation):
    source = record.get('source', 'prompt')
    text = observation.get(source, '').upper()
    if source == 'digits':
        text = text.replace(' ', '')
    return bool(text) and any(word in text for word in record['gate']) and (
        not record.get('required_any') or
        any(word in text for word in record['required_any']))


def automatic_status(text):
    return any(word in text for word in ('LEFT', 'ATTACH', 'ATTACK', 'DYING')) or (
        'BATTLE' in text and not any(word in text for word in ('YES', 'HES', 'NO')))


def single_exit_prompt(text):
    """Accept only the measured single Exit control, never a clipped Next menu."""
    text = text.strip().upper()
    return text if text in ('EXIT', 'EXT', 'EXT.') else ''


class Route:
    def __init__(self, data):
        self.data = data
        records = canonical_start(data['start']) + data['states']
        self.records = {r['name']: r for r in records}
        self.exact_prompt_states = set(data.get('exact_prompt_states', []))
        if self.exact_prompt_states - self.records.keys():
            raise ValueError('unknown exact-prompt state')
        if len(self.records) != len(records):
            raise ValueError('duplicate gate names')
        for r in self.records.values():
            if r['timeout_s'] <= 0 or r['max_visits'] <= 0:
                raise ValueError('gate budgets must be positive')
            if not isinstance(r['gate'], list) or not r['gate']:
                raise ValueError('gate must be a nonempty any-of word list')
            if r['key'] is not None and not isinstance(r['key'], str):
                raise ValueError('key must be an xdotool key name or null')
            for name in r['expect_next']:
                if name not in self.records:
                    raise ValueError('unknown next gate: ' + name)
        if self.records[data['terminal']]['expect_next']:
            raise ValueError('terminal state must have no expected next states')
        self.allowed = ['code']
        self.visits = {}
        self.words = {word for r in records for word in r['gate'] if len(word) >= 3}

    def choose(self, observation):
        if automatic_status(observation.get('prompt', '')):
            return None
        return next((self.records[n] for n in self.allowed
                     if matches(self.records[n], observation) and
                     (n not in self.exact_prompt_states or
                      observation.get('prompt', '').upper().strip(' .') in
                      self.records[n]['gate'])), None)

    def idle(self, observation):
        text = observation.get('prompt', '')
        return (any(self.records[n].get('source') == 'digits' for n in self.allowed)
                or automatic_status(text) or 'LOADING' in text
                or not any(word in text for word in self.words))

    def advance(self, record):
        name = record['name']
        count = self.visits.get(name, 0) + 1
        if count > record['max_visits']:
            raise RouteFailure(10, 'visit budget exhausted: ' + name)
        self.visits[name] = count
        self.allowed = record['expect_next']
        return record['key']


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
        self.final_pixels_sha256 = None

    def log(self, event, **fields):
        row = dict(event=event, elapsed_s=round(time.monotonic()-self.started, 3),
                   wall_ms=time.time_ns()//1000000, **fields)
        print(json.dumps(row), flush=True)
        with (self.run / 'r1-events.jsonl').open('a') as out:
            out.write(json.dumps(row)+'\n')
        if event == 'gate':
            self.events.append(row)

    def command(self, args, **kwargs):
        env = dict(os.environ, OMP_THREAD_LIMIT='1', MAGICK_THREAD_LIMIT='1')
        return subprocess.run(args, check=True, timeout=kwargs.pop('timeout', 30),
                              env=env, **kwargs)

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
                                   *extra], capture_output=True, text=True, timeout=120)
            return ' '.join(result.stdout.upper().split())
        observation = {'prompt': ocr(crop, '7'), 'frame': str(path.relative_to(self.run))}
        if not observation['prompt']:
            single = self.frames / 'single-exit.png'
            self.command(['convert', str(path), '-crop', '110x26+190+562',
                          '-scale', '400%', str(single)])
            fallback = single_exit_prompt(ocr(single, '7'))
            if fallback:
                observation['prompt'] = fallback
                observation['ocr_fallback'] = 'single_exit_crop'
        if any(self.route.records[n].get('source') == 'digits'
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
        idle_limit = int(os.environ.get('COMBAT_IDLE_S', '300'))
        while self.route.allowed:
            now = time.monotonic()
            if now-self.started >= self.total:
                raise RouteFailure(7, 'total route timeout')
            if self.emu.poll() is not None:
                raise RouteFailure(10, 'emulator exited during route')
            timeout = max(self.route.records[n]['timeout_s'] for n in self.route.allowed)
            if now-began >= timeout:
                raise RouteFailure(10, 'gate timeout; expected '+','.join(self.route.allowed))
            time.sleep(2)
            obs = self.observe()
            record = self.route.choose(obs)
            signature = (obs['prompt'], obs.get('digits'))
            self.log('poll', expected=self.route.allowed, **obs)
            if signature != previous:
                previous = signature
                continue
            if not record:
                if self.route.idle(obs) and time.monotonic()-began < idle_limit:
                    continue
                raise RouteFailure(10, 'unknown screen; expected '+','.join(self.route.allowed))
            key = self.route.advance(record)
            self.log('gate', name=record['name'], key=key,
                     wait_s=round(time.monotonic()-began, 3),
                     visit=self.route.visits[record['name']], expected_next=self.route.allowed, **obs)
            if record['name'] == 'at15_4':
                (self.run / 'observer.pin').touch()
                self.log('save_loaded', marker='observer.pin')
            if record['name'] == self.route.data['terminal']:
                shutil.copy2(self.run / obs['frame'], self.run / 'final.png')
                pixels = self.command(['convert', str(self.run/'final.png'),
                                       '-depth', '8', 'rgba:-'], capture_output=True).stdout
                self.final_pixels_sha256 = hashlib.sha256(pixels).hexdigest()
                self.log('final', state=record['name'], frame='final.png',
                         pixels_sha256=self.final_pixels_sha256)
            if key:
                self.key(key)
            began = time.monotonic()
            previous = None

    def close(self):
        if not self.emu:
            return
        self.shutdown = 'already exited'
        if self.emu.poll() is None and self.window:
            try:
                self.key('ctrl+F9')
                self.emu.wait(timeout=10)
                self.shutdown = 'Ctrl+F9'
            except (subprocess.SubprocessError, OSError):
                pass
        if self.emu.poll() is None:
            try:
                os.killpg(self.emu.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            self.shutdown = 'SIGTERM'
            try:
                self.emu.wait(timeout=20)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(self.emu.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                self.emu.wait(timeout=10)
                self.shutdown = 'SIGKILL; observer report may be incomplete'
        self.log('shutdown', method=self.shutdown, launch_rc=self.emu.returncode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--route', type=Path, default=Path(__file__).with_name('routes')/(os.environ.get('WALK_ROUTE', 'r1')+'.json'))
    parser.add_argument('--total', type=int, default=4800)
    args = parser.parse_args()
    driver = Driver(args.run.resolve(), json.loads(args.route.read_text()), args.total)
    success, error, code = False, None, 10
    def interrupted(signum, frame):
        raise RuntimeError('driver interrupted by signal '+str(signum))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    try:
        driver.run_route()
        success = True
    except Exception as exc:
        error = str(exc)
        code = getattr(exc, 'code', 10)
        driver.log('failure', error=error)
    finally:
        driver.close()
        (driver.run / 'r1-summary.json').write_text(json.dumps(dict(
            success=success, error=error, gates=driver.events,
            shutdown=driver.shutdown, visits=driver.route.visits,
            final_pixels_sha256=driver.final_pixels_sha256), indent=2)+'\n')
    return 0 if success else code


if __name__ == '__main__':
    raise SystemExit(main())
