# U3B0 entry4 `+0DB4`: second SYSTEM-copy call preparation

`src/recovered/overlay_u3b0_entry4_successor_0db4.c` executes the
original DOS v1.3 `GAME.OVR` entry4 instructions from `+0DB4` through
`+0DC1` and stops at `IP=+0DC2`, **before** the second SYSTEM
`0AF8:0B25` far call. Its input frame comes from the original first
SYSTEM copy returning to `+0DB4`; the oracle does not invent that
copy's output. This is a bounded C transition, not a complete entry4
translation or a natural gameplay frame.

The pre-implementation `deja "poolrad U3B0 entry4 +0DB4 to +0DC2
second SYSTEM copy return-side C prefix original first-copy frame"`
query timed out (exit 124) and supplied no reusable claim. The
predecessor address and copy-body lead came from reviewed controlled
entry4 work at commit `1dec2e5eadaa5bfc4840aba97426f7157d11b1b3`;
the separate first C prefix is commit
`e461f6486d06ebb22b6b9e47967009ee89e60e7d`. This slice and its
inherited frame were checked independently against original bytes.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| DOS v1.3 `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Executed successor | `GAME.OVR [0x2D683,0x2D691)`; U3B0 `[+0DB4,+0DC2)`; 14 bytes / 8 instructions; SHA-256 `f25a0d186aa9a847bab7b9c5cc291c6b2498cd9f4c986c487c3f825d10b1ecd5` |
| First copy input | Original entry4 `+0D9A..+0DAE` 21 bytes, then `+0DAF` far call to SYSTEM `0AF8:0B25..0B48`, 36 bytes; SYSTEM body SHA-256 `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |
| Stop boundary | `+0DC2` begins the second `9A 25 0B F8 0A` far call; that call is not executed |

The successor loads the second source far pointer from inherited
`SS:[BP+0x26]`, then pushes the source segment, source offset, SS,
destination offset `BP-0x53`, and limit `0x28`. It leaves SP at the
returned frame's `SP-10`, sets AX to `0x28`, and preserves all flags.
It does not read the second source or produce its copy output.

The original-byte oracle starts at entry4 `+0D9A`, executes the first
SYSTEM body and its `RETF 10`, then captures all RAM and CPU state at
`+0DB4`. It independently executes the original 14-byte successor
and C from that same captured frame, comparing every byte of 1 MiB
RAM and all 14 CPU words at `+0DC2`. The 32 controlled cases vary
incoming SP, first source Pascal lengths 0/5/44/60 (with the actual
40-byte cap), second source pointer, and incoming flags. The first
copy output bytes, DS restoration, returned SP, and five outgoing
words are checked. No source bytes or return state are fabricated
after the first call.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 successor `+0DB4..+0DC1` |
| `source_candidates` | `pr_u3b0_entry4_successor_0db4` in `src/recovered/overlay_u3b0_entry4_successor_0db4.c` |
| `signals` | Exact 14-byte hash and 8 decoded instructions, original first-copy return frame, second far-call opcode at stop, inherited five-word call ABI |
| `score` | 32/32 original-copy frames match full 1 MiB RAM and all 14 CPU words; focused strict C11 `-Werror` ASan/UBSan CTest passes |
| `verdict` | `match` for the bounded successor; `unclear` for natural gameplay reach and full entry4 identity |
| `review_notes` | Stops before `+0DC2` execution. No second copy output, downstream branch, or count credit. |

Reproduce with:

```sh
cmake -S . -B resources/build-u3b0-0db4 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0db4 --target poolrad_overlay_u3b0_entry4_successor_0db4_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0db4 \
  -R '^poolrad_overlay_u3b0_entry4_successor_0db4_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-re-tools \
  python3 tests/recovered/check_overlay_u3b0_entry4_successor_0db4.py
```

Original media and sanitizer builds stay under ignored `resources/`.
