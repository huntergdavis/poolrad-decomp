#!/usr/bin/env python3
"""Pin ignored inputs and native code/CPU provenance; print no media."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 6:
    raise SystemExit("usage: audit_slot_a_restore_view_join.py GAME.OVR SAVE GEO_DAX VIEW_RAM CPU_TSV")

identities = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (13137, "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"),
    (2572, "2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595"),
    (655360, "31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3"),
    (None, "4c7f2193e77befaf44464fd3f427bc96bf60d49d67f41b195af26a5112a899a8"),
)
inputs = [Path(arg).read_bytes() for arg in sys.argv[1:]]
for data, (size, digest) in zip(inputs, identities):
    if (size is not None and len(data) != size) or sha256(data).hexdigest() != digest:
        raise SystemExit("private input identity mismatch")

game, _, _, ram, cpu = inputs
original = game[0x1DFE6 + 0x18DF:0x1DFE6 + 0x18EA]
loaded = ram[(0x2A8E << 4) + 0x18DF:(0x2A8E << 4) + 0x18EA]
if sha256(original).hexdigest() != "d0cbac4e6333d378dfbf3dd28383b3415e4c09038e9d03027a9fd9681d4ef63f":
    raise SystemExit("close setup/call span mismatch")
if (len(loaded) != 11 or loaded[:9] != original[:9] or
        int.from_bytes(loaded[9:], "little") != 0x131C):
    raise SystemExit("loaded close call relocation mismatch")
if (original[6] != 0x9A or int.from_bytes(original[7:9], "little") != 0x1491 or
        int.from_bytes(original[9:], "little") != 0x0AF8):
    raise SystemExit("original close call target mismatch")

queued = game[0x1DFE6 + 0x1965:0x1DFE6 + 0x196A]
if sha256(queued).hexdigest() != "7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae":
    raise SystemExit("queued party string call mismatch")

rows = {parts[0]: tuple(int(value, 16) for value in parts[1:])
        for parts in (line.split() for line in cpu.decode("ascii").splitlines())}
if rows["3382"] != (0x3582, 0x3382, 0x14A0, 0x14A0, 0x1D16, 0x3EF6,
                     0x3EF8, 0, 0x3ED4, 0, 0x009A, 0x3DAE, 0x84CA, 0x7297):
    raise SystemExit("first-view caller CPU provenance mismatch")
print("hash-locked save/map/native CPU boundary passed; close original/loaded span verified")
print("loaded close setup/call SHA-256=" + sha256(loaded).hexdigest())
