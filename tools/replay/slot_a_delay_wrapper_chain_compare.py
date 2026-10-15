#!/usr/bin/env python3
"""Compare C U1C/0709/A40 composition with ignored native return frames."""

from __future__ import annotations

import ctypes
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN_ROOT = ROOT / "resources" / "oracle-runs"
RAM_SIZE = 1 << 20
DUMP_SIZE = 0xA0000
CPU_FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
              "cs", "ip", "ds", "es", "ss", "flags")
ENTRY_CPU = (0x9C3A, 0x3E94, 0, 0x03CF, 0, 0, 0x3EE8, 0x3EE4,
             0x3582, 0x271F, 0x14A0, 0x3AD6, 0x1D16, 0)
WRAPPER_RETURN = {
    "ax": 0x00F8, "bx": 0, "cx": 0, "dx": 0,
    "bp": 0x3EE2, "sp": 0x3EE2, "cs": 0x3582, "ip": 0x272B,
    "ds": 0x14A0, "es": 0x0040, "ss": 0x1D16,
}
U1C_RETURN = {
    **WRAPPER_RETURN,
    "bp": 0x3EE8, "sp": 0x3EE8, "ip": 0x3151,
}
INPUT_HASHES = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
RAM_HASHES = {
    "delay-entry-private.ram": "007c1679b4899123e0b5ad29a274534befbcd8dc76e551c5faac6f64e79660b4",
    "wrapper-entry-private.ram": "1675857cbf3c0a2e729d0fc134ae7799f02e5290946660cabdcf2015835c84af",
    "a40-entry-private.ram": "c088ddf8cbc7cf3321a3afdf8a79e012be5cb996ff19a5d2c0a4fe46fd8a7162",
    "u1c-return-private.ram": "7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a",
}
LOADED = (
    (0x3582, 0x271F, 14, "ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2"),
    (0x0F2D, 0x0DBE, 21, "7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be"),
    (0x1264, 0x029C, 33, "7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e"),
    (0x1264, 0x02BD, 12, "d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea"),
)


def require(ok: bool, label: str) -> None:
    if not ok:
        raise ValueError(label)


def locked(path: Path, digest: str) -> bytes:
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == digest,
            f"{path.name}: SHA-256 mismatch")
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def word(memory: bytes | bytearray, segment: int, offset: int) -> int:
    at = physical(segment, offset)
    return memory[at] | (memory[at + 1] << 8)


def dword(memory: bytes | bytearray, segment: int, offset: int) -> int:
    return word(memory, segment, offset) | (word(memory, segment, offset + 2) << 16)


def put_word(memory: bytearray, segment: int, offset: int, value: int) -> None:
    at = physical(segment, offset)
    memory[at] = value & 0xFF
    memory[at + 1] = (value >> 8) & 0xFF


def put_dword(memory: bytearray, segment: int, offset: int, value: int) -> None:
    put_word(memory, segment, offset, value & 0xFFFF)
    put_word(memory, segment, offset + 2, (value >> 16) & 0xFFFF)


def build_shim(directory: Path) -> ctypes.CDLL:
    library = directory / "slot_a_delay_wrapper_private.so"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "tools/replay/slot_a_delay_wrapper_chain_shim.c"),
        str(ROOT / "src/recovered/overlay_u1c_delay_271f.c"),
        str(ROOT / "src/recovered/resident_0709_delay_wrapper_0dbe.c"),
        str(ROOT / "src/recovered/resident_a40_delay_029c.c"),
        "-o", str(library),
    ], check=True, stdout=subprocess.DEVNULL)
    lib = ctypes.CDLL(str(library))
    lib.pr_slot_a_delay_wrapper_chain_run.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint16), ctypes.POINTER(ctypes.c_uint16),
        ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.pr_slot_a_delay_wrapper_chain_run.restype = ctypes.c_int
    return lib


def run_chain(lib: ctypes.CDLL, entry: bytes, ticks: int,
              entry_cpu: tuple[int, ...] = ENTRY_CPU):
    require(len(entry) == DUMP_SIZE, "native RAM dump size")
    memory = bytearray(entry + bytes(RAM_SIZE - DUMP_SIZE))
    raw = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    cpu = (ctypes.c_uint16 * len(CPU_FIELDS))(*entry_cpu)
    wrapper = (ctypes.c_uint16 * len(CPU_FIELDS))()
    counts = (ctypes.c_uint64 * 4)()
    status = lib.pr_slot_a_delay_wrapper_chain_run(
        raw, RAM_SIZE, cpu, wrapper, ticks, counts)
    return status, bytes(memory[:DUMP_SIZE]), dict(zip(CPU_FIELDS, cpu)), \
        dict(zip(CPU_FIELDS, wrapper)), tuple(counts)


