# U3B0 entry4 `+0E5F`: reached finite node-count loop

`src/recovered/overlay_u3b0_entry4_node_count_0e5f.c` executes the
original `+0E5F..+0E96` slice and stops before `+0E97`. It reloads
the caller's head pointer, zeros a second local count, and follows
far next links until the pointer is null. This is a bounded slice of
entry4, not a complete translation or natural saved-game reach.

The target-specific `deja "Poolrad U3B0 entry4 +0E5F +0E97 node count
original bytes parity"` query timed out (exit 124), yielding no
reusable claim. The published [predecessor](U3B0_ENTRY4_NODE_LOOP_0E30.md)
at exact head `3c8e2e1845475fd453876cc291c8b6044e0fa055`
supplies the original two-copy, `+0DFF` caller, `+0067` leaf,
`+0E0C` setup, and `+0E30` flag-loop path. The `+0E5F` oracle
re-executes those bytes before comparing this new slice. Local and
remote ownership scans found no other `+0E5F` source.

| Item | Measured value |
| --- | --- |
| DOS v1.3 `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Node-count slice | `[0x2D72E,0x2D766)`; `+0E5F..+0E96`; 56 bytes / 16 instructions; SHA-256 `aec72078f9e6e042dbc99d93858663fdfc84b48c034d699740a02e7102c6e863` |
| Exit | `JE +0E97` after `OR` of local far-pointer words detects null |
| Traversal | `LES` local pointer, read `ES:[DI+2A/+2C]`, store next link, increment `SS:[BP-8D]`, jump to `+0E73` |

The oracle seeds finite 1–3-node chains before the original `+0067`
caller observes them. It executes the original flag loop from its
reached `+0E30` frame, then compares original node-count bytes and
C from the same reached `+0E5F` frame. **56/56** cases match every
byte of 1 MiB RAM and all 14 CPU words at `+0E97`. Cases cover node
flag patterns, null-link exits, two incoming stack positions, two
Pascal copy-length pairs, and two signed coordinate pairs. The second
count equals the measured chain length in every case.

The C API bounds link traversals. A cycle beyond that bound returns
`LIMIT` at first unexecuted `+0E7D`; it does not assert that cyclic
original input terminates. The focused strict C test covers null
head, one node, deliberate cycle, argument checks, count, and exit.
AF after `XOR` and `OR` is architecturally undefined; full-flags
parity follows Unicorn's original-byte convention without a native
AF claim.

| Field | Verdict |
| --- | --- |
| `target_function` | Original U3B0 entry4 `+0E5F..+0E96` finite node-count slice |
| `source_candidates` | `pr_u3b0_entry4_node_count_0e5f` in `src/recovered/overlay_u3b0_entry4_node_count_0e5f.c` |
| `signals` | Exact 56-byte hash, 16 decoded instructions, and reached original predecessor trace |
| `score` | 56/56 original-byte full 1 MiB RAM/all14 CPU cases; focused strict C11 `-Werror` ASan/UBSan CTest |
| `verdict` | `match` for finite controlled chains to `+0E97`; `unclear` for natural reach and complete entry4 identity |
| `review_notes` | Stops before `+0E97`; later calls and return remain. No full-entry4 or strict full-C count credit. |

Reproduce from this branch's root with the persistent replay checkout:

```sh
cmake -S . -B resources/build-u3b0-0e5f \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build resources/build-u3b0-0e5f --target poolrad_overlay_u3b0_entry4_node_count_0e5f_tests -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir resources/build-u3b0-0e5f \
  -R '^poolrad_overlay_u3b0_entry4_node_count_0e5f_tests$' --output-on-failure
REPLAY_ROOT=/path/to/poolrad-decomp-codex-replay-join
PYTHONPATH="$REPLAY_ROOT/resources/review/capstone5" \
  "$REPLAY_ROOT/resources/venv-string0356/bin/python" \
  tests/recovered/check_overlay_u3b0_entry4_node_count_0e5f.py
```

Original media and sanitizer builds remain under ignored `resources/`.
