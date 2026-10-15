# U3B0 entry4 `+0D9A`: first SYSTEM-copy call preparation

`src/recovered/overlay_u3b0_entry4_prefix_0d9a.c` executes the
original DOS v1.3 `GAME.OVR` entry4 prefix from `+0D9A` through
`+0DAE` and stops at `IP=+0DAF`, **before** the first SYSTEM
`0AF8:0B25` far call. It is a bounded C prefix, not a complete entry4
translation or a natural gameplay frame.

The pre-implementation `deja "poolrad U3B0 entry4 +0D9A to +0DAF
first SYSTEM copy strict C prefix original byte ABI flags"` query
timed out (exit 124) and supplied no reusable claim. The call-target
lead came from the separately reviewed controlled predecessor work at
commit `1dec2e5eadaa5bfc4840aba97426f7157d11b1b3`; this C prefix
was independently checked against its exact original bytes.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Executed prefix | File `[0x2D669,0x2D67E)`; U3B0 `[+0D9A,+0DAF)`; 21 bytes / 11 instructions; SHA-256 `7ecaa950d8a79a4828f9f8106c7c57c890b34cc7c30d0cb39c404936d6cba427` |
| Stop boundary | `+0DAF` begins `9A 25 0B F8 0A`, far call to SYSTEM `0AF8:0B25`; that call is not executed |

The prefix saves BP, allocates `0x18E` bytes of stack locals, loads
the first source far pointer from inherited `SS:[BP+0x2A]`, and
pushes five words for the SYSTEM copy: source segment, source offset,
SS, destination offset `BP-0x2A`, and limit `0x28`. It leaves SP at
`BP-0x198` and preserves the arithmetic flags produced by `SUB SP,
0x18E`. The model does not dereference the source or create copy output.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 prefix `+0D9A..+0DAE` |
| `source_candidates` | `pr_u3b0_entry4_prefix_0d9a` in `src/recovered/overlay_u3b0_entry4_prefix_0d9a.c` |
| `signals` | Exact 21-byte hash, 11 decoded instructions, first far-call opcode at the stop, inherited frame fields and outgoing five-word call ABI |
| `score` | 24/24 original-byte Unicorn cases match full 1 MiB RAM and all 14 CPU words at `+0DAF`; focused strict C11 `-Werror` ASan/UBSan CTest passes |
| `verdict` | `match` for the bounded prefix; `unclear` for natural gameplay reach and full entry4 identity |
| `review_notes` | Controlled source pointer `5000:1000` contains a five-character Pascal string in the oracle, but neither original prefix nor C reads it before the stop. Cases vary SP, pointer words, and incoming flags. No copy output, downstream call, or count credit. |

Reproduce the focused checks:

```sh
cmake -S . -B resources/build-u3b0-0d9a \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0d9a --target poolrad_overlay_u3b0_entry4_prefix_0d9a_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0d9a \
  -R '^poolrad_overlay_u3b0_entry4_prefix_0d9a_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-re-tools \
  python3 tests/recovered/check_overlay_u3b0_entry4_prefix_0d9a.py
```

Original media and sanitizer builds stay under ignored `resources/`.
