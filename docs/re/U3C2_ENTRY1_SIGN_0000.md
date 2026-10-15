# U3C2 entry 1 `+0000`: complete signed-word sign helper

`src/recovered/overlay_u3c2_sign_entry1_0000.c` translates the
complete DOS v1.3 `GAME.OVR` U3C2 entry 1 body from `+0000`
through its `RETF 2` at `+002B`. It reads the caller's signed word,
returns `FFFF` for negative, `0000` for zero, or `0001` for positive,
and consumes the one-word argument. No calls or external callbacks
occur inside the original body.

The target-specific `deja "Poolrad U3C2 entry1 +0000 sign helper 46
bytes RETF 2 parity ownership"` query found no prior session match.
The [function ledger](overlay_function_ledger.csv) identifies this
as entry slot 1, grade A, 46 decoded bytes, 17 instructions, and no
calls. Local/remote ownership scans found only a disjoint U3C2
entry-3 `+02CC` branch, with no `+0000` source. The original bytes
independently confirm the entry, all three paths, and return boundary.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Complete body | `[0x2FB52,0x2FB80)`; U3C2 `+0000..+002D`; 46 bytes / 17 instructions; SHA-256 `5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf` |
| Control flow | Negative `+000C → +0025`; positive `+0019 → +0025`; zero `+0020 → +0025`; all execute `RETF 2` at `+002B` |

The model executes the prologue's `PUSH BP` and local allocation,
the signed compare branches, the local sign-word write, and the exact
far return. Negative and positive paths retain the last `CMP`
flags; the zero path uses `XOR AX,AX` flags. AF after `XOR` is
architecturally undefined; full-flags parity follows Unicorn's
original-byte convention without a native AF claim.

The original-byte oracle executes the whole body through `RETF 2`
and compares C from the same synthetic caller frame. **60/60** cases
match all 1 MiB RAM bytes and all 14 CPU words. Cases include signed
minimum/maximum, `-1`, zero, positive and negative interior values,
two incoming flag sets, and three stack positions including 16-bit
offset wrap. The focused strict C test verifies each branch's result,
flags, local write, saved BP, and far-return destination.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3C2 entry 1 `+0000..+002D` through `RETF 2` |
| `source_candidates` | `pr_u3c2_sign_entry1_0000` in `src/recovered/overlay_u3c2_sign_entry1_0000.c` |
| `signals` | Exact 46-byte hash, 17 decoded instructions, three complete branches, no calls, and far-return stack effect |
| `score` | 60/60 original-byte full 1 MiB RAM/all14 CPU cases; focused strict C11 `-Werror` ASan/UBSan CTest |
| `verdict` | `match` for the complete controlled original body; natural saved-game reach remains unmeasured |
| `review_notes` | No strict full-C count or native gameplay parity claim before PM review. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3c2-0000 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3c2-0000 --target poolrad_overlay_u3c2_sign_entry1_0000_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3c2-0000 \
  -R '^poolrad_overlay_u3c2_sign_entry1_0000_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3c2_sign_entry1_0000.py
```

Original media and sanitizer builds remain under ignored `resources/`.
