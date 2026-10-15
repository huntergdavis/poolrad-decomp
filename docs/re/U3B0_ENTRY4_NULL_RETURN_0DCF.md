# U3B0 entry4 `+0DCF`: reached null path through far return

`src/recovered/overlay_u3b0_entry4_null_return_0dcf.c` executes the
null-head path at `+0DCF..+0DE9` and the shared `+10A1..+10A9`
epilogue. Its input is the `+0DCF` frame reached from original entry4
`+0D9A` after **both** original SYSTEM `0AF8:0B25` copies and the
original `+0DC7` branch. It writes the caller's two output records,
sets the result byte to zero, restores the frame, and executes
`RETF 0x28`. It is a complete null path, not a complete entry4
translation or a natural gameplay frame.

The pre-implementation `deja "Poolrad U3B0 entry4 complete function
+0D9A through return uncovered reached gate original SYSTEM copy
node head"` query found no prior session match. The first uncovered
gate and the two-copy provenance came from the separately reviewed
`+0D9A`, `+0DB4`, and `+0DC7` pieces. A local/remote ownership scan
found no U3B0 `+0DCF` source. This branch is based on published
`2736afab2fe9072a36dfd52c204cd54ec93f09a5` for the `+0DC7`
oracle; its new source and tests have distinct names.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Null branch | `[0x2D69E,0x2D6B9)`; `+0DCF..+0DE9`; 27 bytes / 9 instructions; SHA-256 `27af6cd147143a685677a7bc7e006ac323503866e5b51c2f131b25e9f19ee702` |
| Shared epilogue | `[0x2D970,0x2D979)`; `+10A1..+10A9`; 9 bytes / 4 instructions; SHA-256 `e48af3d400dcabb22010e7972ca8be680047359edd7df4b89cf89381c44466fd` |
| Original code window | `[0x2D669,0x2D979)`; `+0D9A..+10A9`; 784 bytes; SHA-256 `f7da30b1ef0378e05b7543019da9e3b51ef699f8bc88178364750ee6f1de2963` |
| Return | `+0DE7 JMP +10A1`, then `MOV AL,[BP-1]`, `MOV SP,BP`, `POP BP`, `RETF 0x28` |

One controlled frame uses incoming `SP=8000`, Pascal source lengths
`5/9`, and a null node head. After both original copies and the
original gate, `CS:IP=F000:0DCF`, `BP=7FFE`, `SP=7E70`, `AX=0000`,
`DS=6000`, `ES=7000`, `SS=7000`, and flags `0246`. With the verified
original tail loaded, its full 1 MiB RAM SHA-256 is
`eee51fc3bfdba8348ef4fef23ac2b0a7b93eafe08708e1bb3cef8b40ae1467e7`.
The native path goes through `+0DCF,+0DD2,+0DD4,+0DD7,+0DDA,+0DDC,
+0DDF,+0DE3,+0DE7,+10A1,+10A4,+10A6,+10A7` and returns to
`D000:1234` with `SP=802C`. This is controlled reach only.

The parity oracle seeds output pointers **before** execution at
`+0D9A`, runs both original copy bodies and the gate, captures the
exact reached `+0DCF` frame, then compares native instructions and C
from that same frame. **48/48** cases match all 1 MiB RAM bytes and
all 14 CPU words after `RETF 0x28`. Cases vary incoming SP, copy
lengths including the 40-byte cap, incoming flags, ordinary and
identical output pointers, 16-bit `DI+2` wrap, and aliases into the
second pointer, saved BP, and far return frame. The C model reads
later pointers and return words only after earlier writes, preserving
those alias effects. AF after `XOR` is architecturally undefined; the
full-flags comparison follows Unicorn's original-byte convention and
makes no native AF claim.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 null path `+0DCF..+0DE9` plus `+10A1..+10A9` return |
| `source_candidates` | `pr_u3b0_entry4_null_return_0dcf` in `src/recovered/overlay_u3b0_entry4_null_return_0dcf.c` |
| `signals` | Exact original hashes and instruction addresses; two original SYSTEM copies; original null branch; far-return destination and SP measured |
| `score` | 48/48 original-byte full 1 MiB RAM/all14 CPU parity cases; focused strict C11 `-Werror` ASan/UBSan CTest passes |
| `verdict` | `match` for this controlled null return path; `unclear` for natural gameplay reach and complete entry4 identity |
| `review_notes` | The non-null path after `+0DFF` remains separately owned/incomplete. No full-entry4 or strict full-C count credit. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3b0-0dcf \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0dcf --target poolrad_overlay_u3b0_entry4_null_return_0dcf_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0dcf \
  -R '^poolrad_overlay_u3b0_entry4_null_return_0dcf_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3b0_entry4_null_return_0dcf.py
```

Original media and sanitizer builds remain under ignored `resources/`.