def intervals(left: bytes, right: bytes) -> list[tuple[int, int]]:
    require(len(left) == len(right), "RAM image lengths differ")
    spans: list[tuple[int, int]] = []
    begin = -1
    for index, (a, b) in enumerate(zip(left, right)):
        if a != b and begin < 0:
            begin = index
        elif a == b and begin >= 0:
            spans.append((begin, index))
            begin = -1
    if begin >= 0:
        spans.append((begin, len(left)))
    return spans


def describe(spans: list[tuple[int, int]]) -> str:
    return (", ".join(f"[0x{begin:X},0x{end:X})" for begin, end in spans)
            + f" ({sum(end - begin for begin, end in spans)} bytes)")


def audit(run_dir: Path) -> None:
    require(run_dir.resolve().is_relative_to(RUN_ROOT.resolve()),
            "run must be under ignored resources/oracle-runs")
    for name, digest in INPUT_HASHES.items():
        locked(run_dir / "POOLRAD" / name, digest)
    captures = {name: locked(run_dir / name, digest)
                for name, digest in RAM_HASHES.items()}
    require(all(len(data) == DUMP_SIZE for data in captures.values()),
            "native RAM dump size")
    entry = captures["delay-entry-private.ram"]
    native = captures["u1c-return-private.ram"]
    loaded = captures["a40-entry-private.ram"]
    for segment, offset, size, digest in LOADED:
        start = physical(segment, offset)
        require(hashlib.sha256(loaded[start:start + size]).hexdigest() == digest,
                f"loaded code at {segment:04X}:{offset:04X}")
    require(intervals(entry, captures["wrapper-entry-private.ram"]) ==
            [(0x193ED, 0x193EE), (0x2103E, 0x21040)],
            "native U1C-to-wrapper stage intervals")
    require(intervals(captures["wrapper-entry-private.ram"], loaded) ==
            [(0x21036, 0x2103E)],
            "native wrapper-to-A40 stage intervals")
    tick_delta = (dword(native, 0x40, 0x6C) - dword(entry, 0x40, 0x6C)) & 0xFFFFFFFF
    require((word(entry, 0x14A0, 0x49ED), entry[physical(0x14A0, 0x49E2)],
             tick_delta) == (0x9C28, 4, 17), "native input or tick delta")
    with tempfile.TemporaryDirectory(prefix="poolrad-delay-chain-") as tmp:
        lib = build_shim(Path(tmp))
        raw = run_chain(lib, entry, 0)
        calibrated = run_chain(lib, entry, tick_delta)
    expected_intervals = {
        "raw": [(0x46C, 0x46E), (0x21022, 0x2102B),
                (0x2102C, 0x21034)],
        "timer-calibrated": [(0x21022, 0x2102B),
                             (0x2102C, 0x21034)],
    }
    for label, result in (("raw", raw), ("timer-calibrated", calibrated)):
        status, memory, cpu, wrapper, counts = result
        require(status == 0, f"{label}: first C boundary status {status}")
        residual = intervals(memory, native)
        wrapper_diff = {key: (wrapper[key], value)
                        for key, value in WRAPPER_RETURN.items()
                        if wrapper[key] != value}
        u1c_diff = {key: (cpu[key], value)
                    for key, value in U1C_RETURN.items()
                    if cpu[key] != value}
        pc = word(memory, 0x14A0, 0x49ED)
        require(residual == expected_intervals[label],
                f"{label}: captured RAM residual changed")
        require(not wrapper_diff and not u1c_diff,
                f"{label}: recorded CPU fields differ")
        require(pc == 0x9C29, f"{label}: ECL PC differs")
        require(counts == (900, 242101, 900, 242100),
                f"{label}: C callback counts changed")
        print(f"{label}: RAM {describe(residual)}")
        print(f"{label}: wrapper CPU differences "
              f"{wrapper_diff}")
        print(f"{label}: U1C CPU differences "
              f"{u1c_diff}")
        print(f"{label}: ECL PC=0x{pc:04X}; "
              f"A40 units/BIOS/calibration/spin={counts}")
    print("unmeasured native CPU fields: SI, DI, FLAGS")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: slot_a_delay_wrapper_chain_compare.py RUN_DIR")
    try:
        audit(Path(sys.argv[1]))
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        raise SystemExit(f"delay chain comparison failed: {exc}") from exc
