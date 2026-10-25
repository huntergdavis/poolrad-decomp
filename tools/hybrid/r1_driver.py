#!/usr/bin/env python3
"""Disposable original/observer R1 route; run through run_r1.sh in private Xvfb.

Startup/OCR geometry and combat controls reuse walk_driver.py. Route records
name their input gate, key, permitted next gates, timeout and visit bound.
Screenshots/OCR and measured timings stay in the ignored oracle run directory.
"""
import argparse
import json
import os
import re
from pathlib import Path
import hashlib
import shutil
import signal
import struct
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



def load_coordinate_font(path):
    """Read the documented private 8X8D1 block 201; publish no glyph bytes."""
    archive = path.read_bytes()
    if hashlib.sha256(archive).hexdigest() != 'cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021':
        raise ValueError('coordinate font archive differs from pinned input')
    header = struct.unpack_from('<H', archive)[0]
    for at in range(2, 2 + header, 9):
        ident, offset, size, packed = struct.unpack_from('<BIHH', archive, at)
        if ident != 201:
            continue
        source = archive[2 + header + offset:2 + header + offset + packed]
        font = bytearray()
        at = 0
        while at < len(source):
            control = source[at]
            at += 1
            if control < 128:
                font.extend(source[at:at + control + 1])
                at += control + 1
            else:
                font.extend([source[at]] * (256 - control))
                at += 1
        if size != 1416 or len(font) != size or hashlib.sha256(font).hexdigest() != 'fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1':
            raise ValueError('coordinate font block differs from documented input')
        glyphs = {}
        for ch in map(chr, range(32, 96)):
            glyph = bytes(font[(ord(ch) % 64) * 8:(ord(ch) % 64) * 8 + 8])
            glyphs[glyph] = None if glyph in glyphs else ch
        return {glyph: ch for glyph, ch in glyphs.items() if ch is not None}
    raise ValueError('coordinate font block missing')


# The brightness a non-green glyph pixel must exceed. 64 for R1/R2/R4. A route sets "glyph_bright": 128 when its text sits on
# the EGA gray (85,85,85) combat background (R3), so the gray is not read as glyph pixels.
GLYPH_BRIGHT = 64


def decode_font_cells(rgb, glyphs, columns, green_only=False):
    """Decode only exact, uniformly doubled glyphs from the pinned private font."""
    width = columns * 16
    if len(rgb) != width * 16 * 3:
        return None
    def foreground(x, y):
        at = (y * width + x) * 3
        r, g, b = rgb[at:at + 3]
        return (g > 64 and g > 1.5 * r and g > 1.5 * b) if green_only else max(r, g, b) > GLYPH_BRIGHT
    text = ''
    for cell in range(columns):
        rows = []
        for y in range(8):
            row = 0
            for x in range(8):
                pixels = {foreground(cell * 16 + x * 2 + dx, y * 2 + dy)
                          for dx in range(2) for dy in range(2)}
                if len(pixels) != 1:
                    return None
                row = (row << 1) | pixels.pop()
            rows.append(row)
        char = glyphs.get(bytes(rows))
        if char is None:
            return None
        text += char
    return text


def decode_coordinate(rgb, glyphs):
    text = decode_font_cells(rgb, glyphs, 6, green_only=True)
    if text is None or ' ' not in text:
        return ''
    text = text.split(' ', 1)[0]
    match = re.fullmatch(r'([0-9]{1,2}),([0-9]{1,2})', text)
    return text if match and all(int(v) < 16 for v in match.groups()) else ''


