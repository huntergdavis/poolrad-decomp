"""Compare existing run-037 captures; never launch a native/oracle process."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import sys


def pinned(path, size, digest):
    data = path.read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise SystemExit("private input hash/size mismatch: " + path.name)
    return data


def main():
    if sys.flags.optimize:
        raise SystemExit("optimized Python is not supported by this verifier")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    if "resources" not in run.parts or "resources" not in args.library.resolve().parts:
        raise SystemExit("private captures and library must stay under resources")
    before = pinned(run / "ram-1446.bin", 655360,
        "e8d15be71b5c02eaea8cca6fbfb2d4fe7b3b8a31b4c1f6a5cb07672f7301732e")
    expected = pinned(run / "ram-stub.bin", 655360,
        "16eeb185b0e03b4293085b34ad7090661b8ae19a5b1f9a9e738b3f005a7c6383")
    transcript = pinned(run / "ev-numeric.tsv", 494,
        "4868fecf51abc0d656a8f5f5e8774e9794cd38024c0730e0cacc1af5e704fbc5")
    pinned(run / "POOLRAD/START.EXE", 65231,
        "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd")
    pinned(run / "POOLRAD/GAME.OVR", 206105,
        "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614")
    pinned(run / "POOLRAD/SAVGAMA.DAT", 13137,
        "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1")
    code = before[(0x2a8e << 4) + 0x1446:(0x2a8e << 4) + 0x157f]
    if hashlib.sha256(code).hexdigest() != \
            "cd7d4b58fff401796a53d4779c4c55e5a731215f5eecf16f304a25b4b48a9dad":
        raise SystemExit("loaded insertion code hash mismatch")
    for offset, size, digest in (
        (0x16a8, 20, "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"),
        (0x0dc0, 33, "753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c"),
    ):
        lo = (0x131c << 4) + offset
        if hashlib.sha256(before[lo:lo + size]).hexdigest() != digest:
            raise SystemExit("loaded SYSTEM helper hash mismatch")
    lines = [line.split() for line in transcript.decode().splitlines()]
    header = lines[0]
    rows = {row[0]: {name.lower(): int(value, 16)
                    for name, value in zip(header[1:], row[1:])}
            for row in lines[1:]}
    fields = "ax bx cx dx si di bp sp cs ds es ss fs gs ip flags".split()

    class CPU(C.Structure):
        _fields_ = [(name, C.c_uint16) for name in fields]

    class State(C.Structure):
        _fields_ = [("cpu", CPU), ("ram", C.POINTER(C.c_uint8)),
                    ("ram_size", C.c_size_t)]

    ram = (C.c_uint8 * (1 << 20))()
    C.memmove(ram, before, len(before))
    state = State(CPU(**rows["insert_entry"]), ram, len(ram))
    library = C.CDLL(str(args.library.resolve()))
    step = library.pr_u6f_1446_step
    step.argtypes = [C.POINTER(State), C.c_void_p, C.c_void_p,
                     C.c_void_p, C.c_size_t]
    step.restype = C.c_int
    for steps in range(10000):
        if (state.cpu.cs, state.cpu.ip) == (0x08d0, 0x25):
            break
        status = step(C.byref(state), None, None, None, 0)
        if status != 0:
            raise SystemExit("unexpected prefix boundary status " + str(status))
    else:
        raise SystemExit("external prefix observation budget exhausted")
    actual = bytes(ram)
    cpu_diff = [name for name in fields
                if getattr(state.cpu, name) != rows["cold_stub"][name]]
    ram_diff = sum(a != b for a, b in zip(actual, expected))
    tail_changed = sum(value != 0 for value in actual[655360:])
    result = dict(boundary="1446 entry to cold stub, before dispatcher",
                  steps=steps, cpu_fields=16, cpu_differences=cpu_diff,
                  captured_bytes=655360, ram_differences=ram_diff,
                  initialized_tail_changed=tail_changed,
                  output_sha256=hashlib.sha256(actual[:655360]).hexdigest(),
                  new_native_run=False, cold_handler_validated=False)
    print(json.dumps(result, sort_keys=True))
    if cpu_diff or ram_diff or tail_changed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
