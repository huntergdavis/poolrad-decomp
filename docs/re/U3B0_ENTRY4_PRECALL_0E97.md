# U3B0 entry4 `+0E97`: reached prefix before first near call

`src/recovered/overlay_u3b0_entry4_precall_0e97.c` executes the
original `+0E97..+0ED1` prefix and stops before the near `CALL +0CC5`
at `+0ED2`. It makes two signed comparisons against `DS:[6B29]`,
possibly updates that word and a caller byte, increments the caller's
word, and pushes the three words that precede the call. The call
itself and the remainder of entry4 are outside this slice.

The target-specific `deja "Poolrad U3B0 entry4 +0E97 next boundary
after node count first call parity"` lookup yielded no reusable
result. The [reviewed predecessor](U3B0_ENTRY4_NODE_COUNT_0E5F.md)
at exact head `655a48a52ef828b7a78eca0cfd5984f13a082347`
provides the original two-copy, `+0DFF` caller, `+0067` leaf,
`+0E0C` setup, `+0E30` flag loop, and `+0E5F` count-loop path.
The oracle re-executes those original bytes to reach `+0E97`.
Ownership scans found no other `+0E97` source.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Precall prefix | `[0x2D766,0x2D7A1)`; `+0E97..+0ED1`; 59 bytes / 22 instructions; SHA-256 `cd205b64caeb9a17a2f23f4221798e28eb432a44420909d7db6fc28a521d146d` |
| Stop | `CS:IP` at `+0ED2`, before near `CALL +0CC5`; three argument words remain on the stack |
| Branches | Signed `JLE` after `CMP AX,ES:[DI]` and `CMP AX,SS:[BP-8D]`; both outcomes of each observed |

The original-byte oracle reaches `+0E97` through the predecessor
path. At that boundary, it deliberately varies only `DS:[6B29]`
and the pointed caller word; the caller byte is reset to zero. These
are controlled branch inputs, not values claimed from a saved game.
**128/128** cases match all 1 MiB RAM bytes and all 14 CPU words
at `+0ED2`. The native trace covers all four branch pairs, with
positive and negative 16-bit comparison values, 1- and 3-node
predecessor chains, two stack positions, two Pascal copy-length
pairs, and two signed coordinate pairs. The focused strict C test
checks updates and the exact pushed `CS`, `BP`, and `AX` words.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 `+0E97..+0ED1` precall prefix |
| `source_candidates` | `pr_u3b0_entry4_precall_0e97` in `src/recovered/overlay_u3b0_entry4_precall_0e97.c` |
| `signals` | Exact 59-byte hash, 22 decoded instructions, reached predecessor trace, all four signed branch pairs |
| `score` | 128/128 original-byte full 1 MiB RAM/all14 CPU cases; focused strict C11 `-Werror` ASan/UBSan CTest |
| `verdict` | `match` for the controlled prefix to `+0ED2`; `unclear` for natural reach and complete entry4 identity |
| `review_notes` | Stops before `CALL +0CC5`; no full-entry4 or strict full-C count credit. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3b0-0e97 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0e97 --target poolrad_overlay_u3b0_entry4_precall_0e97_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0e97 \
  -R '^poolrad_overlay_u3b0_entry4_precall_0e97_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3b0_entry4_precall_0e97.py
```

Original media and sanitizer builds remain under ignored `resources/`.
