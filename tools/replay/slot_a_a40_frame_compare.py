#!/usr/bin/env python3
"""Compare the bounded A40 C body with ignored native entry/return frames.

The BIOS timer advance is an explicit supplied external effect. No native
return bytes or CPU values are copied into the simulated output frame.
"""

from __future__ import annotations

import ctypes
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN_ROOT = ROOT / "resources" / "oracle-runs"
RAM_SIZE = 0xA0000
SS = 0x1D16
ENTRY_SP = 0x3ED6
INPUT_HASHES = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
RAM_HASHES = {
    "a40-entry-private.ram": "c088ddf8cbc7cf3321a3afdf8a79e012be5cb996ff19a5d2c0a4fe46fd8a7162",
    "a40-return-private.ram": "7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a",
}
NATIVE_ENTRY_CPU = {
    "ax": 0x0384, "bx": 0x3E94, "cx": 0, "dx": 0,
    "bp": 0x3EDC, "sp": ENTRY_SP, "cs": 0x1264, "ip": 0x029C,
    "ds": 0x14A0, "es": 0x3AD6, "ss": SS,
}
NATIVE_RETURN_CPU = {
    "ax": 0x00F8, "bx": 0, "cx": 0, "dx": 0,
    "bp": 0x3EDC, "sp": 0x3EDC, "cs": 0x0F2D, "ip": 0x0DD1,
    "ds": 0x14A0, "es": 0x0040, "ss": SS,
}


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
    library = directory / "slot_a_a40_frame_private.so"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "tools/replay/slot_a_a40_frame_shim.c"),
        str(ROOT / "src/recovered/resident_a40_delay_029c.c"),
        "-o", str(library),
    ], check=True, stdout=subprocess.DEVNULL)
    lib = ctypes.CDLL(str(library))
    lib.pr_slot_a_a40_frame_run.argtypes = [
        ctypes.c_uint16, ctypes.c_uint8, ctypes.c_uint16,
        ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.pr_slot_a_a40_frame_run.restype = ctypes.c_int
    return lib


def run_model(lib: ctypes.CDLL, units: int, bios: int,
              calibration: int, budget: int) -> tuple[int, tuple[int, ...]]:
    output = (ctypes.c_uint64 * 10)()
    status = lib.pr_slot_a_a40_frame_run(units, bios, calibration,
                                         budget, output)
    return status, tuple(output)


def timer_advance(memory: bytearray, ticks: int) -> None:
    """External BIOS scheduler callback; ticks are supplied, not predicted."""
    prior = dword(memory, 0x40, 0x6C)
    put_dword(memory, 0x40, 0x6C, (prior + ticks) & 0xFFFFFFFF)


def simulate_frame(entry: bytes, entry_cpu: dict[str, int],
                   report: tuple[int, ...], tick_callback,
                   ticks: int) -> tuple[bytearray, dict[str, int]]:
    requested, completed, _, _, spin_calls, _, baseline, observed, _, cx = report
    require(requested > 0 and completed == requested and spin_calls > 0,
            "A40 model did not complete the nonzero frame")
    sp, ss = entry_cpu["sp"], entry_cpu["ss"]
    require(word(entry, ss, sp + 4) == requested, "A40 argument frame")
    memory = bytearray(entry)
    # +02B4 near CALL writes its return IP at SP-2; all iterations reuse it.
    put_word(memory, ss, sp - 2, 0x02B7)
    tick_callback(memory, ticks)
    cpu = dict(entry_cpu)
    cpu.update(ax=baseline, bx=0, cx=cx,
               dx=(requested - completed) & 0xFFFF,
               sp=(sp + 6) & 0xFFFF,
               cs=word(entry, ss, sp + 2), ip=word(entry, ss, sp),
               es=0x0040, di=0)
    require(observed == baseline, "constant BIOS-byte callback changed")
    return memory, cpu


def intervals(left: bytes | bytearray, right: bytes | bytearray) -> list[tuple[int, int]]:
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


def format_intervals(spans: list[tuple[int, int]]) -> str:
    return ", ".join(f"[0x{start:X},0x{end:X})" for start, end in spans)


def audit(run_dir: Path) -> None:
    require(run_dir.resolve().is_relative_to(RUN_ROOT.resolve()),
            "run must be under ignored resources/oracle-runs")
    for name, digest in INPUT_HASHES.items():
        locked(run_dir / "POOLRAD" / name, digest)
    entry = locked(run_dir / "a40-entry-private.ram",
                   RAM_HASHES["a40-entry-private.ram"])
    native_return = locked(run_dir / "a40-return-private.ram",
                           RAM_HASHES["a40-return-private.ram"])
    require(len(entry) == len(native_return) == RAM_SIZE, "RAM size")
    start = physical(0x1264, 0x029C)
    require(hashlib.sha256(entry[start:start + 45]).hexdigest() ==
            "d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb",
            "A40 loaded-code SHA-256")
    units = word(entry, SS, ENTRY_SP + 4)
    bios = entry[physical(0x40, 0)]
    calibration = word(entry, 0x14A0, 0x84FC)
    tick_delta = (dword(native_return, 0x40, 0x6C)
                  - dword(entry, 0x40, 0x6C)) & 0xFFFFFFFF
    require((units, bios, calibration, tick_delta) == (900, 248, 269, 17),
            "reached input or timer calibration changed")
    with tempfile.TemporaryDirectory(prefix="poolrad-a40-frame-") as tmp:
        lib = build_shim(Path(tmp))
        status, report = run_model(lib, units, bios, calibration, 243001)
    require(status == 0, f"C A40 status {status}")
    no_timer, cpu = simulate_frame(entry, NATIVE_ENTRY_CPU, report,
                                   timer_advance, 0)
    calibrated, _ = simulate_frame(entry, NATIVE_ENTRY_CPU, report,
                                    timer_advance, tick_delta)
    raw_diff = intervals(no_timer, native_return)
    calibrated_diff = intervals(calibrated, native_return)
    cpu_diff = {name: (cpu[name], native)
                for name, native in NATIVE_RETURN_CPU.items()
                if cpu[name] != native}
    print(f"C body: units={report[1]} BIOS reads={report[2]} "
          f"calibration reads={report[3]} spin calls={report[4]}")
    print(f"external timer: supplied native delta={tick_delta} guest ticks")
    print(f"raw RAM differences: {format_intervals(raw_diff)} "
          f"({sum(end - start for start, end in raw_diff)} bytes)")
    print(f"timer-calibrated RAM differences: {format_intervals(calibrated_diff)} "
          f"({sum(end - start for start, end in calibrated_diff)} bytes)")
    print("observed CPU differences:", cpu_diff)
    print("unmeasured native CPU fields: SI, DI, FLAGS")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: slot_a_a40_frame_compare.py RUN_DIR")
    try:
        audit(Path(sys.argv[1]))
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        raise SystemExit(f"A40 frame comparison failed: {exc}") from exc
