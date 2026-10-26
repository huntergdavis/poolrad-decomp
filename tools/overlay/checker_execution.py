"""Media-host differential execution; dependency selection is conservative and derived."""
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys

CANNOT_RUN = 4
NOT_COVERED = 5
HEAVY = Path(__file__).resolve().parents[3] / 'poolrad-heavy.sh'
PROFILES = 'tools/overlay/checker_execution_profiles.json'
MEDIA = ('resources/DOS/POOLRAD/START.EXE', 'resources/DOS/POOLRAD/GAME.OVR')


def cmake_sources(root):
    """The tracked C source universe, from CMake rather than a maintained row map."""
    text = (root / 'CMakeLists.txt').read_text()
    text = re.sub(r'#[^\n]*', '', text)
    return {Path(s) for s in re.findall(r'(?<![\w/])src/recovered/[\w./-]+\.c\b', text)}


def source_map(root, paths, compiler_commands):
    """Declared SOURCES/literal compiler inputs plus local helper import closure.

    An opaque build gets the CMake source universe, never an empty guessed set.
    This deliberately over-selects rather than silently omitting a dependent.
    """
    universe = cmake_sources(root)
    if not universe:
        raise ValueError('CMake recovered-source universe is empty')
    by_stem = {p.stem: p for p in universe}
    cache = {}
    directories = (root / 'tools/overlay', root / 'tests/recovered')

    literal_paths = {str(p): p for p in universe}
    literal_paths.update({p.name: p for p in universe})
    literal_paths.update(by_stem)

    def node(path):
        path = path.resolve()
        if path in cache:
            return cache[path]
        source = path.read_text()
        tree = ast.parse(source, filename=str(path))
        values = {n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str)}
        direct = {literal_paths[v] for v in values if v in literal_paths}
        opaque = False
        try:
            commands = compiler_commands(tree, root, strict=True)
        except ValueError:
            commands, opaque = [], True
        for command in commands:
            for arg in command:
                if isinstance(arg, str) and arg.endswith('.c'):
                    candidate = Path(arg)
                    if candidate.is_absolute():
                        try:
                            candidate = candidate.relative_to(root)
                        except ValueError:
                            continue
                    if candidate in universe:
                        direct.add(candidate)
        reasons = set()
        if '.glob(' in source and '*.c' in values:
            direct |= universe
            reasons.add(str(path.relative_to(root)) + ': recovered-source glob')
        assignments = {t.id: n.value for n in ast.walk(tree) if isinstance(n, ast.Assign)
                       for t in n.targets if isinstance(t, ast.Name)}
        aliases = {item.asname or item.name for n in ast.walk(tree) if isinstance(n, ast.Import)
                   for item in n.names if any((d / (item.name + '.py')).is_file() for d in directories)}

        def closed_sources(expr, seen=frozenset()):
            if isinstance(expr, ast.Constant):
                return isinstance(expr.value, str) and expr.value in literal_paths
            if isinstance(expr, (ast.List, ast.Tuple, ast.Set)):
                return all(closed_sources(x, seen) for x in expr.elts)
            if isinstance(expr, ast.Starred):
                return closed_sources(expr.value, seen)
            if isinstance(expr, ast.Name) and expr.id in assignments and expr.id not in seen:
                return closed_sources(assignments[expr.id], seen | {expr.id})
            if isinstance(expr, ast.Attribute) and expr.attr == 'SOURCES':
                base = expr.value
                while isinstance(base, ast.Attribute):
                    base = base.value
                return isinstance(base, ast.Name) and base.id in aliases
            if isinstance(expr, ast.BinOp) and isinstance(expr.op, (ast.Add, ast.BitOr)):
                return closed_sources(expr.left, seen) and closed_sources(expr.right, seen)
            if isinstance(expr, ast.Call) and len(expr.args) == 1 and not expr.keywords:
                fn = expr.func
                allowed = isinstance(fn, ast.Name) and fn.id in ('list', 'tuple', 'set', 'sorted')
                allowed |= (isinstance(fn, ast.Attribute) and fn.attr == 'fromkeys'
                            and isinstance(fn.value, ast.Name) and fn.value.id == 'dict')
                return allowed and closed_sources(expr.args[0], seen)
            return False

        declared = 'SOURCES' in assignments and closed_sources(assignments['SOURCES'])
        if values & {'cc', 'gcc', 'clang', 'cmake'} and ((not commands and not declared) or opaque):
            direct |= universe
            reasons.add(str(path.relative_to(root)) + ': opaque compiler source list; CMake superset')
        imports = set()
        for n in ast.walk(tree):
            if isinstance(n, ast.Import):
                imports.update(item.name for item in n.names)
            elif isinstance(n, ast.ImportFrom) and n.module:
                imports.add(n.module)
        targets = set()
        for name in imports:
            if '.' in name:
                continue
            target = next((d / (name + '.py') for d in (path.parent, *directories)
                           if (d / (name + '.py')).is_file()), None)
            if target:
                targets.add(target.resolve())
        cache[path] = direct, reasons, targets
        return cache[path]

    result = {}
    for path in paths:
        pending, visited, sources, reasons = [root / path], set(), set(), set()
        while pending:
            current = pending.pop().resolve()
            if current in visited:
                continue
            visited.add(current)
            direct, notes, dependencies = node(current)
            sources |= direct
            reasons |= notes
            pending.extend(dependencies - visited)
        if not sources:
            sources = set(universe)
            reasons = {str(path) + ': opaque helper build; CMake superset'}
        result[path] = dict(sources=sorted(map(str, sources)), conservative=sorted(reasons))

    return result


