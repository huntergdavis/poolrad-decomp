# Resident INT 3Fh handler prefix at `+02E6`

The loaded handler prefix has a bounded full-state C translation from
entry `12C6:02E6` to the **first** `CALL +0338` at either `+0303` or
`+0314`. It includes all reached stack, segment-register, arithmetic
flag, and saved interrupt-frame effects before that call. It does not
execute `+0338`, the handler tail, the patched stub, or unit 0x9B.
Consequently it adds no full-routine or game-identity count.

Before implementation, both `deja "Poolrad u6F 1446 INT3F 12C6 02E6
0338 cold loader full RAM register composition"` and the shorter
`deja "02E6 0338"` stalled during local index refresh; they were
stopped without a recall result. The merged
[cold-boundary note](U6F_INSERTION_COLD_BOUNDARY.md) and
[native CPU checkpoints](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md)
supplied the candidate addresses and private replay locations. This
audit independently rechecked the executable, loaded bytes, branch
instructions, CPU tuples, and captured RAM.

## Byte and control-flow boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:02E6..0337` | 82 bytes; 42 16-bit instructions; SHA-256 `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c` |
| Loaded `12C6:02E6..0337` | 82 bytes; SHA-256 `5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef` |
| Relocation | Only immediate bytes `+02F2/+02F3` differ; loaded `MOV AX,imm16` selects `0x14A0` on this replay. C reads the loaded immediate from RAM. |

The loaded interrupt vector at physical `0x00FC..0x00FF` points to
`12C6:02E6`. The already-pushed INT frame at `SS:SP` contains return
IP, CS, and FLAGS. The prefix pushes BP and eight registers, installs
BP, loads DS from the relocated immediate, enables interrupts, loads
`ES:BX` from the saved IP/CS words at `SS:[BP+2]`, and pushes the word
at `ES:BX`. It then subtracts two from saved IP at `SS:[BP+2]`.

- If the result is zero, `JNE +0308` is clear and execution stops
  before `CALL +0338` at `+0303`. BP still addresses the pushed BP.
- Otherwise, `+0308` adds six to BP, exchanges the word at
  `SS:[BP-6]` with AX loaded from `SS:[BP]`, writes the exchanged AX
  back to `SS:[BP]`, and stops before the other `CALL +0338` at
  `+0314`. This is the measured saved route.

Both branches require the loader to continue. The visible loader
`+0338..+03E8` spans 177 bytes and calls further resident helpers;
its RAM/CPU behavior is not represented by this prefix API.

## Saved native parity

The ignored run-038 handler entry has `CS:IP=12C6:02E6`, `SS:SP=
1D16:3790`, saved IP `0027`, saved CS `08D0`, and FLAGS `7002`.
Its numeric CPU transcript is 726 bytes, SHA-256
`6dedfa81d1e1b78ba490d7abd64c88e44fa180087d42749d97aaf93ce22f46bb`.
After the prefix, the native pre-loader stop is `12C6:0314`,
`SP=377C`, `BP=3794`, `AX=37AE`, `BX=0027`, `DS=14A0`,
`ES=08D0`, and FLAGS `7212`; the saved IP is `0025`. The C function
reproduces every one of the 14 recorded CPU fields and all 655,360
captured RAM bytes. The entry-to-stop comparison changes 23 bytes in
two contiguous address runs. The private capture does not cover the
upper 384 KiB of the C function's 1 MiB RAM space; the audit seeds
those bytes to zero and does not make a native parity claim there.

Focused synthetic tests cover the measured branch, the `+0303`
zero-result branch, loaded-immediate variation, a segment-offset
stack wrap, and invalid arguments without mutation. The branch that
stops at `+0303` has no native saved-route checkpoint; its behavior
is instruction-derived and synthetic-tested.

Reproduce locally from the isolated checkout using ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_handler_prefix_02e6_audit.py
cmake -S . -B /tmp/poolrad-handler-prefix-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-handler-prefix-build
ctest --test-dir /tmp/poolrad-handler-prefix-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original code,
RAM captures, saves, and game media remain under ignored `resources/`.
