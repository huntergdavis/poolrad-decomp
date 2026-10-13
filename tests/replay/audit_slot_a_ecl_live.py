#!/usr/bin/env python3
"""Pin the private same-save +3171 Up state without publishing game bytes."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: audit_slot_a_ecl_live.py GAME.OVR SAVGAMA.DAT UP_RAM")

expected = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (13137, "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"),
    (655360, "f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558"),
)
game, save, ram = [Path(name).read_bytes() for name in sys.argv[1:]]
for data, identity in zip((game, save, ram), expected):
    if (len(data), sha256(data).hexdigest()) != identity:
        raise SystemExit("source, save, or native RAM identity mismatch")

runner = game[0x3EB7:0x3EF9]
loaded = ram[(0x3582 << 4) + 0x3171:(0x3582 << 4) + 0x31B3]
runner_hash = "4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489"
if sha256(runner).hexdigest() != runner_hash or runner != loaded:
    raise SystemExit("original/loaded +3171 runner mismatch")

def word(off):
    p = (0x14A0 << 4) + off
    return int.from_bytes(ram[p:p + 2], "little")

for slot, width, digest in (
    (0x49D2, 0x800, "9890932b9bbe242f176efa7022d769a87e4417df2d7dd3772f970aca82826c86"),
    (0x49D6, 0x800, "cc41cd4e220d1575e980a7c4e3609e42a60736326958d8784bd3626b957fa634"),
    (0x49DA, 0x400, "00893ed7433feac0f1a7182d5eed9b0d6d4cebbc8c73352f30aa7abdf36c3aae"),
    (0x49DE, 0x1E00, "bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8"),
):
    offset, segment = word(slot), word(slot + 2)
    data = ram[(segment << 4) + offset:(segment << 4) + offset + width]
    if sha256(data).hexdigest() != digest:
        raise SystemExit(f"native pointer/span mismatch at DS:{slot:04X}")
    if slot == 0x49DE and data != save[0x1401:0x3201]:
        raise SystemExit("native script differs from the saved transfer")

offset, segment = word(0x5D92), word(0x5D94)
record = ram[(segment << 4) + offset:(segment << 4) + offset + 285]
if (offset, segment) != (8, 0x449A) or sha256(record).hexdigest() != \
        "22efd553e03bcf9b89a35aaba1abaa9163b9811441c590c46206ec8c2c75e474":
    raise SystemExit("active record identity mismatch")
print("hash-locked live +3171 script, windows, record, and code passed")