class Route:
    def __init__(self, data):
        self.data = data
        global GLYPH_BRIGHT
        GLYPH_BRIGHT = int(data.get('glyph_bright', 64))
        records = canonical_start(data['start']) + data['states']
        self.records = {r['name']: r for r in records}
        self.actor_gates = data.get('actor_gates', {})
        self.combat_actor_gates = data.get('combat_actor_gates', {})
        self.visual_gates = data.get('visual_gates', {})
        self.message_gates = data.get('message_gates', {})
        if self.message_gates.keys() - self.records.keys():
            raise ValueError('unknown message gate state')
        if any(not isinstance(words, list) or not words or
               any(not isinstance(word, str) or not word for word in words)
               for words in self.message_gates.values()):
            raise ValueError('message gate must be a nonempty word list')
        self.navigation_states = data.get('navigation_states', {})
        if self.navigation_states.keys() - self.records.keys():
            raise ValueError('unknown navigation state')
        for position in self.navigation_states.values():
            if (not isinstance(position, list) or len(position) != 2 or
                    any(type(v) is not int or not 0 <= v < 16 for v in position)):
                raise ValueError('navigation position must be two grid coordinates')
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

    def needs_digits(self):
        return any(n in self.navigation_states or
                   self.records[n].get('source') == 'digits' for n in self.allowed)

    def position_matches(self, name, observation):
        if name not in self.navigation_states:
            return True
        x, y = self.navigation_states[name]
        return re.search(rf'(?<![0-9]){x}\s*,\s*{y}(?![0-9])',
                         observation.get('digits', '')) is not None

    def choose(self, observation):
        if automatic_status(observation.get('prompt', '')):
            return None
        return next((self.records[n] for n in self.allowed
                     if matches(self.records[n], observation) and
                     (n not in self.actor_gates or observation.get('actor') == self.actor_gates[n]) and
                     (n not in self.combat_actor_gates or observation.get('combat_actor') == self.combat_actor_gates[n]) and
                     all(observation.get(field) == value for field, value in self.visual_gates.get(n, {}).items()) and
                     self.position_matches(n, observation) and
                     (n not in self.message_gates or any(
                         word in observation.get('message', '').upper()
                         for word in self.message_gates[n])) and
                     (n not in self.exact_prompt_states or
                      observation.get('prompt', '').upper().strip(' .') in
                      self.records[n]['gate'])), None)

    def idle(self, observation):
        text = observation.get('prompt', '')
        return (any(n in self.visual_gates for n in self.allowed)
                or any(self.records[n].get('source') == 'digits' for n in self.allowed)
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
        self.coordinate_font = (load_coordinate_font(run / 'POOLRAD' / '8X8D1.DAX')
                                if self.route.navigation_states or data.get('font_text') else None)

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
        if self.route.data.get('font_text'):
            rgb = self.command(['convert', str(path), '-crop', '640x16+192+568',
                                '-depth', '8', 'rgb:-'], capture_output=True).stdout
            exact = decode_font_cells(rgb, self.coordinate_font, 40)
            if exact is not None and exact.strip():
                observation['prompt_ocr'] = observation['prompt']
                observation['prompt'] = exact.strip()
                observation['prompt_source'] = 'pinned_font_cells'
        if any(n in self.route.message_gates for n in self.route.allowed):
            message = self.frames / 'message.png'
            self.command(['convert', str(path), '-crop', '620x104+204+452',
                          '-scale', '200%', str(message)])
            observation['message'] = ocr(message, '6')
            if self.route.data.get('font_text'):
                rgb = self.command(['convert', str(path), '-crop', '608x96+208+456',
                                    '-depth', '8', 'rgb:-'], capture_output=True).stdout
                stride = 608 * 16 * 3
                lines = [decode_font_cells(rgb[i * stride:(i + 1) * stride],
                                           self.coordinate_font, 38) for i in range(6)]
                if all(line is not None for line in lines):
                    observation['message_ocr'] = observation['message']
                    observation['message'] = ' '.join(line.strip() for line in lines if line.strip())
                    observation['message_source'] = 'pinned_font_cells'

        if any(n in self.route.navigation_states for n in self.route.allowed):
            rgb = self.command(['convert', str(path), '-crop', '96x16+464+424',
                                '-depth', '8', 'rgb:-'], capture_output=True).stdout
            observation['digits'] = decode_coordinate(rgb, self.coordinate_font)
            observation['digits_source'] = 'pinned_font_cells'
        elif self.route.needs_digits():
            observation['digits'] = ocr(path, '11', ['-c', 'tessedit_char_whitelist=0123456789,NSWE:'])
        if self.route.data.get('actor_gates'):
            rgb = self.command(['convert', str(path), '-crop', '240x96+464+248',
                                '-depth', '8', 'rgb:-'], capture_output=True).stdout
            stride = 240 * 16 * 3
            names = []
            for i in range(6):
                row = rgb[i*stride:(i+1)*stride]
                white = bytearray(len(row))
                for at in range(0, len(row), 3):
                    if min(row[at:at+3]) > 128:
                        white[at:at+3] = row[at:at+3]
                name = decode_font_cells(white, self.coordinate_font, 15)
                if name and name.strip():
                    names.append(name.strip())
            observation['actor'] = names[0] if len(names) == 1 else ''
        if self.route.data.get('combat_cursor'):
            from PIL import Image   # R3's combat/visual gates only; R1/R2/R4 never import it
            im = Image.open(path).convert('RGB')
            found = []
            for y in range(7):
                for x in range(7):
                    xx, yy = 208+48*x, 200+48*y
                    pts = {(xx+i,yy) for i in range(48)} | {(xx+i,yy+47) for i in range(48)} | {(xx,yy+i) for i in range(48)} | {(xx+47,yy+i) for i in range(48)}
                    if sum(min(im.getpixel(pt))>220 for pt in pts)>60:
                        found.append([x,y])
            observation['combat_cursor'] = found[0] if len(found)==1 else None
        if any(n in self.route.combat_actor_gates for n in self.route.allowed):
            from PIL import Image   # R3's combat/visual gates only; R1/R2/R4 never import it
            im = Image.open(path).convert('RGB')
            rgb = im.crop((560,200,816,216)).tobytes()
            rgb = bytes(v if max(rgb[n-n%3:n-n%3+3])-min(rgb[n-n%3:n-n%3+3])>64 else 0 for n,v in enumerate(rgb))
            name = decode_font_cells(rgb, self.coordinate_font, 16)
            observation['combat_actor'] = name.strip() if name else ''
        if any(n in self.route.visual_gates for n in self.route.allowed):
            from PIL import Image   # R3's combat/visual gates only; R1/R2/R4 never import it
            im = Image.open(path).convert('RGB')
            def cells(x, y, columns, white=False):
                rgb = im.crop((x,y,x+16*columns,y+16)).tobytes()
                if white:
                    rgb = bytes(v if min(rgb[n-n%3:n-n%3+3])>128 else 0 for n,v in enumerate(rgb))
                result = decode_font_cells(rgb, self.coordinate_font, columns)
                return result.strip() if result is not None else None
            observation['spell_title'] = cells(208,200,38)
            observation['spell_bar'] = cells(192,552,40)
            rows = [cells(240,y,36) for y in range(280,552,16)]
            white_rows = [cells(240,y,36,True) for y in range(280,552,16)]
            observation['spell_rows'] = [row for row in rows if row]
            observation['selected_spell'] = [row for row in white_rows if row]
            observation['selected_spell_y'] = [280+16*i for i,row in enumerate(white_rows) if row]
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
        self.drive_route()

    def drive_route(self):
        """Run a bounded FSM on this driver's already-open private guest."""
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
            signature = (obs['prompt'], obs.get('digits'), obs.get('message'), obs.get('actor'),
                         obs.get('spell_title'), obs.get('spell_rows'), obs.get('selected_spell'), obs.get('selected_spell_y'), obs.get('combat_actor'), obs.get('spell_bar'), obs.get('combat_cursor'))
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
