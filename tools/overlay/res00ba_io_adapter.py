#!/usr/bin/env python3
"""Reusable adapter: original-byte RAM state -> the RES_00BA pr_rec_* io structs (1509, 15F7, 23B7, 23DB, 2637, 2E03, 2E46, 2E8B).

The C side (res00ba_io_adapter.c) implements every io callback over one 1 MiB A20-off RAM image and records the models'
declared boundaries in call order. Import this module and call build(directory) once; then each function takes a
bytearray `ram` (modified in place), the data segment `ds` and the far pointers the original takes on its stack.

    import res00ba_io_adapter as R
    lib = R.build(tmpdir)
    status, result = R.res_23b7(lib, ram, ds, (off, seg))

Boundaries are returned as a list of tuples: ('A', bytes) GetMem; ('M', src_seg, src_off, bytes) Move (performed in RAM);
('R', seg, off, bytes) FreeMem; ('L', seg, off) StrLoad literal; ('P', parameter) message. Used by
res00ba_rec_models_audit.py; written to be imported by the hybrid glue as well (PM i21978).
"""
# NOT_A_ROW_CHECKER: shared adapter module; the rows are registered by res00ba_rec_models_audit.py
import ctypes, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ('resident_sidecar_remove', 'resident_sidecar_append', 'resident_character_field_zero', 'resident_party_count',
           'resident_character_target_reset', 'resident_character_link_check', 'resident_character_link_flags',
           'resident_character_target_select')
SIZE = 1 << 20
LOG_MAX = 64
u8, u16, sz = ctypes.c_uint8, ctypes.c_uint16, ctypes.c_size_t
P8, P16, PU = ctypes.POINTER(u8), ctypes.POINTER(u16), ctypes.POINTER(ctypes.c_uint)


def build(directory):
    """Compile the adapter with the eight model sources; returns the loaded library with argument types set."""
    so = Path(directory) / 'res00ba_io_adapter.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'tools/overlay/res00ba_io_adapter.c')] + [str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES]
                   + ['-o', str(so)], check=True)
    lib = ctypes.CDLL(str(so))
    lib.adapt_res00ba_1509.argtypes = [P8, u16, u16, u16, u16, u16, sz, P16, PU]
    lib.adapt_res00ba_15f7.argtypes = [P8, u16, u16, u16, u16, u16, u16, u16, sz, P16, PU]
    lib.adapt_res00ba_23b7.argtypes = [P8, u16, u16, u16, P8]
    lib.adapt_res00ba_23db.argtypes = [P8, u16, sz]
    lib.adapt_res00ba_2637.argtypes = [P8, u16, u16, u16, P16]
    lib.adapt_res00ba_2e03.argtypes = [P8, u16, u16, u16, P8]
    lib.adapt_res00ba_2e46.argtypes = [P8, u16, u16, u16, P8]
    lib.adapt_res00ba_2e8b.argtypes = [P8, u16, u16, u16, u16, u16, P16]
    for name in ('1509', '15f7', '23b7', '23db', '2637', '2e03', '2e46', '2e8b'):
        getattr(lib, f'adapt_res00ba_{name}').restype = ctypes.c_int
    return lib


class _Ram:
    """A ctypes view of a bytearray for the call; copied back into the bytearray afterwards."""
    def __init__(self, ram):
        if len(ram) != SIZE:
            raise ValueError('RAM must be exactly 1 MiB')
        self.ram, self.buf = ram, (u8 * SIZE).from_buffer(ram)


def _log(words, n):
    out = []
    for k in range(min(n, LOG_MAX)):
        kind, a, b, c = words[4 * k:4 * k + 4]
        kind = chr(kind)
        out.append({'A': (kind, a), 'M': (kind, a, b, c), 'R': (kind, a, b, c), 'L': (kind, a, b), 'P': (kind, a)}[kind])
    return out


def res_1509(lib, ram, ds, target, character, max_nodes=4096):
    r, log, n = _Ram(ram), (u16 * (4 * LOG_MAX))(), ctypes.c_uint(0)
    status = lib.adapt_res00ba_1509(r.buf, ds, target[0], target[1], character[0], character[1], max_nodes, log, ctypes.byref(n))
    return status, _log(list(log), n.value)


def res_15f7(lib, ram, ds, source, character, block, max_nodes=4096):
    r, log, n = _Ram(ram), (u16 * (4 * LOG_MAX))(), ctypes.c_uint(0)
    status = lib.adapt_res00ba_15f7(r.buf, ds, source[0], source[1], character[0], character[1], block[0], block[1], max_nodes,
                                    log, ctypes.byref(n))
    return status, _log(list(log), n.value)


def res_23b7(lib, ram, ds, character):
    r, out = _Ram(ram), u8(0xEE)
    return lib.adapt_res00ba_23b7(r.buf, ds, character[0], character[1], ctypes.byref(out)), out.value


def res_23db(lib, ram, ds, max_nodes=4096):
    r = _Ram(ram)
    return lib.adapt_res00ba_23db(r.buf, ds, max_nodes)


def res_2637(lib, ram, ds, character, ax_in=0):
    """AX is in/out: the model keeps the caller's AH, as the original does."""
    r, out = _Ram(ram), u16(ax_in)
    return lib.adapt_res00ba_2637(r.buf, ds, character[0], character[1], ctypes.byref(out)), out.value


def res_2e03(lib, ram, ds, character):
    r, out = _Ram(ram), u8(0xEE)
    return lib.adapt_res00ba_2e03(r.buf, ds, character[0], character[1], ctypes.byref(out)), out.value


def res_2e46(lib, ram, ds, character):
    r, out = _Ram(ram), u8(0xEE)
    return lib.adapt_res00ba_2e46(r.buf, ds, character[0], character[1], ctypes.byref(out)), out.value


def res_2e8b(lib, ram, ds, character, output_slot, ax_in=0):
    r, out = _Ram(ram), u16(ax_in)
    return lib.adapt_res00ba_2e8b(r.buf, ds, character[0], character[1], output_slot[0], output_slot[1], ctypes.byref(out)), out.value