def load_profiles(root):
    profiles = json.loads((root / PROFILES).read_text())
    if not isinstance(profiles, dict):
        raise ValueError('execution profile table must be an object')
    for path, profile in profiles.items():
        if not isinstance(profile, dict):
            raise ValueError(f'invalid execution profile: {path}')
        args = profile.get('args')
        if not isinstance(args, list) or any(not isinstance(a, str) for a in args):
            raise ValueError(f'invalid execution arguments: {path}')
        if any(a in ('--compile-only', '--quick', '--allow-incomplete') for a in args):
            raise ValueError(f'non-differential or reduced execution profile: {path}')
        rows = profile.get('rows')
        if rows is not None and (not isinstance(rows, list) or any(not isinstance(r, str) for r in rows)):
            raise ValueError(f'invalid covered-rows list: {path}')
        waived = profile.get('waive_not_covered')
        if waived is not None and (not isinstance(waived, list) or any(not isinstance(r, str) for r in waived)):
            raise ValueError(f'invalid waived-rows list: {path}')
    return profiles


def execute(root, registry, paths, profiles, output=None, *, heavy=HEAVY, timeout=3600, metadata=None):
    """Run one checker at a time; preserve every output and name affected rows."""
    if not paths:
        raise ValueError('no checker selected for execution')
    missing = [str(root / p) for p in MEDIA if not (root / p).is_file()]
    if missing:
        raise ValueError('missing owner media: ' + ', '.join(missing))
    if not heavy.is_file() or not os.access(heavy, os.X_OK):
        raise ValueError(f'heavy-job wrapper unavailable: {heavy}')
    absent = [str(p) for p in paths if str(p) not in profiles]
    if absent:
        raise ValueError('no differential execution profile: ' + ', '.join(absent))
    report = dict(scope='checker differential contracts; no credit or natural-reach claim', checkers=[])
    report.update(metadata or {})
    report['media_sha256'] = {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in MEDIA}
    status = 0
    for path in paths:
        registered = sorted(row for row, owners in registry.items() if path in owners)
        profile = profiles[str(path)]
        declared = profile.get('rows')
        if declared is None:
            # A single-row checker is unambiguous. A multi-row checker MUST say which
            # rows its command executes; defaulting to all is how AGREE attached to
            # rows that never ran.
            if len(registered) > 1:
                raise ValueError(f"multi-row checker needs an explicit 'rows' list: {path}")
            covered = registered
        else:
            covered = sorted(declared)
            unknown = sorted(set(covered) - set(registered))
            if unknown:
                raise ValueError(f'profile rows not registered for {path}: ' + ', '.join(unknown))
        waived = sorted(set(profile.get('waive_not_covered', [])))
        unknown_w = sorted(set(waived) - set(registered))
        if unknown_w:
            raise ValueError(f'waived rows not registered for {path}: ' + ', '.join(unknown_w))
        uncovered = [row for row in registered if row not in covered]
        unwaived = [row for row in uncovered if row not in waived]
        argv = [str(heavy), sys.executable, str(root / path), *profile['args']]
        print('EXECUTE ' + ','.join(registered) + ': ' + str(path), flush=True)
        try:
            process = subprocess.Popen(argv, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                       text=True, start_new_session=True)
            try:
                stdout, stderr = process.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                # This process group was created by this invocation only.
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    process.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.communicate()
                raise
            done = subprocess.CompletedProcess(argv, process.returncode, stdout, stderr)
        except (OSError, subprocess.TimeoutExpired) as exc:
            # Do not kill an unknown descendant or call an incomplete run agreement.
            entry = dict(checker=str(path), rows=registered, command=argv, result='CANNOT_RUN',
                         row_results={row: 'CANNOT_RUN' for row in registered}, error=str(exc))
            status = CANNOT_RUN
        else:
            text = done.stdout + '\n' + done.stderr
            unavailable = (done.returncode < 0 or done.returncode in (2, 3, 4, 75, 124, 126, 127, 137, 143) or
                           any(s in text for s in ('ModuleNotFoundError', 'FileNotFoundError', 'No such file or directory',
                                                  'undefined symbol:', 'build failed', 'compilation terminated')))
            if done.returncode == 0 and done.stdout.strip() and 'CHECKER_BUILD_OK' not in done.stdout.splitlines():
                result = 'AGREE'
            elif unavailable or done.returncode == 0:
                result = 'CANNOT_RUN'; status = CANNOT_RUN
            else:
                result = 'DIFFERENTIAL_FAILED'
                if status != CANNOT_RUN:
                    status = 1
            if result == 'AGREE':
                if unwaived:
                    # The command passed, but a registered row's differential never ran.
                    # NOT-COVERED is the honest result; the run fails unless waived.
                    result = 'NOT_COVERED'
                    status = NOT_COVERED
                row_results = {row: ('AGREE' if row in covered else 'NOT_COVERED')
                               for row in registered}
            else:
                row_results = {row: result for row in registered}
            entry = dict(checker=str(path), rows=registered, command=argv, result=result,
                         row_results=row_results, covered=covered, not_covered=uncovered,
                         waived=waived, returncode=done.returncode, stdout=done.stdout,
                         stderr=done.stderr,
                         sha256=hashlib.sha256((root / path).read_bytes()).hexdigest())
        report['checkers'].append(entry)
        report['exit_code'] = status
        if output:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, indent=2) + '\n')
        if covered:
            print('AGREE ' + ','.join(covered), flush=True)
        if uncovered:
            print('NOT_COVERED ' + ','.join(uncovered), flush=True)
        if entry['result'] not in ('AGREE', 'NOT_COVERED'):
            print(entry['result'] + ' ' + ','.join(registered), flush=True)
        if entry['result'] == 'CANNOT_RUN':
            break
    return status, report
